# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

if (CONFIG_FTEST)
  zephyr_library()
  zephyr_include_directories(include)
  zephyr_include_directories(../common/include)
endif()
