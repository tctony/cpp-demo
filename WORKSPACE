load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "rules_foreign_cc",
    strip_prefix = "rules_foreign_cc-master",
    url = "https://github.com/bazelbuild/rules_foreign_cc/archive/master.zip",
)

load("@rules_foreign_cc//:workspace_definitions.bzl", "rules_foreign_cc_dependencies")

# for projects that compile with configure&make
rules_foreign_cc_dependencies()

# protobuf
http_archive(
    name = "rules_proto",
    sha256 = "6117a0f96af1d264747ea3f3f29b7b176831ed8acfd428e04f17c48534c83147",
    strip_prefix = "rules_proto-8b81c3ccfdd0e915e46ffa888d3cdb6116db6fa5",
    # commit date 2020-04-01
    urls = [
        "https://github.com/bazelbuild/rules_proto/archive/8b81c3ccfdd0e915e46ffa888d3cdb6116db6fa5.tar.gz",
    ],
)

load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")

rules_proto_dependencies()

rules_proto_toolchains()

local_repository(
    name = "com_github_grpc_grpc",  # 1.28.1
    path = "../lib/grpc",
)

load("@com_github_grpc_grpc//bazel:grpc_deps.bzl", "grpc_deps")

grpc_deps()

load("@com_github_grpc_grpc//bazel:grpc_extra_deps.bzl", "grpc_extra_deps")

grpc_extra_deps()

local_repository(
    name = "gtest_local",  # 1.10
    path = "../lib/googletest",
)

bind(
    name = "gtest_main",
    actual = "@gtest_local//:gtest_main",
)

local_repository(
    name = "abseil",  # 20200225.1
    path = "../lib/abseil-cpp",
)

local_repository(
    name = "asio",  # 1.16
    path = "../lib/asio",
)

local_repository(
    name = "ThreadPool",
    path = "../lib/ThreadPool",
)

local_repository(
    name = "base",
    path = "../base",
)
