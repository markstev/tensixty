package(default_visibility = ["//visibility:public"])

cc_library(name = "packet",
           srcs = ["packet.cc"],
           hdrs = ["packet.h"],
)

cc_test(name = "packet_test",
        srcs = ["packet_test.cc"],
        deps = [":packet",
                "@google_googletest//:gtest",
                "@google_googletest//:gtest_main"
        ])

cc_library(name = "commlink",
           srcs = ["commlink.cc"],
           hdrs = ["commlink.h"],
           deps = [
               ":packet",
               ":arduino_simulator",
           ],
)

cc_test(name = "commlink_test",
        srcs = ["commlink_test.cc"],
        deps = [":commlink",
                ":arduino_simulator",
                "@google_googletest//:gtest",
                "@google_googletest//:gtest_main"
        ])

cc_library(name = "arduino_simulator",
           srcs = ["arduino_simulator.cc"],
           hdrs = [
               "arduino.h",
               "arduino_simulator.h",
               "serial_interface.h",
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
            ":commlink",
            ":arduino_simulator",
            "@google_googletest//:gtest",
            "@google_googletest//:gtest_main",
        ])
