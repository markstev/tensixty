#=============================================================================#
# Author: MarkStevens                                                         #
# Date:   2020/04/04                                                          #
#                                                                             #
# Description: Defines a cmake library for error-resilient serial             #
# communication.                                                              #
#                                                                             #
#=============================================================================#

cmake_minimum_required(VERSION 3.1)

# !!! POINT THESE TO YOUR OWN PATHS !!!
set(CMAKE_TOOLCHAIN_FILE /home/pi/arduinoio/cmake/ArduinoToolchain.cmake)
set(ARDUINO_SDK /usr/share/arduino-1.0.6)
set(CMAKE_SOURCE_DIR $(pwd)/..) # Source dir:

# Prevent CMake from setting -rdynamic on Linux (!!).
SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

# Protocol Buffers
set(CMAKE_MODULE_PATH /home/pi/armnyak/nanopb/extra)
find_package(Nanopb REQUIRED)
include_directories(${NANOPB_INCLUDE_DIRS})
nanopb_generate_cpp(PROTO_SRCS PROTO_HDRS motor_command.proto)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
set_source_files_properties(${PROTO_SRCS} ${PROTO_HDRS}
    PROPERTIES GENERATED TRUE)
message(${PROTO_HDRS})

set(tensixty_SRCS
  ${PROTO_SRCS}
  motor_bank_module.cc
  serial_module.cc
  motor.cc
  module_dispatcher.cc
  commlink.cc
  packet.cc
  real_arduino.cc)
set(tensixty_HDRS
  ${PROTO_HDRS}
  motor_bank_module.h
  serial_module.h
  motor.h
  module_dispatcher.h
  commlink.h
  packet.h
  arduino.h
  real_arduino.h
  serial_interface.h
  clock_interface.h)
set(tensixty_BOARD mega2560)

generate_arduino_library(tensixty)
