#include "ftest_entity_loader.h"
#include "ftest_dl.h"
#include "ftest_entity_api.h"
#include "ftest_sched.h"
#include "zephyr/kernel.h"
#include "zephyr/logging/log.h"
#include <errno.h>
#include <stdio.h>

/******************************************************************************
 Definitions
 ******************************************************************************/

#define DT_DRV_COMPAT ftest_entity_loader

/******************************************************************************
 Module configuration
 ******************************************************************************/

LOG_MODULE_REGISTER(entity_loader);

/******************************************************************************
 Structures
 ******************************************************************************/

struct ftest_entity_loader_data {
  void *entity_handle;
  struct ftest_entity_api *api;
  struct ftest_shed_entity_config entity_config;
};

struct ftest_entity_loader_api {
  void *(*get_sym)(const struct device *dev, const char *func_name);
};

/******************************************************************************
 Utilities
 ******************************************************************************/

static void *entity_loader_get_sym_close_on_fail(const struct device *dev,
                                                 const char *sym_name) {
  const struct ftest_entity_loader_api *api = dev->api;
  if (!api || !api->get_sym) {
    LOG_ERR("Entity loader API not initialized");
    return NULL;
  }

  void *symbol = api->get_sym(dev, sym_name);

  if (!symbol) {
    LOG_ERR("Failed to find symbol '%s' in entity library", sym_name);
    struct ftest_entity_loader_data *data = dev->data;
    if (data->entity_handle) {
      ftest_dl_close_lib(data->entity_handle);
      data->entity_handle = NULL;
    }
  }

  return symbol;
}

/******************************************************************************
 Driver implementation
 ******************************************************************************/

static int entity_loader_init(const struct device *dev) {
  const struct ftest_entity_loader_api *api = dev->api;
  const struct ftest_entity_loader_config *config = dev->config;
  struct ftest_entity_loader_data *data = dev->data;

  if (!config || !config->entity_path) {
    return -EINVAL;
  }

  data->entity_handle = ftest_dl_open_lib(config->entity_path);

  if (data->entity_handle == NULL) {
    return -ENOENT;
  }

  struct {
    void *sym_assign;
    const char *sym_name;
  } symbols[] = {
      {&data->entity_config.init_func, "nsi_init"},
      {&data->entity_config.exec_func, "nsi_hws_one_event"},
      {&data->entity_config.find_next_event, "nsi_hws_find_next_event"},
      {&data->entity_config.get_next_event_time, "nsi_hws_get_next_event_time"},
  };

  for (size_t i = 0; i < ARRAY_SIZE(symbols); i++) {
    void **sym_ptr = symbols[i].sym_assign;
    *sym_ptr = entity_loader_get_sym_close_on_fail(dev, symbols[i].sym_name);
    if (!*sym_ptr) {
      return -ENOENT;
    }
  }

  int status = ftest_add_entity_to_schedule(&data->entity_config);
  if (status < 0) {
    LOG_ERR("Failed to add entity to scheduler: %d", status);
    ftest_dl_close_lib(data->entity_handle);
    return status;
  }

  struct ftest_entity_api *(*get_api_func)(void) =
      api->get_sym(dev, "ftest_entity_api_get");

  if (!get_api_func) {
    LOG_ERR("Failed to find 'ftest_entity_api_get' in entity library");
    ftest_dl_close_lib(data->entity_handle);
    return -ENOENT;
  }

  unsigned retry_count = 10;
  while (get_api_func() == NULL) {
    if (--retry_count == 0) {
      LOG_ERR("Entity API not initialized after retries");
      ftest_dl_close_lib(data->entity_handle);
      return -EAGAIN;
    }

    LOG_DBG("Waiting for entity API to be initialized...");
    k_msleep(100);
  }

  data->api = get_api_func();

  if (!data->api || !data->api->initialized) {
    LOG_ERR("Unexpected entity API state: %p", data->api);
    ftest_dl_close_lib(data->entity_handle);
    return -ENOSYS;
  }

  LOG_INF("Entity library loaded successfully: %s", config->entity_path);

  return 0;
}

static void *entity_loader_get_sym(const struct device *dev,
                                   const char *sym_name) {
  struct ftest_entity_loader_data *data = dev->data;

  void *symbol = ftest_dl_get_sym(data->entity_handle, sym_name);

  if (symbol == NULL) {
    return NULL;
  }

  return symbol;
}

/******************************************************************************
 Driver API
 ******************************************************************************/

void *ftest_entity_loader_get_sym(const struct device *dev,
                                  const char *sym_name) {
  const struct ftest_entity_loader_api *api = dev->api;

  if (!api || !api->get_sym) {
    LOG_ERR("Entity loader API not initialized");
    return NULL;
  }

  return api->get_sym(dev, sym_name);
}

struct ftest_entity_api *ftest_entity_loader_get_api(const struct device *dev) {
  struct ftest_entity_loader_data *data = dev->data;

  if (!data || !data->api) {
    LOG_ERR("Entity loader API not initialized");
    return NULL;
  }

  return data->api;
}

/******************************************************************************
 Driver registration
 ******************************************************************************/

#define FTEST_ENTITY_LOADER_INIT(inst)                                         \
  static struct ftest_entity_loader_config entity_loader_config_##inst = {     \
      .entity_path = DT_INST_PROP(inst, entity_path),                          \
  };                                                                           \
                                                                               \
  static struct ftest_entity_loader_data entity_loader_data_##inst = {0};      \
                                                                               \
  static const struct ftest_entity_loader_api entity_loader_api_##inst = {     \
      .get_sym = entity_loader_get_sym,                                        \
  };                                                                           \
                                                                               \
  DEVICE_DT_INST_DEFINE(                                                       \
      inst, entity_loader_init, NULL, &entity_loader_data_##inst,              \
      &entity_loader_config_##inst, POST_KERNEL,                               \
      CONFIG_FTEST_ENTITY_LOADER_INIT_PRIORITY, &entity_loader_api_##inst);

DT_INST_FOREACH_STATUS_OKAY(FTEST_ENTITY_LOADER_INIT)