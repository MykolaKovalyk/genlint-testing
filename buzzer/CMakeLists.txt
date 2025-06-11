# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/../_modules/entity_lib)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(buzzer)

target_include_directories(app PRIVATE include)

target_sources(app PRIVATE 
    src/main.c
    src/pattern.c
    src/patterns.c
    src/command_processor.c
)

