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
           ],
)

cc_test(name = "commlink_test",
        srcs = ["commlink_test.cc"],
        deps = [":commlink",
                "@google_googletest//:gtest",
                "@google_googletest//:gtest_main"
        ])
