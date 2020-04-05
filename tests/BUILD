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
            "//cc:commlink",
            ":arduino_simulator",
            "@google_googletest//:gtest",
            "@google_googletest//:gtest_main",
        ])
