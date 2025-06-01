#include "ftest_dev_iface.h"

#include "ftest_entity_api.h"
#include "ftest_entity_loader.h"
#include "zephyr/devicetree.h"
#include "zephyr/logging/log.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/******************************************************************************
 Definitions
 ******************************************************************************/

#define DT_DRV_COMPAT ftest_dev_iface
#define DT_PARENT_COMPAT ftest_entity_loader

/******************************************************************************
 Assumptions
 ******************************************************************************/

static_assert(CONFIG_FTEST_IFACE_INIT_PRIORITY >
              CONFIG_FTEST_ENTITY_LOADER_INIT_PRIORITY);

/******************************************************************************
 Module configuration
 ******************************************************************************/

LOG_MODULE_REGISTER(gpio_iface);

/******************************************************************************
 Structures
 ******************************************************************************/

struct ftest_device_iface_config {
  const char *remote_label;
  const struct device *entity_dev;
};

/******************************************************************************
 Helpers
 ******************************************************************************/

static const struct device *
ftest_iface_get_remote_device(const struct device *entity,
                              const char *dev_name) {
  const struct ftest_entity_api *ftest_entity_api =
      ftest_entity_loader_get_api(entity);

  if (!ftest_entity_api) {
    LOG_ERR("Failed to get device_get_binding symbol from entity loader");
    return NULL;
  }

  const struct device *remote_dev =
      ftest_entity_api->device_get_binding(dev_name);
  if (!remote_dev) {
    LOG_ERR("Failed to get remote device %s", dev_name);
    return NULL;
  }

  return remote_dev;
}

static int
ftest_iface_get_api(const struct device *entity_dev,
                    const struct ftest_entity_api **ftest_entity_api) {

  if (!entity_dev || !device_is_ready(entity_dev)) {
    LOG_ERR("Entity device %s is not ready", entity_dev->name);
    return -ENODEV;
  }

  *ftest_entity_api = ftest_entity_loader_get_api(entity_dev);

  if (!*ftest_entity_api) {
    LOG_ERR("Failed to get device_get_binding symbol from entity loader");
    return -ENOSYS;
  }

  return 0;
}

/******************************************************************************
 Driver implementation
 ******************************************************************************/

static int device_iface_init(const struct device *dev) {
  const struct ftest_device_iface_config *config = dev->config;
  struct ftest_remote_dev_iface *data = dev->data;

  if (!device_is_ready(config->entity_dev)) {
    LOG_ERR("Entity device %s is not ready", config->entity_dev->name);
    return -ENODEV;
  }

  data->remote_dev =
      ftest_iface_get_remote_device(config->entity_dev, config->remote_label);

  if (!data->remote_dev) {
    LOG_ERR("Failed to get remote device %s", config->remote_label);
    return -ENODEV;
  }

  int ret = ftest_iface_get_api(config->entity_dev, &data->entity_api);

  if (ret < 0) {
    LOG_ERR("Failed to get entity API: %d", ret);
    return ret;
  }

  LOG_INF(
      "Runner interface initialized with remote device name: %s, address: %p",
      config->remote_label, data->remote_dev);
  return 0;
}

/******************************************************************************
 Driver API
 ******************************************************************************/

const struct ftest_remote_dev_iface
ftest_device_iface_get_remote(const struct device *iface_dev) {
  if (!iface_dev || !iface_dev->data) {
    k_panic();
  }

  const struct ftest_remote_dev_iface *remote_iface = iface_dev->data;

  if (!remote_iface->remote_dev) {
    k_panic();
  }

  if (!remote_iface->entity_api) {
    k_panic();
  }

  return *remote_iface;
}

/******************************************************************************
 Driver registration
 ******************************************************************************/

#define FTEST_GPIO_IFACE_DEFINE(inst)                                          \
  static_assert(                                                               \
      DT_NODE_HAS_COMPAT(DT_INST_PARENT(inst), DT_PARENT_COMPAT),              \
      "The parent node of \"" STRINGIFY(                                       \
          DT_DRV_COMPAT) "\" driver shall necessarily be an instance of "      \
                         "\"" STRINGIFY(DT_PARENT_COMPAT) "\"");               \
                                                                               \
  static const struct ftest_device_iface_config                                \
      ftest_device_iface_config_##inst = {                                     \
          .remote_label = DT_INST_PROP(inst, remote_label),                    \
          .entity_dev = DEVICE_DT_GET(DT_INST_PARENT(inst)),                   \
  };                                                                           \
                                                                               \
  static struct ftest_remote_dev_iface ftest_device_iface_data_##inst = {0};   \
                                                                               \
  DEVICE_DT_INST_DEFINE(inst, device_iface_init, NULL,                         \
                        &ftest_device_iface_data_##inst,                       \
                        &ftest_device_iface_config_##inst, POST_KERNEL,        \
                        CONFIG_FTEST_IFACE_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(FTEST_GPIO_IFACE_DEFINE)