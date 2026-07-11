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
    defines = [
        'WRAP_VERSION="0.1.0"',
    ],
    includes = [
        "include",
    ],
    deps = [
        "@fmt",
        "@folly//folly:json",
        "@proxygen//proxygen:httpserver",
    ],
)
