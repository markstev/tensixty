package(default_visibility = ["//visibility:public"])

py_library(
    name = "hardware_abstraction",
    srcs = ["hardware_abstraction.py"],
)

py_test(
    name = "hardware_abstraction_test",
    srcs = ["hardware_abstraction_test.py"],
    deps = [":hardware_abstraction"],
    python_version = "PY3",
)

py_library(
    name = "file_relay",
    srcs = ["file_relay.py"],
)

py_test(
    name = "tensixty_fault_test",
    srcs = ["tensixty_fault_test.py"],
    deps = [
        ":file_relay",
        ":tensixty",
    ],
    python_version = "PY3",
)

py_library(
    name = "tensixty",
    srcs = ["tensixty.py"],
    data = [
        ":commlink_main",
    ],
)

py_test(
    name = "tensixty_test",
    srcs = ["tensixty_test.py"],
    deps = [
        ":tensixty",
    ],
    python_version = "PY3",
)

cc_binary(
    name = "commlink_main",
    srcs = ["commlink_main.cc"],
    deps = [
        "//cc:commlink",
        "//tests:arduino_simulator",
    ],
)
