# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

if (CONFIG_FTEST_ENTITY)
  zephyr_library()
  zephyr_include_directories(include)
  zephyr_include_directories(../common/include)

  # Compile the whole binary as a shared library
  if(NOT CONFIG_FTEST)
    target_compile_options(native_simulator INTERFACE "-fPIC")
    target_link_options(native_simulator INTERFACE "-shared -Wl,-z,nodynamic-undefined-weak")
  endif()

  # Exclude main (scheduler), as it will be provided by runner
  target_compile_options(native_simulator INTERFACE "-DNSI_NO_MAIN=1")
endif()

