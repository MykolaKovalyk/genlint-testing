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

  target_link_options(native_simulator INTERFACE "-Wl,--export-dynamic")
  zephyr_ld_options(-Wl,--export-dynamic)

  target_sources(native_simulator INTERFACE 
    src/ftest_dl.c
    src/ringbuffer.c
    src/ftest_sched.c
    src/ftest_eth_buf.c
  )

  zephyr_library_sources(
    drivers/ftest_entity_loader.c
    drivers/ftest_dev_iface.c
  )
endif()

