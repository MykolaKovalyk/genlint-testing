# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(runner)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(EXTERNAL_BOARD native_sim/native/64)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_sources(app PRIVATE test/main.c)
