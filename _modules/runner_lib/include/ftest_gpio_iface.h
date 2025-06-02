#pragma once
#include "ftest_dev_iface.h"
#include "ftest_entity_api.h"
#include "zephyr/device.h"
#include "zephyr/drivers/gpio.h"

/******************************************************************************
 API
 ******************************************************************************/

static inline int
ftest_gpio_emul_input_set_masked(const struct device *gpio_iface,
                                 gpio_port_pins_t pins,
                                 gpio_port_value_t values) {
  struct ftest_remote_dev_iface api = ftest_device_iface_get_remote(gpio_iface);
  return api.entity_api->gpio_emul_input_set_masked(api.remote_dev, pins,
                                                    values);
}

static inline int ftest_gpio_emul_input_set(const struct device *gpio_iface,
                                            gpio_pin_t pin, int value) {
  struct ftest_remote_dev_iface api = ftest_device_iface_get_remote(gpio_iface);
  return api.entity_api->gpio_emul_input_set(api.remote_dev, pin, value);
}

static inline int
ftest_gpio_emul_output_get_masked(const struct device *gpio_iface,
                                  gpio_port_pins_t pins,
                                  gpio_port_value_t *values)

{
  struct ftest_remote_dev_iface api = ftest_device_iface_get_remote(gpio_iface);
  return api.entity_api->gpio_emul_output_get_masked(api.remote_dev, pins,
                                                     values);
}

static inline int ftest_gpio_emul_output_get(const struct device *gpio_iface,
                                             gpio_pin_t pin) {
  struct ftest_remote_dev_iface api = ftest_device_iface_get_remote(gpio_iface);
  return api.entity_api->gpio_emul_output_get(api.remote_dev, pin);
}

static inline int ftest_gpio_emul_flags_get(const struct device *gpio_iface,
                                            gpio_pin_t pin,
                                            gpio_flags_t *flags) {
  struct ftest_remote_dev_iface api = ftest_device_iface_get_remote(gpio_iface);
  return api.entity_api->gpio_emul_flags_get(api.remote_dev, pin, flags);
}