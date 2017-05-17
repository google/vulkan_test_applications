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
'''This contains useful functions for talking to android devices and getting
information about .apks'''

import collections
import os
import subprocess


def adb(params, program_args):
    '''Runs a single command through ADB.

    Arguments:
        params: A list of the parameters to pass to adb
        program_args: The arguments to this program.

        program_args must contain a .verbose member.

        If program_args.verbose is true, then the command and the output is
          printed,
        otherwise no output is present.
    '''
    args = ['adb']
    args.extend(params)
    if program_args.verbose:
        print args
        subprocess.check_call(args)
    else:
        subprocess.check_call(
            args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)


def adb_stream(params, program_args):
    '''Runs a single command through ADB, returns the process with stdout
    redirected.

    Arguments:
        params: A list of the parameters to pass to adb
        program_args: The arguments to this program.

        program_args must contain a .verbose member.

        If program_args.verbose is true, then the command is printed.
    '''
    args = ['adb']
    args.extend(params)
    if program_args.verbose:
        print args
    return subprocess.Popen(args, stdout=subprocess.PIPE)


def install_apk(apk_info, program_args):
    '''Installs an apk.

    Overwrites existing APK if it exists and grants all permissions.

        program_args must have a .verbose member.

    '''
    adb(['install', '-r', '-g', apk_info.apk_name], program_args)
    adb(['shell', 'pm', 'grant', apk_info.package_name,
        'android.permission.WRITE_EXTERNAL_STORAGE'], program_args)


def get_apk_info(apk):
    """Returns a named tuple (test_name, package_name, activity_name) for the
    given apk."""
    test_name = os.path.splitext(os.path.basename(apk))[0]
    package_name = 'com.example.test.' + test_name
    activity_name = 'android.app.NativeActivity'
    apk_info = collections.namedtuple(
        'ApkInfo', ['test_name', 'package_name', 'activity_name', 'apk_name'])
    return apk_info(test_name, package_name, activity_name, apk)


def watch_process(silent, program_args):
    ''' Watches the output of a running android process.

    Arguments:
        silent: True if output should be consumed, false otherwise
        program_args: The arguments passed to the program.

        program_args must contain a .verbose member.

    It is expected that the log was cleared before the process started.

    Returns the return code that it produced

    '''
    proc = adb_stream(
        ['logcat', '-s', '-v', 'brief', 'VulkanTestApplication:V'],
        program_args)
    return_value = 0

    while True:
        line = proc.stdout.readline()
        if line != '':
            if 'beginning of crash' in line:
                print '**Application Crashed**'
                proc.kill()
                return -1
            split_line = line.split(':')[1:]
            if not split_line:
                continue
            line_text = ':'.join(split_line)[1:]
            if split_line and split_line[0] == ' RETURN':
                if program_args.verbose:
                    print line_text,
                return_value = int(split_line[1])
                break
            if program_args.verbose or not silent:
                print line_text,
    proc.kill()
    return return_value
