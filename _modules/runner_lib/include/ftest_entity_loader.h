#pragma once
#include "zephyr/device.h"

/******************************************************************************
 Structures
 ******************************************************************************/

struct ftest_entity_loader_config {
  const char *entity_path;
};

/******************************************************************************
 API
 ******************************************************************************/

void *ftest_entity_loader_get_sym(const struct device *dev,
                                  const char *sym_name);