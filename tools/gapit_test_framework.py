#!/usr/bin/python
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
'''The main test framework for all Gapit tests.
'''

import argparse
import fnmatch
import inspect
import os
import re
import tempfile
import shutil
import subprocess
import sys
import traceback
from distutils.version import StrictVersion

from gapit_tester import run_on_single_apk
from gapit_tester import RunArgs
from gapit_trace_reader import get_device_and_architecture_info_from_trace_file
from gapit_trace_reader import parse_trace_file, NamedAttributeError

SUCCESS = 0
FAILURE = 1
WARNING = 2
SKIPPED = 3

PIXEL_C = {"vendor_id": 0x10DE, "device_id": 0x92BA03D7}
NVIDIA_K2200 = {"vendor_id": 0x10DE, "device_id": 0x13ba}
NVIDIA_965M = {"vendor_id": 0x10DE, "device_id": 0x1427}

UNKNOWN_OS, WINDOWS, OSX, LINUX, ANDROID = range(5)


class GapitTestException(Exception):

    """Base test exception"""
    pass


class GapidUnsupportedException(GapitTestException):

    """Unsupported test exception"""
    pass


def get_read_offset_function(atom, pointer):
    """Returns a function that is a integer field decoder for any atom.
    It decodes from the atoms read_data, starting at the given pointer"""

    def function(offset, size):
        return little_endian_bytes_to_int(
            require(atom.get_read_data(pointer + offset, size), 3))

    return function


def get_write_offset_function(atom, pointer):
    """Returns a function that is a integer field decoder for any atom.
    It decodes from the atoms write_data, starting at the given pointer"""

    def function(offset, size):
        return little_endian_bytes_to_int(
            require(atom.get_write_data(pointer + offset, size), 3))

    return function


def little_endian_bytes_to_int(val):
    '''Takes a sequence of bytes in little-endian format, and turns
    them into an integer'''
    total = 0
    place = 1
    for byte in val:
        total = total + (byte * place)
        place *= 256
    return total


def require(val, stack_depth=0):
    '''Takes a tuple (Object, String).

    If Object is None, throws an exception with the string and fails the test,
    otherwise, prints nothing and returns the Object. stack_depth is the
    stack_frame to display, where 0 is the callee of this function.
    '''
    if val[0] is not None:
        return val[0]
    else:
        call_site = traceback.format_list(
            traceback.extract_stack(limit=2 + stack_depth))
        raise GapitTestException(val[1] + "\n" + call_site[0])


def require_true(val, stack_depth=0):
    """Takes a value. If that value is not true, throws an exception with an
    error message"""
    if val:
        return
    call_site = traceback.format_list(
        traceback.extract_stack(limit=2 + stack_depth))
    raise GapitTestException("Expected:" + str(val) + "to be true\n" +
                             call_site[0])


def require_false(val, stack_depth=0):
    """Takes a value. If that value is true, throws an exception with an
    error message"""
    if not val:
        return
    call_site = traceback.format_list(
        traceback.extract_stack(limit=2 + stack_depth))
    raise GapitTestException("Expected:" + str(val) + "to be false\n" +
                             call_site[0])


def require_equal(param, val, stack_depth=0):
    """Takes 2 values. If they are equal, does nothing, otherwise
    throws an exception with an error message"""
    # For floating point numbers, we do equivalence check like other types.
    # But still, we need to make sure both the expected and actual values
    # are floating point numbers, to avoid accidentally converting types
    # and altering values.
    float_count = int(isinstance(param, float)) + int(isinstance(val, float))
    if float_count >= 1:
        assert float_count == 2, "should compare float with float"

    if type(val)(param) == val:
        return
    call_site = traceback.format_list(
        traceback.extract_stack(limit=2 + stack_depth))
    raise GapitTestException("Expected:" + str(param) + "==" + str(val) + "\n" +
                             call_site[0])


def require_not_equal(param, val, stack_depth=0):
    """Takes 2 values. If they are not equal, does nothing, otherwise
    throws an exception with an error message"""
    if type(val)(param) != val:
        return
    call_site = traceback.format_list(
        traceback.extract_stack(limit=2 + stack_depth))
    raise GapitTestException("Expected:" + str(param) + "!=" + str(val) + "\n" +
                             call_site[0])


def require_not(val, stack_depth=0):
    '''Takes a tuple (Object, String). If Object is not None, throws an
    exception stating that we expected this to not exist. Otherwise does
    nothing'''
    if val[0] is not None:
        call_site = traceback.format_list(
            traceback.extract_stack(limit=2 + stack_depth))
        raise GapitTestException("Found element we were not expecting\n" +
                                 call_site[0])


