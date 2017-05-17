# GAPID Tests

This is a set of test applications, and associated python files. The purpose
of these tests is to validate that the correct information is being
recorded during a trace using [GAPID](https://github.com/google/gapid).

Each test contains a set of `.cpp` files. These consitute the actual
application. Each test also contains one or more associated python files.
The python files define expectations for the contents of the traces.

The test flow from `tools/gapit_test_framework.py` is as follows:

1. Each apk under test will be installed on an Android device.
2. For each test in the associated python file, the trace will be parsed,
and expectations checked.
3. If any expectations are not met, the test will fail.
4. The next test will be run.

These tests are explicitly to make sure that a trace file contains the
data that is required. They do not however make sure that replay
functions.