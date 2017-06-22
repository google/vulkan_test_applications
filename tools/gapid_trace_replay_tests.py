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
'''The main test framework for testing trace/replay
'''

import argparse
import os
import re
import tempfile
import shutil
import subprocess
import sys

from gapit_tester import run_on_single_apk
from gapit_tester import RunArgs

SUCCESS = 0
FAILURE = 1
WARNING = 2
SKIPPED = 3


class Test(object):

    def __init__(self, executable, mid_execution):
        self.executable = executable
        self.mid_execution = mid_execution


class TestManager(object):

    """Manages and runs all requested tests."""

    def __init__(self, root_directory, verbose, host):
        """root_directory is the directory that contains all of the tests"""
        self.root_directory = root_directory
        self.verbose = verbose
        self.host = host
        # tests is a dictionary of test-name to Test objects
        self.tests = {}
        self.number_of_tests_run = 0
        self.failed_tests = []
        self.warned_tests = []
        self.skipped_tests = []

    def gather_all_tests(self, include_regex, exclude_regex):
        '''Finds all tests in the samples.txt file.'''
        include_matcher = re.compile(include_regex)
        exclude_matcher = re.compile(exclude_regex)

        with open(os.path.join(self.root_directory, "samples.txt"), 'r') as f:
            for test_name in f:
                if (include_matcher.match(test_name) and
                        exclude_matcher.match(test_name) is None):
                    self.add_test(test_name.strip(), test_name.strip(), False)
                mec_test_name = test_name.strip() + ".mid_execution"
                if (include_matcher.match(mec_test_name) and
                        exclude_matcher.match(mec_test_name) is None):
                    self.add_test(
                        mec_test_name, test_name.strip(), True)

    def print_test_names(self, file_handle):
        """Prints out to the given file handle a list of all tests."""
        for test in sorted(self.tests.keys()):
            file_handle.write(test + "\n")

    def add_test(self, test_name, test_executable, mid_execution):
        """Adds a test case to the manager from the given apk"""
        self.tests[test_name] = Test(test_executable, mid_execution)

    def run_apk(self, verbose, test_name, apk_name, capture_name, mid_execution):
        args = RunArgs()
        additional_args = ["-observe-frames", "1", "-capture-frames", "10"]
        if mid_execution:
            additional_args.extend(["-start-at-frame", "100"])

        if verbose:
            args.set_verbose()
        args.set_output(capture_name)
        setattr(args, "verbose", verbose)
        setattr(args, "keep", False)
        setattr(args, "output", [capture_name])
        setattr(args, "additional_params", additional_args)
        print "[ " + "TRACING".center(10) + " ] " + test_name
        try:
            return_value = run_on_single_apk(apk_name, args)
        except:
            return (FAILURE, "Could not generate trace file.")
        if return_value != 0:
            return (FAILURE, "Could not generate trace file.")
        if verbose:
            print "Successfully traced application"
        print "[ " + "DONE".center(10) + " ] " + test_name
        return (SUCCESS, "")

    def run_host(self, verbose, test_name, host_name, capture_name, mid_execution):
        print "[ " + "TRACING".center(10) + " ] " + test_name
        try:
            gapit_args = ['gapit']

            if verbose:
                gapit_args.extend(['-log-level', 'Debug'])
            gapit_args.extend(['trace', '-out',
                               capture_name, '-local-app', host_name])
            # On Ubuntu 14.04 with Nvidia K2200, driver: 375.66, an window of
            # size 1000 x 1000 may hang the device with sample:
            # 'copy_querypool_results' and 'wireframe'. However the same size
            # window does not hang when run on Ubuntu 14.04 with Nvidia driver:
            # 375.39. This might be a driver bug.
            # TODO: Test with 1000 x 1000 on latest Nvidia driver again.
            gapit_args.extend(['-local-args', '-w=1024 -h=1024'])
            gapit_args.extend([
                "-observe-frames", "1", "-capture-frames", "10"])
            if mid_execution:
                gapit_args.extend(["-start-at-frame", "100"])
            if verbose:
                print gapit_args
                subprocess.check_call(gapit_args)
            else:
                subprocess.check_call(
                    gapit_args, stdout=open(os.devnull, 'wb'),
                    stderr=open(os.devnull, 'wb'))
        except subprocess.CalledProcessError:
            return (FAILURE, "Could not generate trace file.")
        if verbose:
            print "Successfully traced application"
        print "[ " + "DONE".center(10) + " ] " + test_name
        return (SUCCESS, "")

    def run_test(self, test_name, verbose, host, temp_directory):
        capture_name = os.path.join(
            temp_directory, test_name + ".gfxtrace")
        if host:
            success, error = self.run_host(verbose, test_name, os.path.join(
                self.root_directory, self.tests[
                    test_name].executable), capture_name,
                self.tests[test_name].mid_execution)
        else:
            success, error = self.run_apk(verbose, test_name, os.path.join(
                self.root_directory, self.tests[
                    test_name].executable + ".apk"), capture_name,
                self.tests[test_name].mid_execution)
        if not success == SUCCESS:
            return (success, error)
        print "[ " + "RUN".ljust(10) + " ] " + test_name

        video_flags = ["gapit", "video", "-out",
                       os.path.join(temp_directory, test_name + ".mp4"),
                       capture_name]
        if self.verbose:
            print video_flags
        if verbose:
            proc = subprocess.Popen(
                video_flags, stderr=subprocess.PIPE)
        else:
            proc = subprocess.Popen(
                video_flags, stdout=open(os.devnull, 'wb'),
                stderr=subprocess.PIPE)
        _, stderr = proc.communicate()
        err = proc.wait()
        if err:
            print "[ " + "FAILED".rjust(10) + " ] " + test_name
            error_msg = "See {}\n{}".format(os.path.join(temp_directory,
                                                         test_name + ".mp4"),
                                            stderr)

            print error_msg
            return (FAILURE, error_msg)
        print "[ " + "OK".rjust(10) + " ] " + test_name
        return SUCCESS, ""

    def run_all_tests(self, temp_directory):
        """Runs all of the tests"""
        for test in sorted(self.tests):
            self.accumulate_result(
                test, self.run_test(test, self.verbose, self.host,
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
        if self.failed_tests:
            return -1
        return 0


def main():
    """Entry-point for the testing framework"""
    parser = argparse.ArgumentParser(description='''
This testing framework will look in a directory for a samples.txt and
associated binaries. It will build its tests from there.
        ''')
    parser.add_argument(
        "--test-dir", nargs=1, help="Directory that contains the executables/apks")
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
    parser.add_argument(
        "--keep",
        action="store_true",
        help="Do not delete output files")
    args = parser.parse_args()

    test_directory = os.getcwd()
    if args.test_dir:
        test_directory = args.test_dir[0]
    manager = TestManager(
        test_directory, args.verbose, args.host)
    manager.gather_all_tests(args.include, args.exclude)

    if args.list_tests:
        manager.print_test_names(sys.stdout)
        return 0
    else:
        temp_directory = tempfile.mkdtemp("", "GAPID_VIDEO-")
        print "Running tests in {}".format(temp_directory)
        manager.run_all_tests(temp_directory)
        if not args.keep:
            shutil.rmtree(temp_directory)
        return manager.print_summary_and_return_code()


if __name__ == "__main__":
    sys.exit(main())
