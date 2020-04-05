package(default_visibility = ["//visibility:public"])

cc_library(name = "packet",
           srcs = ["packet.cc"],
           hdrs = ["packet.h"],
)

cc_library(name = "commlink",
           srcs = ["commlink.cc"],
           hdrs = ["commlink.h"],
           deps = [
               ":packet",
               ":arduino_simulator",
           ],
)

cc_library(name = "interfaces",
           hdrs = [
               "arduino.h",
               "clock_interface.h",
               "serial_interface.h",
           ])
