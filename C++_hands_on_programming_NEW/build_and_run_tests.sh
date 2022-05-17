#!/usr/bin/sh
set -x
bazel clean --expunge_async
bazel build //src/bin:ipc_receivefile 
bazel build //src/bin:ipc_sendfile 

bazel test //gtest:Test_All 
rm -r -f output


mkdir output

cp bazel-bin/src/bin/ipc_sendfile output
cp bazel-bin/src/bin/ipc_receivefile output
cp bazel-testlogs/gtest/Test_All/test.log output

set +x