#!/usr/bin/python
# -*- coding: utf-8 -*-
# Copyright 2017 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""This contains functions for parsing a trace file using gapit dump."""

import argparse
import json
import os
import re
import subprocess
import sys
from collections import namedtuple


class Observation(object):

    '''A single observation.

    Contains the start, end and id of the observation as well as
    the memory that was actually observed.'''

    def __init__(self, observation_type, memory_start, memory_end, memory_id, data):
        self.type = observation_type
        self.contents = data
        self.memory_start = memory_start
        self.memory_end = memory_end
        self.memory_id = memory_id

    def __str__(self):
        val = "   {} : [{:016x} - {:016x}]\n{}".format(
            self.type, self.memory_start, self.memory_end,
            ''.join(["{:02x}".format(i) for i in self.contents]))
        return val

    def get_memory_range(self):
        """ Returns a pair (start, end) of the memory region"""
        return (self.memory_start, self.memory_end)

    def get_memory(self):
        """ Returns the bytes inside the range"""
        return self.contents


def find_memory_in_observations(observations, address, num_bytes):
    '''Searches for the given range of bytes in the given array of observations.

    Returns the bytes if they exist, or returns None if they could not be
    found
    '''

    for observation in observations:
        rng = observation.get_memory_range()
        last_address = address + num_bytes - 1
        if (address <= rng[1] and address >= rng[0] and
                last_address <= rng[1] and last_address >= rng[0]):
            return observation.get_memory()[address - rng[0]:address - rng[0] +
                                            num_bytes]
    return None


def find_string_in_observations(observations, address):
    '''Finds a null-terminated string starting at the given address in
    the observations. Returns a python string containing all of the non
    null bytes, or None if it could not be found'''
    for observation in observations:
        rng = observation.get_memory_range()
        if address <= rng[1] and address >= rng[0]:
            memory = observation.get_memory()
            string_data = bytearray()
            for idx in range(address - rng[0], rng[1] - rng[0] + 1):
                ch = memory[idx]
                if ch is 0:
                    return str(string_data)
                string_data.append(ch)
    return None


class NamedAttributeError(AttributeError):

    """An exception that is thrown when trying to access a parameter from an
    object"""

    def __init__(self, message):
        super(NamedAttributeError, self).__init__(message)


class Extra(object):

    """A single extra attached to an atom"""

    def __init__(self, name, parameters):
        self.name = name
        self.parameters = {}
        for parameter in parameters:
            self.parameters[parameter[0]] = parameter[1]

    def __getattr__(self, name):
        """Returns parameters that were on this extra.

        Based on the prefix (hex_, int_, float_, <none>) we convert the
        parameter from a string with the given formatting.
        """

        if name.startswith('hex_') or name.startswith('int_'):
            if name[4:] in self.parameters:
                if name.startswith('hex_'):
                    return int(self.parameters[name[4:]], 16)
                elif name.startswith('int_'):
                    return int(self.parameters[name[4:]])

        if name.startswith('float_'):
            if name[6:] in self.parameters:
                return float(self.parameters[name[6:]])

        if name in self.parameters:
            return self.parameters[name]
        raise NamedAttributeError('Could not find parameter ' + name +
                                  ' on extra ' + self.name + '\n')


