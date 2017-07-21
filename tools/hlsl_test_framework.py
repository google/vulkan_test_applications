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
'''The main test framework for testing HLSL to SPIR-V flows.
'''

import argparse
import filecmp
import fnmatch
import inspect
import os
import platform
import re
import tempfile
import shutil
import subprocess
import sys
import traceback

SUCCESS = 0
FAILURE = 1

CAPTURED_FRAME = 100

WIN = platform.system() == 'Windows'


class HlslTest(object):
    '''
    Base class for all HLSL vs. GLSL tests
    '''

    def __init__(self, test_name, executable_path, run_dir, verbose,
                 shader_compiler_1, shader_compiler_2):
        self.exe = executable_path
        self.name = test_name
        self.run_dir = run_dir
        self.verbose = verbose
        self.shader_compiler_1 = shader_compiler_1
        self.shader_compiler_2 = shader_compiler_2

    def getImageSuffix(self, compiler):
        if compiler == 'glslc-glsl':
            return '.glslc.glsl.ppm'
        elif compiler == 'glslc-hlsl':
            return '.glslc.hlsl.ppm'
        elif compiler == 'dxc-hlsl':
            return '.dxc.hlsl.ppm'

    def getFirstImageSuffix(self):
        if self.shader_compiler_1 == self.shader_compiler_2:
            return '.first' + self.getImageSuffix(self.shader_compiler_1)
        else:
            return self.getImageSuffix(self.shader_compiler_1)

    def getSecondImageSuffix(self):
        if self.shader_compiler_1 == self.shader_compiler_2:
            return '.second' + self.getImageSuffix(self.shader_compiler_2)
        else:
            return self.getImageSuffix(self.shader_compiler_2)

    def run(self):
        '''Runs this test case. Returns (FAILURE|SUCCESS) on completion.'''
        print "[ " + "RUN".ljust(10) + " ] " + self.name

        # First Run
        first_ppm = self.name + self.getFirstImageSuffix()
        first_ppm_full_path = os.path.join(self.run_dir, first_ppm)
        args = []
        args.append(self.exe)
        args.append('-output-frame=' + str(CAPTURED_FRAME))
        args.append('-output-file=' + first_ppm_full_path)
        args.append('-shader-compiler=' + self.shader_compiler_1)
        args.append('-fixed')
        if self.verbose:
            subprocess.call(args)
        else:
            null_file = open(os.devnull, 'w')
            subprocess.call(args, stdout=null_file, stderr=null_file)

        # Second Run
        second_ppm = self.name + self.getSecondImageSuffix()
        second_ppm_full_path = os.path.join(self.run_dir, second_ppm)
        args = []
        args.append(self.exe)
        args.append('-output-frame=' + str(CAPTURED_FRAME))
        args.append('-output-file=' + second_ppm_full_path)
        args.append('-shader-compiler=' + self.shader_compiler_2)
        args.append('-fixed')
        if self.verbose:
            subprocess.call(args)
        else:
            null_file = open(os.devnull, 'w')
            subprocess.call(args, stdout=null_file, stderr=null_file)

        # First run failed :(
        if not os.path.isfile(first_ppm_full_path):
            print "File " + first_ppm + " not found."
            print "[ " + "FAILED".rjust(10) + " ] " + self.name
            return FAILURE
        elif self.verbose:
            print "File " + first_ppm + " has been generated."

        # Second run failed :(
        if not os.path.isfile(second_ppm_full_path):
            print "File " + second_ppm + " not found."
            print "[ " + "FAILED".rjust(10) + " ] " + self.name
            return FAILURE
        elif self.verbose:
            print "File " + second_ppm + " has been generated."

        # The two images do not match :(
        if not filecmp.cmp(first_ppm_full_path, second_ppm_full_path):
            print "Images captured from the two pipelines do not match."
            print "[ " + "FAILED".rjust(10) + " ] " + self.name
            return FAILURE
        elif self.verbose:
            print "Images match."

        # Everything was successful!
        print "[ " + "OK".rjust(10) + " ] " + self.name
        return SUCCESS


