package(default_visibility = ["//visibility:public"])

py_library(
    name = "hardware_abstraction",
    srcs = ["hardware_abstraction.py"],
)

py_library(
    name = "commlink",
    srcs = ["commlink.py"],
    deps = [
        ":hardware_abstraction",
    ],
)

py_test(
    name = "hardware_abstraction_test",
    srcs = ["hardware_abstraction_test.py"],
    deps = [":hardware_abstraction"],
    python_version = "PY3",
)

py_test(
    name = "packet_test",
    srcs = ["packet_test.py"],
    deps = [":commlink",
            ":hardware_abstraction"],
    python_version = "PY3",
)

py_test(
    name = "commlink_writer_test",
    srcs = ["commlink_writer_test.py"],
    deps = [":commlink",
            ":hardware_abstraction"],
    python_version = "PY3",
)

py_test(
    name = "commlink_reader_test",
    srcs = ["commlink_reader_test.py"],
    deps = [":commlink",
            ":hardware_abstraction"],
    python_version = "PY3",
)

py_test(
    name = "commlink_test",
    srcs = ["commlink_test.py"],
    deps = [":commlink",
            ":hardware_abstraction"],
    timeout = "short",
    python_version = "PY3",
)

py_test(
    name = "commlink_fault_test",
    srcs = ["commlink_fault_test.py"],
    deps = [":commlink",
            ":hardware_abstraction"],
    timeout = "eternal",
    python_version = "PY3",
)

py_test(
    name = "commlink_profile",
    srcs = ["commlink_profile.py"],
    deps = [":commlink",
            ":hardware_abstraction"],
    python_version = "PY3",
)

py_test(
    name = "cc_python_integration_test",
    srcs = ["cc_python_integration_test.py"],
    deps = [
        ":commlink",
        ":hardware_abstraction",
    ],
    data = [
        "//tests:serial_main_for_test",
    ],
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