class Atom(object):

    '''A single Atom that was observed.

    Contains all of the memory observations, parameters, index and return code
    of the atom.'''

    def __init__(self, index, name, return_val):
        self.parameters = {}
        self.read_observations = []
        self.write_observations = []
        self.index = index
        self.name = name
        self.return_val = return_val

    def __getattr__(self, name):
        """Returns parameters that were on this atom.

        Based on the prefix (hex_, int_, float_, <none>) we convert the
        parameter from a string with the given formatting.
        """

        # A helper function to iterate through each element of a given array
        # to process the array type parameters
        def process_array_parameter(parameter_array, f):
            l = []
            for p in parameter_array:
                l.append(f(p))
            return l

        if name.startswith('hex_') or name.startswith('int_'):
            if name[4:] in self.parameters:
                if name.startswith('hex_'):
                    p = self.parameters[name[4:]]
                    if isinstance(p, list):
                        return process_array_parameter(p, lambda x: int(x, 16))
                    else:
                        return int(p, 16)
                elif name.startswith('int_'):
                    p = self.parameters[name[4:]]
                    if isinstance(p, list):
                        return process_array_parameter(p, lambda x: int(x))
                    else:
                        return int(p)

        if name.startswith('float_'):
            if name[6:] in self.parameters:
                p = self.parameters[name[6:]]
                if isinstance(p, list):
                    return process_array_parameter(p, lambda x: float(x))
                else:
                    return float(self.parameters[name[6:]])

        if name in self.parameters:
            return self.parameters[name]
        if name.startswith('extra_'):
            if name[6:] in self.extras:
                return self.extras[name[6:]]

        raise NamedAttributeError('Could not find parameter ' + name +
                                  ' on atom [' + str(
                                      self.index) + ']' + self.name + '\n')

    def num_observations(self):
        """Returns the number of observations on this object"""
        return len(self.read_observations) + len(self.write_observations)

    def get_read_data(self, address, num_bytes):
        """Returns num_bytes from the starting address in the read observations.

        If the range address->address + num_bytes is not contained in any
        combination of read Observations (None, "Error_Message") is returned,
        otherwise (bytes, "") is returned
        """
        mem = find_memory_in_observations(self.read_observations, address,
                                          num_bytes)
        if mem is not None:
            return (mem, '')

        return (None, 'Could not find a read observation starting at ' +
                str(hex(address)) + ' containing ' + str(num_bytes) + ' bytes')

    def get_read_string(self, address):
        """Returns a null-terminated string starting at address in the
        read observations.

        If there is no null-terminated string at address, then
        (None, "Error_Message") is returned otherwise (string, "")
        is returned"""
        mem = find_string_in_observations(self.read_observations, address)
        if mem is not None:
            return mem, ''

        return (None, 'Could not find a string starting at ', address)

    def get_write_data(self, address, num_bytes):
        """Returns num_bytes from the starting address in the write
        observations.

        If the range address->address + num_bytes is not contained in a write
        Observation (None, "Error_Message") is returned, otherwise (bytes, "")
        is returned
        """
        mem = find_memory_in_observations(self.write_observations, address,
                                          num_bytes)
        if mem is not None:
            return (mem, '')
        else:
            return (None, 'Could not find a write observation starting at ',
                    str(address), ' containing ', str(num_bytes), ' bytes')

    def get_write_string(self, address):
        """Returns a null-terminated string starting at address in the
        read observations.

        If there is no null-terminated string at address, then
        (None, "Error_Message") is returned otherwise (string, "")
        is returned"""
        mem = find_string_in_observations(self.write_observations, address)
        if mem is not None:
            return mem, ''

        return (None, 'Could not find a string starting at ', address)

    def add_parameter(self, name, value):
        """Adds a parameter with the given name and value to the atom."""
        self.parameters[name] = value

    def get_parameter(self, parameter_name):
        '''Expects there to be a paramter of the given name on the given atom.
        Returns (parmeter, "") if it exists, and (None, "Error_message") if it
        does not'''
        if parameter_name in self.parameters:
            return (self.parameters[parameter_name], '')
        else:
            return (None, 'Could not find paramter ' + parameter_name +
                    ' on atom [' + str(self.index) + '] ' + self.name())

    def add_read_observation(self, memory_start, memory_end, memory_id, data):
        '''Adds a read observation to the atom. Does not associate any memory
        with the read'''
        self.read_observations.append(
            Observation("R", memory_start, memory_end,
                        memory_id, data))

    def add_write_observation(self, memory_start, memory_end, memory_id, data):
        '''Adds a write observation to the atom. Does not associate any memory
        with the write'''
        self.write_observations.append(
            Observation("W", memory_start, memory_end,
                        memory_id, data))

    def add_extra(self, name, parameters):
        """Takes a name and an array of parameter tuples (name, value) and adds
        this extra to the atom"""
        self.extras[name] = Extra(name, parameters)

    def __str__(self):
        val = "[{}] {}({})".format(self.index, self.name, self.parameters)
        if self.return_val:
            val += " -> {}".format(self.return_val)
        if self.read_observations or self.write_observations:
            val += "\n"
            val += "\n".join([
                "\n".join([str(observation)
                           for observation in self.read_observations]),
                "\n".join([str(observation)
                           for observation in self.write_observations])])
        return val


