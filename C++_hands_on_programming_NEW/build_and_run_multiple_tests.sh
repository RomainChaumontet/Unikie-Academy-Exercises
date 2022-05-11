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


bazel clean --expunge_async
bazel test //gtest:Test_All
cp bazel-testlogs/gtest/Test_All/test.log output/test1.log
bazel clean --expunge_async
bazel test //gtest:Test_All
cp bazel-testlogs/gtest/Test_All/test.log output/test2.log
bazel clean --expunge_async
bazel test //gtest:Test_All
cp bazel-testlogs/gtest/Test_All/test.log output/test3.log
bazel clean --expunge_async
bazel test //gtest:Test_All
cp bazel-testlogs/gtest/Test_All/test.log output/test4.log
bazel clean --expunge_async
bazel test //gtest:Test_All
cp bazel-testlogs/gtest/Test_All/test.log output/test5.log
bazel clean --expunge_async
bazel test //gtest:Test_All
cp bazel-testlogs/gtest/Test_All/test.log output/test6.log
bazel clean --expunge_async
bazel test //gtest:Test_All
cp bazel-testlogs/gtest/Test_All/test.log output/test7.log
bazel clean --expunge_async
bazel test //gtest:Test_All
cp bazel-testlogs/gtest/Test_All/test.log output/test8.log
bazel clean --expunge_async
bazel test //gtest:Test_All
cp bazel-testlogs/gtest/Test_All/test.log output/test9.log
bazel clean --expunge_async
set +x