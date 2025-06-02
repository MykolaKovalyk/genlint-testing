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

  /** GPIO */
  int (*gpio_emul_input_set)(const struct device *port, gpio_pin_t pin,
                             int value);
  int (*gpio_emul_input_set_masked)(const struct device *port,
                                    gpio_port_pins_t pins,
                                    gpio_port_value_t values);
  int (*gpio_emul_output_get)(const struct device *port, gpio_pin_t pin);
  int (*gpio_emul_output_get_masked)(const struct device *port,
                                     gpio_port_pins_t pins,
                                     gpio_port_value_t *values);
  int (*gpio_emul_flags_get)(const struct device *port, gpio_pin_t pin,
                             gpio_flags_t *flags);
};

/******************************************************************************
 API
 ******************************************************************************/

const struct ftest_entity_api *ftest_entity_api_get(void);