package(default_visibility = ["//visibility:public"])

cc_library(name = "commlink",
           srcs = ["commlink.cc"],
           hdrs = ["commlink.h"],
)

cc_test(name = "commlink_test",
        srcs = ["commlink_test.cc"],
        deps = [":commlink",
                "@google_googletest//:gtest",
                "@google_googletest//:gtest_main"
        ])