class TestManager(object):
    """Manages and runs all requested tests."""

    def __init__(self, apps_directory, bin_directory, app_name, verbose,
                 shader_compiler_1, shader_compiler_2):
        valid_shader_compilers = ['glslc-glsl', 'glslc-hlsl', 'dxc-hlsl']
        if shader_compiler_1 not in valid_shader_compilers:
            print "Invalid shader-compiler option:" + shader_compiler_1
            return FAILURE
        if shader_compiler_2 not in valid_shader_compilers:
            print "Invalid shader-compiler option:" + shader_compiler_2
            return FAILURE
        self.apps_directory = apps_directory
        self.bin_directory = bin_directory
        self.app_name = app_name
        if not app_name:
            self.run_all = True
        else:
            self.run_all = False
        self.verbose = verbose
        self.shader_compiler_1 = shader_compiler_1
        self.shader_compiler_2 = shader_compiler_2
        self.test_paths = []
        self.tests = {}
        self.number_of_tests_run = 0
        self.failed_tests = []

    def getExecutableName(self, app_name):
        if WIN:
            return app_name + ".exe"
        return app_name

    def hasHLSLShaders(self, app_name):
        files = os.listdir(os.path.join(self.apps_directory, app_name))
        for file in files:
            if '.hlsl.' in file:
                return True
        return False

    def executableExists(self, app_name):
        exe_name = self.getExecutableName(app_name)
        return os.path.isfile(os.path.join(self.bin_directory, exe_name))

    def gather_all_tests(self):
        '''Populates the list of tests to be run'''
        app_names = [f for f in os.listdir(
            self.apps_directory) if '.' not in f]
        for app_name in app_names:
            if self.executableExists(app_name) and self.hasHLSLShaders(app_name):
                print app_name
                if self.run_all or app_name == self.app_name:
                    self.tests[app_name] = os.path.join(
                        self.bin_directory, app_name)
        if not self.tests:
            if self.run_all:
                print "Error: found no applications in " + self.bin_directory
                return FAILURE
            else:
                print "Error: Application named " + self.app_name + \
                    " was not found in " + self.bin_directory
                return FAILURE
        # Everything was successful
        return SUCCESS

    def print_test_names(self, file_handle):
        """Prints out to the list of all tests."""
        for test in self.tests.keys():
            print test

    def run_all_tests(self, temp_directory):
        """Runs all of the tests"""
        for test_name, test_exe in self.tests.items():
            self.accumulate_result(
                test_name,
                HlslTest(test_name, test_exe, temp_directory, self.verbose,
                         self.shader_compiler_1, self.shader_compiler_2).run())

    def accumulate_result(self, test_name, result):
        """Tracks the results for the given test"""
        self.number_of_tests_run += 1
        if result == FAILURE:
            self.failed_tests.append(test_name)

    def print_summary_and_return_code(self):
        '''Prints a summary of the test results. Returns 0 if every test
        was successful and -1 if any test failed.
        Warnings do not count as failure, but are printed out in the summary.'''
        print "===================================================="
        print "Total Tests Run: " + str(self.number_of_tests_run)
        print("Total Tests Passed: " +
              str(self.number_of_tests_run - len(self.failed_tests)))
        for failure in self.failed_tests:
            print "[ " + "FAILED".rjust(10) + " ] " + failure
        if self.failed_tests:
            return FAILURE
        return SUCCESS


def main():
    """Entry-point for the HLSL to SPIR-V testing framework"""
    parser = argparse.ArgumentParser(description='''
This framework executes tests on all applications (or the selected application)
from 'application_sandbox'.

For each test, the Vulkan graphics pipeline is executed twice using 2 sets of
shaders.

A specifc frame is chosen from the two executions, and an image (.ppm) is
written to disk for each execution. The two images are then comapred. If the
two images match, the test is considered a PASS.

The shaders fed to each execution can be chosen via the --shader-compiler-1 and
--shader-compiler-2 options.
        ''')
    parser.add_argument(
        "--app-dir", nargs=1, help="Directory containing sample applications")
    parser.add_argument(
        "--app-name", nargs=1, help="Name of the application to test")
    parser.add_argument(
        "--bin-dir",
        nargs=1,
        help="Directory containing application executables")
    parser.add_argument(
        "--shader-compiler-1",
        nargs=1,
        help="The first set of shaders that are used in the pipeline.\
            Valid options are: glslc-glsl, glslc-hlsl, dxc-hlsl")
    parser.add_argument(
        "--shader-compiler-2",
        nargs=1,
        help="The second set of shaders that are used in the pipeline.\
            Valid options are: glslc-glsl, glslc-hlsl, dxc-hlsl")
    parser.add_argument(
        "--list-tests",
        action="store_true",
        help="Only print the names of all tests")
    parser.add_argument(
        "--verbose", action="store_true", help="Show verbose output")
    args = parser.parse_args()

    if not args.app_dir:
        print "error: --app-dir option is required. this option should point \
to the directory containing sample applications."

        return -1
    if not args.bin_dir:
        print "error: --bin-dir option is required. this option should point \
to the directory containing application executables."

        return -1
    if not args.shader_compiler_1:
        print "error: --shader-compiler-1 option is required. this option \
specifies the source of the first set of shaders that are used. \
Valid options are: glslc-glsl, glslc-hlsl, dxc-hlsl."

        return -1
    if not args.shader_compiler_2:
        print "error: --shader-compiler-2 option is required. this option \
specifies the source of the second set of shaders that are used. \
Valid options are: glslc-glsl, glslc-hlsl, dxc-hlsl."

        return -1
    if args.app_name:
        app_name = args.app_name[0]
    else:
        app_name = ""

    manager = TestManager(args.app_dir[0], args.bin_dir[0], app_name,
                          args.verbose, args.shader_compiler_1[0],
                          args.shader_compiler_2[0])
    if manager.gather_all_tests():
        return -1

    if args.list_tests:
        manager.print_test_names(sys.stdout)
        return 0
    else:
        test_directory = tempfile.mkdtemp()
        manager.run_all_tests(test_directory)
        shutil.rmtree(test_directory)
        return manager.print_summary_and_return_code()


if __name__ == "__main__":
    sys.exit(main())
