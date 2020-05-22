package(default_visibility = ["//visibility:public"])

# Testing libraries
cc_library(name = "arduino_simulator",
           srcs = ["arduino_simulator.cc"],
           hdrs = ["arduino_simulator.h"],
           deps = ["//cc:interfaces"])

cc_binary(name = "serial_main_for_test",
          srcs = ["serial_main_for_test.cc"],
          deps = [
              ":arduino_simulator",
              "//cc:commlink"
          ])

# Tests
cc_test(name = "packet_test",
        srcs = ["packet_test.cc"],
        deps = ["//cc:packet",
                "@google_googletest//:gtest",
                "@google_googletest//:gtest_main"
        ])

cc_test(name = "commlink_test",
        srcs = ["commlink_test.cc"],
        deps = ["//cc:commlink",
                ":arduino_simulator",
                "@google_googletest//:gtest",
                "@google_googletest//:gtest_main"
        ])

cc_test(name = "arduino_simulator_test",
        srcs = ["arduino_simulator_test.cc"],
        deps = [":arduino_simulator",
                "@google_googletest//:gtest",
                "@google_googletest//:gtest_main"
        ])

cc_test(name = "commlink_pair_test",
        srcs = ["commlink_pair_test.cc"],
        deps = [
            ":arduino_simulator",
            "//cc:commlink",
            "@google_googletest//:gtest",
            "@google_googletest//:gtest_main",
        ])

cc_test(name = "module_dispatcher_test",
        srcs = ["module_dispatcher_test.cc"],
        deps = [
            "//cc:module_dispatcher",
            "@google_googletest//:gtest",
            "@google_googletest//:gtest_main",
        ])

cc_test(name = "motor_test",
        srcs = ["motor_test.cc"],
        deps = [
            ":arduino_simulator",
            "//cc:motor",
            "@google_googletest//:gtest",
            "@google_googletest//:gtest_main",
        ])

cc_test(name = "motor_bank_module_test",
        srcs = ["motor_bank_module_test.cc"],
        deps = [
            ":arduino_simulator",
            "//cc:module_dispatcher",
            "//cc:motor_bank_module",
            "@google_googletest//:gtest",
            "@google_googletest//:gtest_main",
        ])

cc_test(name = "read_module_test",
        srcs = ["read_module_test.cc"],
        deps = [
            ":arduino_simulator",
            "//cc:module_dispatcher",
            "//cc:read_module",
            "@google_googletest//:gtest",
            "@google_googletest//:gtest_main",
        ])
