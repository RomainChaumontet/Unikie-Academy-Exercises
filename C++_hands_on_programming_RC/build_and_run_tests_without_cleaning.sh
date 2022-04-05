#!/usr/bin/sh
#bazel clean

bazel build //src/ipc_receivefile:ipc_receivefile 
bazel build //src/ipc_sendfile:ipc_sendfile 

bazel test //gtest:Gtest_ipc
rm -r -f output


mkdir output

cp bazel-bin/src/ipc_sendfile/ipc_sendfile output
cp bazel-bin/src/ipc_receivefile/ipc_receivefile output
cp bazel-testlogs/gtest/Gtest_ipc/test.log output
