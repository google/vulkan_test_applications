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
"""This turns a .d file into a cmake dependency file.

It takes a single file .d file as input, and generates a .d.cmake.tmp
file as output with the same basename.
"""

import sys


def main():

    with open(sys.argv[1]) as content_file:
        content = str(content_file.read())
        content = content.split(":", 1)[1]
        content = content.replace("\n", " ")
        files = content.split(" ")
        target_file_contents = "set(FILE_DEPS\n"
        for f in files:
            target_file_contents += f + "\n"
        target_file_contents += ")"

        with open(sys.argv[1] + ".cmake.tmp", "w+") as write_file:
            write_file.write(target_file_contents)


if __name__ == "__main__":
    sys.exit(main())
