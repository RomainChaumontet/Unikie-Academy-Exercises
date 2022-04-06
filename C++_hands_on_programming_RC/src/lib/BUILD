
load("//src/lib:variables.bzl", "COPTS")

cc_library(
    name = "ipcCopyFileLib",
    srcs = glob(["**/*.cpp"]),
    hdrs = glob(["**/*.h"]),
    linkopts = ["-lrt", "-pthread"],
    copts = COPTS,
    visibility = ["//visibility:public"],
)