
load("//src/lib:variables.bzl", "COPTS")

cc_binary(
    name = "ipc_sendfile",
    srcs = ["ipc_sendfile.cpp"],
    linkopts = ["-lrt", "-pthread"],
    copts = COPTS,
    deps = ["//src/lib:ipcCopyFileLib"],
)

cc_binary(
    name = "ipc_receivefile",
    srcs = ["ipc_receivefile.cpp"],
    linkopts = ["-lrt", "-pthread"],
    copts = COPTS,
    deps = ["//src/lib:ipcCopyFileLib"],
)