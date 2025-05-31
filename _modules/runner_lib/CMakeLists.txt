# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

if (CONFIG_FTEST)
  zephyr_library()
  zephyr_include_directories(include)
  zephyr_include_directories(../common/include)

  target_compile_options(native_simulator INTERFACE 
    -I${CMAKE_CURRENT_SOURCE_DIR}/include
    -DCONFIG_FTEST_SHED_MAX_ENTITIES=${CONFIG_FTEST_SHED_MAX_ENTITIES}
  )
  target_sources(native_simulator INTERFACE src/ftest_sched.c)
endif()