class GapitTest(object):

    '''Base class for all Gapit tests.

    This is responsible for tracing our application as well as processing the
    log file. Any subclasses 'expect' method will be run.
    '''

    def __init__(self):
        self.device = None
        self.architecture = None
        self.atom_generator = None
        self.warnings = []

    def name(self):
        """Returns the name of this class."""
        return self.__class__.__name__

    def next_call_of(self, call_name):
        '''Consumes atoms until a call of the given type is found, returns
        a tuple (Atom, ""). Returns (None, "error_message") if the atom could
        not be found'''
        if self.verbose:
            print "Finding next {}".format(call_name)
        try:
            while True:
                atom = self.atom_generator.next()
                if atom.name == call_name:
                    return (atom, "")
                if self.verbose:
                    print "Skipping {}".format(atom.name)
        except StopIteration:
            return (None, "Could not find atom of type " + call_name)

    def nth_call_of(self, call_name, index):
        '''Consumes atoms until the nth call of the given type is found,
           returns a tuple (Atom, ""). Returns (None, "error_message") if
           the atom could not be found'''
        atom = (None, "")
        if self.verbose:
            print "Finding {}th {}".format(index, call_name)
        for _ in range(0, index):
            atom = self.next_call_of(call_name)
        if atom[0]:
            return (atom[0], "")
        else:
            return (
                None,
                "Could not find " + str(index) + " atoms of type " + call_name)

    def next_call(self, call_name):
        '''Expects the next call to be of the specified type. Returns
        a tuple (Atom, ""), if it could be found, otherwise returns
        (None, "Error_message")
        '''
        if self.verbose:
            print "Expecting next atom is {}".format(call_name)
        try:
            atom = self.atom_generator.next()
            if atom.name == call_name:
                return (atom, "")
        except StopIteration:
            return (None, "The next atom was not of type " + call_name)

    def not_android_version(self, android_version_name):
        """If the trace device info matches indicates that the application is
        running on Android and the version name matches with the given version
        name, emits a warning and returns False. Otherwise returns True."""
        if (self.device.Configuration.OS.Kind == ANDROID and
                StrictVersion(self.device.Configuration.OS.Name) <=
                StrictVersion(android_version_name)):
            call_site = traceback.format_list(traceback.extract_stack(limit=2))
            self.warnings.append(
                "Code block disabled due to known Android issue\n" + call_site[
                    0])
            return False
        return True

    def not_device(self, device_properties, driver, device):
        """If the device_properties matches the given
        device, and the device_properties driver_version is less
        than driver, Emits a warning, and returns False, otherwise
        returns True.

        device_properties is expected to be a valid
        vkGetPhysicalDeviceProperties
        Atom.

        driver is expected to be a numerical value.

        device is expected to be a map containing
            { "vendor_id": int, "device_id": int }
        """

        # VkPhysicalDeviceProperties looks like:
        # typedef struct VkPhysicalDeviceProperties {
        #     uint32_t                            apiVersion;
        #     uint32_t                            driverVersion;
        #     uint32_t                            vendorID;
        #     uint32_t                            deviceID;
        #     VkPhysicalDeviceType                deviceType;
        #     char                                deviceName[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE];
        #     uint8_t                             pipelineCacheUUID[VK_UUID_SIZE];
        #     VkPhysicalDeviceLimits              limits;
        #     VkPhysicalDeviceSparseProperties    sparseProperties;
        # } VkPhysicalDeviceProperties;

        driver_version = little_endian_bytes_to_int(
            require(
                device_properties.get_write_data(
                    device_properties.hex_pProperties + 4, 4)))
        vendor_id = little_endian_bytes_to_int(
            require(
                device_properties.get_write_data(
                    device_properties.hex_pProperties + 2 * 4, 4)))
        device_id = little_endian_bytes_to_int(
            require(
                device_properties.get_write_data(
                    device_properties.hex_pProperties + 3 * 4, 4)))
        if (device["vendor_id"] == vendor_id and
                device["device_id"] == device_id and driver_version <= driver):
            call_site = traceback.format_list(traceback.extract_stack(limit=2))
            self.warnings.append("Code block disabled due to known driver bug\n"
                                 + call_site[0])
            return False
        return True

    def run_apk(self, verbose, apk_name, capture_name):
        args = RunArgs()
        if verbose:
            args.set_verbose()
        args.set_output(capture_name)
        setattr(args, "verbose", verbose)
        setattr(args, "keep", False)
        setattr(args, "output", [capture_name])
        print "[ " + "TRACING".center(10) + " ] " + apk_name
        try:
            return_value = run_on_single_apk(apk_name, args)
        except:
            return (FAILURE, "Could not generate trace file.")
        if return_value != 0:
            return (FAILURE, "Could not generate trace file.")
        if verbose:
            print "Successfully traced application"
        print "[ " + "DONE".center(10) + " ] " + apk_name
        return (SUCCESS, "")

    def run_host(self, verbose, host_name, capture_name):
        print "[ " + "TRACING".center(10) + " ] " + host_name
        try:
            gapit_args = ['gapit']
            if verbose:
                gapit_args.extend(['-log-level', 'Debug'])
            gapit_args.extend(['trace', '-out',
                               capture_name, '-local-app', host_name])
            if verbose:
                subprocess.check_call(gapit_args)
            else:
                subprocess.check_call(
                    gapit_args, stdout=open(os.devnull, 'wb'),
                    stderr=open(os.devnull, 'wb'))
        except:
            return (FAILURE, "Could not generate trace file.")
        if verbose:
            print "Successfully traced application"
        print "[ " + "DONE".center(10) + " ] " + host_name
        return (SUCCESS, "")

    def run_test(self, verbose, host, capture_directory):
        '''Runs this test case. Returns a tuple,
            (FAILURE|SUCCESS|WARNING, message) on completion.'''

        program_name = getattr(self, "gapit_test_name")
        self.verbose = verbose
        capture_name = os.path.join(
            capture_directory, program_name + ".gfxtrace")

        if not os.path.isfile(capture_name):
            # If the file has not been created yet, in this test run, then
            # generate the capture file contents. This is a slow process so
            # only do it once.
            if not host:
                success, error = self.run_apk(
                    verbose, program_name + ".apk", capture_name)
            else:
                success, error = self.run_host(
                    verbose, program_name, capture_name)
            if not success == SUCCESS:
                return (success, error)
        test_name = program_name + "." + self.name()
        self.device, self.architecture = get_device_and_architecture_info_from_trace_file(
            capture_name)
        if self.architecture is None:
            return (FAILURE, "Failed to obtain device architecture info from trace")
        self.atom_generator = parse_trace_file(capture_name)

        def clear_atoms(status, message):
            # Empty out all of the atoms in the generator, this way gapit can be
            # killed.
            for x in self.atom_generator:
                pass
            return (status, message)
        print "[ " + "RUN".ljust(10) + " ] " + test_name
        try:
            getattr(self, "expect")()
        except GapidUnsupportedException as error:
            print "[ " + "SKIPPED".rjust(10) + " ] " + test_name
            print "     " + error.message
            return clear_atoms(SKIPPED, error.message)
        except GapitTestException as error:
            print "[ " + "FAILED".rjust(10) + " ] " + test_name
            print "     " + error.message
            return clear_atoms(FAILURE, error.message)
        except NamedAttributeError as error:
            exc_type, exc_value, exc_tb = sys.exc_info()
            print "[ " + "FAILED".rjust(10) + " ] " + test_name
            call_site = traceback.format_exception(
                exc_type, exc_value, exc_tb, limit=2)
            print "    " + error.message + call_site[1]
            return clear_atoms(FAILURE, error.message)
        return_val = WARNING if len(self.warnings) > 0 else SUCCESS
        if return_val == WARNING:
            print "[ " + "WARNING".rjust(10) + " ]"
            for warning in self.warnings:
                print warning
        print "[ " + "OK".rjust(10) + " ] " + test_name
        return clear_atoms(return_val, None)


