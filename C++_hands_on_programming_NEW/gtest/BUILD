
load("//src/lib:variables.bzl", "COPTS")

cc_test(
    name = "Test_All",
    size = "medium",
    timeout = "short",
    srcs = glob(["**/*.cpp", "**/*.h"]),
    deps = [
        "//src/lib:ipcCopyFileLib",
        "@com_google_googletest//:gtest_main",
    ],
    linkopts = ["-lrt", "-pthread"],
    copts = COPTS,
    visibility = ["//visibility:public"]
)


cc_test(
    name="Test_Shm",
    srcs = ["Gtest_Ipc.cpp",
        "Gtest_Ipc.h",
        "Gtest_IpcShm.cpp"],
    deps = [
        "//src/lib:ipcCopyFileLib",
        "@com_google_googletest//:gtest_main",
    ],
    linkopts = ["-lrt", "-pthread"],
    copts = COPTS,
    visibility = ["//visibility:public"],
)

cc_test(
    name="Test_Pipe",
    srcs = ["Gtest_Ipc.cpp",
        "Gtest_Ipc.h",
        "Gtest_IpcPipe.cpp"],
    deps = [
        "//src/lib:ipcCopyFileLib",
        "@com_google_googletest//:gtest_main",
    ],
    linkopts = ["-lrt", "-pthread"],
    copts = COPTS,
    visibility = ["//visibility:public"],
)

cc_test(
    name="Test_Queue",
    srcs = ["Gtest_Ipc.cpp",
        "Gtest_Ipc.h",
        "Gtest_IpcPipe.cpp"],
    deps = [
        "//src/lib:ipcCopyFileLib",
        "@com_google_googletest//:gtest_main",
    ],
    linkopts = ["-lrt", "-pthread"],
    copts = COPTS,
    visibility = ["//visibility:public"],
)
        