def parse_atom_line(line):
    '''Parses a single line from a trace dump.

    Example lines:
    [0] switchThread(threadID: 1)
    [36] vkGetDeviceProcAddr(device: 3853508608, pName: vkGetDeviceProcAddr) → 0xd3a0e6c5

    This is expected to be first line of a new atom
    '''
    match = re.match(r'\[(\d+)\] ([a-zA-Z0-9]*)\((.*)\)(?: → (.*))?', line)
    if not match:
        print 'failed line:: "{}"'.format(line)
    number = int(match.group(1))
    command = match.group(2)

    all_parameters = match.group(3)
    # Group 0: Parameter name
    # Group 1: Array type parameter, empty if the parameter is not array type
    # Group 2: Non-array type parameter
    parameters = re.findall(
        r'([a-zA-Z]*): (?:\[(.*)\]|([a-zA-Z0-9\.]*))', all_parameters)

    return_val = None
    if match.group(4):
        return_val = match.group(4)

    atom = Atom(number, command, return_val)
    for parameter in parameters:
        if parameter[1] != '':
            atom.add_parameter(parameter[0], parameter[1].split(' '))
        else:
            atom.add_parameter(parameter[0], parameter[2])

    return atom


def parse_memory_observations(observation):
    '''Parses all memory observations from a formatted list of observations.

    Example Line:
    {Range: [0x00000000eaafff34-0x00000000eaafff37],                           \
        ID: 8f85530ad0db61eee69aa0ba583751043584d500}

    Returns a list of tuples containing the range start, range end, and ID of
    the observation.
    '''
    observations = re.findall(
        r'\{Range: \[0x([0-9a-f]+)-0x([0-9a-f]+)\], ID: ([0-9a-f]+)\}',
        observation)
    for observation in observations:
        yield observation


def parse_extra(name, extra_text, atom):
    """Parses extra_text from an extra named `extra_name`


    Example extra_text:
    &{Generate:{} U64Alignment: 8}
    """
    interior = extra_text.lstrip('&').strip('{}')
    parameters = re.findall(r'([a-zA-Z0-9]+):(\S)+', interior)
    atom.add_extra(name, parameters)


def parse_observation(observation_type, start, end, line, atom):
    '''Parses the second line of a memory observation.
    '''
    start_match = re.match(
        r'(?:0x)?([0-9a-f]+)(@[0-9]+)?', start)
    start_pool = 0
    if start_match.group(2):
        start_pool = int(start_match.group(2))

    end_match = re.match(
        r'(?:0x)?([0-9a-f]+)(@[0-9]+)?', end)

    if observation_type == "R":
        atom.add_read_observation(
            int(start_match.group(1), 16), int(end_match.group(1), 16),
                start_pool, parse_memory_line(line))
    else:
        atom.add_write_observation(
            int(start_match.group(1), 16), int(end_match.group(1), 16),
                start_pool, parse_memory_line(line))


def parse_memory_line(line):
    """Parses the memory lines of an atom, returns the bytes of the
    observation

    2c00f4a6
    """
    return [int(line[i:i + 2], 16) for i in range(0, len(line.strip()), 2)]

# FIRST_LINE These are the states that we use when parsing.
# is the first line in the entire file
FIRST_LINE = 1
# NEW_ATOM is the state we are in just before reading a new atom
NEW_ATOM = 2
# MEMORY is the state we are in when we have finished reading the ATOM
# and now we have to read all of the memory
MEMORY = 3


def next_line(proc):
    ''' Returns the next valid line from the input '''

    while True:
        line = proc.stdout.readline()
        if line == '\n':
            continue
        if line == '\r\n':
            continue
        if re.match(r'\d\d?:\d\d?:\d\d?\.\d*.* <gapi\w>', line):
            continue
        return line.strip("\r\n").strip()


