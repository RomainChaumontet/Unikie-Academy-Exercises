cc_test(
    name = "Gtest_ipc",
    size = "medium",
    timeout = "short",
    srcs = glob(["**/*.cpp", "**/*.h"]),
    deps = [
        "//src/lib:ipcCopyFileLib",
        "@com_google_googletest//:gtest_main",
    ]
)