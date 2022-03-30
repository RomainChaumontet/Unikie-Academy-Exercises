
load("//src/lib:variables.bzl", "COPTS")

cc_binary(
    name = "ipc_receivefile",
    srcs = ["main.cpp"],
    linkopts = ["-lrt", "-pthread"],
    copts = COPTS,
    deps = ["//src/lib:ipcCopyFileLib"],
)