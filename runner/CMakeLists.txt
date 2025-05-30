# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/../_modules/entity_lib)
list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/../_modules/runner_lib)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(runner)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

ExternalProject_Add(
    ftest_master
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../master
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/ftest_master
    CONFIGURE_COMMAND ""
    BUILD_COMMAND west build -b ${CONFIG_BOARD_TARGET} --build-dir <BINARY_DIR> <SOURCE_DIR> -DCONFIG_FTEST_ENTITY=y
    INSTALL_COMMAND ""
    BUILD_ALWAYS TRUE
)

ExternalProject_Add(
    ftest_buzzer
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../buzzer
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/ftest_buzzer
    CONFIGURE_COMMAND ""
    BUILD_COMMAND west build -b ${CONFIG_BOARD_TARGET} --build-dir <BINARY_DIR> <SOURCE_DIR> -DCONFIG_FTEST_ENTITY=y
    INSTALL_COMMAND ""
    BUILD_ALWAYS TRUE
)

ExternalProject_Add(
    ftest_button
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../button
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/ftest_button
    CONFIGURE_COMMAND ""
    BUILD_COMMAND west build -b ${CONFIG_BOARD_TARGET} --build-dir <BINARY_DIR> <SOURCE_DIR> -DCONFIG_FTEST_ENTITY=y
    INSTALL_COMMAND ""
    BUILD_ALWAYS TRUE
)

ExternalProject_Add(
    ftest_potentiometer
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../potentiometer
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/ftest_potentiometer
    CONFIGURE_COMMAND ""
    BUILD_COMMAND west build -b ${CONFIG_BOARD_TARGET} --build-dir <BINARY_DIR> <SOURCE_DIR> -DCONFIG_FTEST_ENTITY=y
    INSTALL_COMMAND ""
    BUILD_ALWAYS TRUE
)


add_dependencies(app ftest_master ftest_buzzer ftest_button ftest_potentiometer)


target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_sources(app PRIVATE test/main.c)

