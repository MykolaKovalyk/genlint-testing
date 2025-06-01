#pragma once
#include "zephyr/device.h"
#include "zephyr/drivers/gpio.h"
#include <stdbool.h>

/******************************************************************************
 API structure
 ******************************************************************************/

struct ftest_entity_api {
  bool initialized;
  const struct device *(*device_get_binding)(const char *name);
};

/******************************************************************************
 API
 ******************************************************************************/

const struct ftest_entity_api *ftest_entity_api_get(void);