def get_device_and_architecture_info_from_trace_file(filename):
    '''Parses a trace file and returns the device architecture info

    Arguments:
        filename is the name of the trace file to read
    Return:
        device architecture that contains device hardware info if the device
        info header is parsed successfully, otherwise returns None
    '''
    proc = subprocess.Popen(
        ['gapit', '-log-level', 'Fatal', 'dump',
            '-showabiinfo', '-showdeviceinfo', filename],
        stdout=subprocess.PIPE)

    def get_json_str():
        return_str = ""
        line = next_line(proc)
        while not line.startswith('}') or not line.strip() == '}':
            return_str += line
            line = proc.stdout.readline()
            if line == '':
                return None
        # append the ending '}'
        return_str += line.strip() + "\n"
        return return_str

    device = None
    architecture = None

    # This parsing routine is basically a state machine to parse the device info
    # which is stored in json string form.
    # The first several lines of the process is un-necessary.
    # readline keeps the newline on the end so EOF is when readline returns
    # an empty string.
    line = next_line(proc)
    while line != '':
        if line.strip() == "Device Information:":
            device_info_str = get_json_str()
            device = json.loads(device_info_str,
                                object_hook=lambda d: namedtuple('Device', d.keys())(*d.values()))

        elif line.strip() == "Trace ABI Information:":
            abi_info_str = get_json_str()
            abi_info = json.loads(abi_info_str)
            memory_layout = abi_info["memory_layout"]
            layout_dict = {}
            for t in memory_layout:
                if not isinstance(memory_layout[t], dict):
                    continue
                for p in memory_layout[t]:
                    key = 'int_' + t[0].lower() + t[1:] + p.title()
                    layout_dict[key] = memory_layout[t][p]
            architecture = type("Architecture", (), layout_dict)
        if (device is not None) and (architecture is not None):
            break
        line = next_line(proc)
    proc.kill()
    return device, architecture


def parse_trace_file(filename):
    '''Parses a trace file, for every atom parsed, yields an atom.

    Arguments:
        filename is the name of the trace file to read
    '''
    proc = subprocess.Popen(
        ['gapit', '-log-level', 'Fatal', 'dump',
            '-observations-data', '-raw', filename],
        stdout=subprocess.PIPE, stderr=open(os.devnull, 'wb'))

    # This parsing routine is basically a state machine
    # The very first several lines of the process is un-necessary
    # Every atom has 3 parts:
    # The first several lines are the number/name/parameters
    # The second line is information about the read/write locations
    # The subsequent lines are the memory contents of those read/write locations
    #   IMPORTANT: These memory contents are what is present AFTER the
    #              call is made, not before. So you cannot get information
    #              about what the memory was before a call.
    num_read_observations = 0
    num_write_observations = 0
    current_memory_observation = 0
    current_state = FIRST_LINE
    atom = None
    # readline keeps the newline on the end so EOF is when readline returns
    # an empty string
    line = next_line(proc)
    while line != '':
        if current_state == FIRST_LINE:
            # Only atom lines starts with an index number, and the first atom
            # always has index value 0, so the string starts with '0' is the
            # first atom line.
            if line.startswith('[0]'):
                current_state = NEW_ATOM
                continue
        elif current_state == NEW_ATOM:
            if atom:
                yield atom
            atom = parse_atom_line(line)
            current_state = MEMORY
        elif current_state == MEMORY:
            match = re.match(
                r'\s*(R|W): \[((?:0x)?[0-9a-f]+) - ((?:0x)?[0-9a-f]+)\]', line)
            match_empty = re.match(r'\s*(R|W): \[0 - 0\]', line)
            if not match:
                current_state = NEW_ATOM
                continue
            line = next_line(proc)
            parse_observation(
                match.group(1), match.group(2), match.group(3), line, atom)
        line = next_line(proc)
    if atom:
        yield atom
    proc.kill()
import time


def main():
    """Main entry point for the parser."""
    parser = argparse.ArgumentParser(
        description='Prints out the name of all atoms found in a trace file')
    parser.add_argument('trace', help='trace file to read')
    args = parser.parse_args()

    for atom in parse_trace_file(args.trace):
        print atom
    return 0


if __name__ == '__main__':
    sys.exit(main())
