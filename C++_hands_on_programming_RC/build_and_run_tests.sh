#!/usr/bin/sh
set -x

bazel build --cxxopt='-std=c++14' //src/ipc_receivefile:ipc_receivefile --linkopt="-lrt"
bazel build --cxxopt='-std=c++14' //src/ipc_sendfile:ipc_sendfile --linkopt="-lrt"

bazel test --cxxopt='-std=c++14' //gtest:Gtest_ipc --linkopt="-lrt"
rm -r -f output


mkdir output

cp bazel-bin/src/ipc_sendfile/ipc_sendfile output
cp bazel-bin/src/ipc_receivefile/ipc_receivefile output
cp bazel-testlogs/gtest/Gtest_ipc/test.log output

bazel clean
set +x