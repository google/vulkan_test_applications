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
'''This runs a given APK that was compiled from this repository.

This runs a given APK on the connected Android device. It makes assumptions
about log output and package names that means it is only useful
for this repository
'''

import argparse
import os
import subprocess
import sys

import android


def run_on_single_apk(apk, args):
    """Installs, runs and optionally uninstalls a single APK from an android device."""
    apk_info = android.get_apk_info(apk)
    android.install_apk(apk_info, args)
    android.adb(['logcat', '-c'], args)
    android.adb(['shell', 'am', 'start', '-n',
                 apk_info.package_name + '/' + apk_info.activity_name], args)

    return_value = android.watch_process(False, args)

    android.adb(['shell', 'am', 'force-stop', apk_info.package_name], args)
    if not args.keep:
        android.adb(['uninstall', apk_info.package_name], args)
    sys.exit(return_value)


def main():
    parser = argparse.ArgumentParser(
        description='Run a .apk file on an android device')
    parser.add_argument('apk', help='apk to run')
    parser.add_argument(
        '--keep', action='store_true', help='do not uninstall on completion')
    parser.add_argument(
        '--verbose', action='store_true', help='enable verbose output')
    args = parser.parse_args()
    run_on_single_apk(args.apk, args)


if __name__ == '__main__':
    main()
