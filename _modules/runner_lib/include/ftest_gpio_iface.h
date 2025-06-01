#pragma once
#include "zephyr/device.h"
#include "zephyr/drivers/gpio.h"

/******************************************************************************
 Driver API
 ******************************************************************************/

int ftest_gpio_emul_input_set_masked(const struct device *gpio_iface,
                                     gpio_port_pins_t pins,
                                     gpio_port_value_t values);
int ftest_gpio_emul_input_set(const struct device *gpio_iface, gpio_pin_t pin,
                              int value);

int ftest_gpio_emul_output_get_masked(const struct device *gpio_iface,
                                      gpio_port_pins_t pins,
                                      gpio_port_value_t *values);

int ftest_gpio_emul_output_get(const struct device *gpio_iface, gpio_pin_t pin);

int ftest_gpio_emul_flags_get(const struct device *gpio_iface, gpio_pin_t pin,
                              gpio_flags_t *flags);