def gapit_test(gapit_test_name):
    ''' This is used as a decoration to classes.

        It checks that the class it is attached to is of the right type,
        and sets the program_name on it.'''

    def checked_decorator(clss):
        """ This actually performs all of the checks"""
        if not inspect.isclass(clss):
            raise GapitTestException("The test case should be a class")
        if not issubclass(clss, GapitTest):
            raise GapitTestException(
                "The test case should be a subclass of GapitTest")
        clss.gapit_test_name = gapit_test_name
        return clss

    return checked_decorator


class TestManager(object):

    """Manages and runs all requested tests."""

    def __init__(self, root_directory, verbose, host):
        """root_directory is the directory that contains all of the tests"""
        self.root_directory = root_directory
        self.verbose = verbose
        self.host = host
        # The key is the name of the APK, and the value is a list of tuples
        # of (name, GapitTest) for the APK
        self.tests = {}
        self.number_of_tests_run = 0
        self.failed_tests = []
        self.warned_tests = []
        self.skipped_tests = []

    def gather_all_tests(self, include_regex, exclude_regex):
        '''Finds all classes in all python files that contain the attribute
            @gapit_test whose names match include_regex and do not match
            exclude_regex'''
        include_matcher = re.compile(include_regex)
        exclude_matcher = re.compile(exclude_regex)

        # __import__ does not re-generated *.pyc files correctly,
        # so remove it all-together, it would be best to not clutter
        # our directory anyway
        sys.dont_write_bytecode = True

        default_path = sys.path
        for root, _, filenames in os.walk(self.root_directory):
            for filename in fnmatch.filter(filenames, "*.py"):
                sys.path = default_path
                sys.path.append(root)
                if self.verbose:
                    print("Searching " + os.path.join(root, filename) +
                          " for tests")
                try:
                    mod = __import__(os.path.splitext(filename)[0])
                    for _, obj in inspect.getmembers(mod):
                        if inspect.isclass(obj) and hasattr(
                                obj, "gapit_test_name"):
                            if self.verbose:
                                print("Found test " + obj.__name__ + " in " +
                                      os.path.join(root, filename))
                            test_name = os.path.relpath(root,
                                                        self.root_directory)
                            test_name = test_name.replace("/", ".")
                            test_name = test_name.replace("\\", ".")
                            test = obj()
                            test_name += "." + test.name()
                            if (include_matcher.match(test_name) and
                                    exclude_matcher.match(test_name) is None):
                                self.add_test(obj.gapit_test_name,
                                              test_name, test)
                except:
                    print("Warning: Could not parse " + os.path.join(
                        root, filename) + " for tests.")
                    pass

    def print_test_names(self, file_handle):
        """Prints out to the given file handle a list of all tests."""
        for _, tests in self.tests.iteritems():
            for test in tests:
                file_handle.write(test[0] + "\n")

    def add_test(self, program_name, test_name, test):
        """Adds a test case to the manager from the given apk"""
        if program_name not in self.tests:
            self.tests[program_name] = []
        self.tests[program_name].append((test_name, test))

    def run_all_tests(self, temp_directory):
        """Runs all of the tests"""
        for apk in sorted(self.tests.keys()):
            for test in sorted(self.tests[apk], key=lambda x: x[0]):
                self.accumulate_result(
                    test[0], test[1].run_test(self.verbose, self.host,
                                              temp_directory))

    def accumulate_result(self, test_name, result):
        """Tracks the results for the given test"""
        self.number_of_tests_run += 1
        if result[0] == FAILURE:
            self.failed_tests.append((test_name, result[1]))
        if result[0] == WARNING:
            self.warned_tests.append(test_name)
        if result[0] == SKIPPED:
            self.skipped_tests.append(test_name)

    def print_summary_and_return_code(self):
        '''Prints a summary of the test results. Returns 0 if every test
        was successful and -1 if any test failed.
        Warnings do not count as failure, but are printed out in the summary.'''
        print "Total Tests Run: " + str(self.number_of_tests_run -
                                        len(self.skipped_tests))
        print("Total Tests Passed: " +
              str(self.number_of_tests_run - len(self.failed_tests) -
                  len(self.skipped_tests)))
        if self.warned_tests:
            print "Total Test Warnings: " + str(len(self.warned_tests))
        for warning in self.warned_tests:
            print "[ " + "WARNING".rjust(10) + " ] " + warning
        if self.skipped_tests:
            print "Total Skipped Tests: " + str(len(self.skipped_tests))
        for skipped in self.skipped_tests:
            print "[ " + "SKIPPED".rjust(10) + " ] " + skipped
        for failure in self.failed_tests:
            print "[ " + "FAILED".rjust(10) + " ] " + failure[0]
            print "     " + failure[1]
        if self.failed_tests:
            return -1
        return 0


