#!/usr/bin/sh


bazel build --cxxopt='-std=c++14' //src/ipc_receivefile:ipc_receivefile
bazel build --cxxopt='-std=c++14' //src/ipc_sendfile:ipc_sendfile

bazel test --cxxopt='-std=c++14' //gtest:Gtest_ipc
rm -r -f output


mkdir output

cp bazel-bin/src/ipc_sendfile/ipc_sendfile output
cp bazel-bin/src/ipc_receivefile/ipc_receivefile output
cp bazel-testlogs/gtest/Gtest_ipc/test.log output

bazel clean
gedit output/test.log
