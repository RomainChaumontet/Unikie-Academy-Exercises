
load("//src/lib:variables.bzl", "COPTS")

cc_binary(
    name = "ipc_sendfile",
    srcs = ["main.cpp"],
    linkopts = ["-lrt", "-pthread"],
    copts = COPTS,
    deps = ["//src/lib:ipcCopyFileLib"],
)