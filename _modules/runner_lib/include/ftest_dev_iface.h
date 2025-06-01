#pragma once
#include "zephyr/device.h"

/******************************************************************************
 Driver API
 ******************************************************************************/

struct ftest_remote_dev_iface {
  const struct device *remote_dev;
  const struct ftest_entity_api *entity_api;
};

/******************************************************************************
 Driver API
 ******************************************************************************/

const struct ftest_remote_dev_iface
ftest_device_iface_get_remote(const struct device *iface_dev);