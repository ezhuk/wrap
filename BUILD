load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "wrap",
    srcs = [
        "src/app.cpp",
        "src/wrap.cpp",
    ],
    hdrs = glob([
        "include/**/*.h",
    ]),
    includes = [
        "include",
    ],
    defines = [
        'WRAP_VERSION="0.1.0"',
    ],
    deps = [
        "@fmt//:fmt",
        "@folly//folly:json",
        "@proxygen//:httpserver",
    ],
)
