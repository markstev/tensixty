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

# Prevent CMake from setting -rdynamic on Linux (!!).
SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

set(tensixty_SRCS
  commlink.cc
  packet.cc
  real_arduino.cc)
set(tensixty_HDRS
  commlink.h
  packet.h
  arduino.h
  real_arduino.h
  serial_interface.h
  clock_interface.h)
set(tensixty_BOARD mega2560)

generate_arduino_library(tensixty)
