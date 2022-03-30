
load("//src/lib:variables.bzl", "COPTS")

cc_test(
    name = "Gtest_ipc",
    size = "medium",
    timeout = "short",
    srcs = glob(["**/*.cpp", "**/*.h"]),
    deps = [
        "//src/lib:ipcCopyFileLib",
        "@com_google_googletest//:gtest_main",
    ],
    linkopts = ["-lrt", "-pthread"],
    copts = COPTS,
    visibility = ["//visibility:public"],
)


cc_test(
    name="Gtest_shm",
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

        