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
'''This runs a single apk through gapit trace and gapit dump. Passes the exit
code of the apk code through to its own exit code.'''

import argparse
import os
import subprocess
import sys
import android


class RunArgs(object):

    """Arguments to run_on_single_apk"""

    def __init__(self):
        self.verbose = False
        self.output = None
        self.keep = False

    def set_verbose(self):
        """Sets the verbose flag to true"""
        self.verbose = True

    def set_output(self, output):
        """Sets the output to the given output"""
        self.output = output

    def set_keep(self):
        """Sets the keep flag to true"""
        self.keep = True

    verbose = False
    output = None
    keep = False


def run_on_single_apk(apk, args):
    '''Installs, traces and optionally uninstalls a single APK from an android
    device. args must conform to the RunArgs interface. Returns the exit code
    of the running apk.'''
    apk_info = android.get_apk_info(apk)
    android.install_apk(apk_info, args)
    android.adb(['logcat', '-c'], args)

    gapit_args = ['gapit']

    if args.verbose:
        gapit_args.extend(['-log-level', 'Debug'])

    gapit_args.extend(['trace'])
    if args.output:
        gapit_args.extend(['-out', args.output[0]])
    gapit_args.extend([apk_info.package_name])

    if args.verbose:
        print gapit_args

    if args.verbose:
        subprocess.Popen(gapit_args)
    else:
        null_file = open(os.devnull, 'w')
        subprocess.Popen(gapit_args, stdout=null_file, stderr=null_file)

    return_value = android.watch_process(True, args)
    android.adb(['shell', 'am', 'force-stop', apk_info.package_name], args)
    if not args.keep:
        android.adb(['uninstall', apk_info.package_name], args)

    return return_value


def main():
    """Main entry point to the APK runner"""
    parser = argparse.ArgumentParser(
        description='Run a .apk file on an android device')
    parser.add_argument('apk', help='apk to run')
    parser.add_argument(
        '--keep', action='store_true', help='do not uninstall on completion')
    parser.add_argument(
        '--verbose', action='store_true', help='enable verbose output')
    parser.add_argument(
        '--output',
        nargs=1,
        help='If specified, then the trace will be written here')
    args = parser.parse_args()
    return run_on_single_apk(args.apk, args)


if __name__ == '__main__':
    sys.exit(main())
