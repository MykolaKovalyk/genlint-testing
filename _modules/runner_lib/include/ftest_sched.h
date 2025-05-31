#pragma once

/******************************************************************************
 Structures
 ******************************************************************************/

#include <stdint.h>
struct ftest_shed_entity_config {
  void (*init_func)(int argc, char *argv[]);
  void (*exec_func)(void);
  void (*find_next_event)(void);
  uint64_t (*get_next_event_time)(void);
};

/******************************************************************************
 API
 ******************************************************************************/

int ftest_add_entity_to_schedule(
    struct ftest_shed_entity_config *entity_config);