def main():
    """Entry-point for the testing framework"""
    parser = argparse.ArgumentParser(description='''
This testing framework will look in a given directory and accumulate
tests from there.

Any class that is a subclass of GapitTest and has the annotation @gapit_test
will be considered.

It is expected the CWD of this script is the output directory for the APK files
from a build.
        ''')
    parser.add_argument(
        "--test-dir", nargs=1, help="Directory to gather the tests from")
    parser.add_argument(
        "--verbose", action="store_true", help="Show verbose output")
    parser.add_argument(
        "--list-tests",
        action="store_true",
        help="Only print the names of all tests")
    parser.add_argument(
        "--host",
        action="store_true",
        help="Run the tests on the host, not an android device")
    parser.add_argument(
        "--include",
        default=".*",
        help="Run tests matching this regular expression")
    parser.add_argument(
        "--exclude",
        default="^$",
        help="Exclude tests that match this regular expression")
    args = parser.parse_args()

    test_directory = os.getcwd()
    if args.test_dir:
        test_directory = args.test_dir[0]

    manager = TestManager(test_directory, args.verbose, args.host)
    manager.gather_all_tests(args.include, args.exclude)

    if args.list_tests:
        manager.print_test_names(sys.stdout)
        return 0
    else:
        test_directory = tempfile.mkdtemp("", "GAPID-")
        print "Running tests in {}".format(test_directory)
        manager.run_all_tests(test_directory)
        shutil.rmtree(test_directory)
        return manager.print_summary_and_return_code()


if __name__ == "__main__":
    sys.exit(main())
