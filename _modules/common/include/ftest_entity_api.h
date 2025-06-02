#pragma once
#include "zephyr/device.h"
#include "zephyr/drivers/gpio.h"
#include "zephyr/drivers/serial/uart_emul.h"
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

  /** UART */
  void (*uart_emul_callback_tx_data_ready_set)(
      const struct device *dev, uart_emul_callback_tx_data_ready_t cb,
      void *user_data);
  uint32_t (*uart_emul_put_rx_data)(const struct device *dev,
                                    const uint8_t *data, size_t size);
  uint32_t (*uart_emul_get_tx_data)(const struct device *dev, uint8_t *data,
                                    size_t size);
  uint32_t (*uart_emul_flush_rx_data)(const struct device *dev);
  uint32_t (*uart_emul_flush_tx_data)(const struct device *dev);
  void (*uart_emul_set_errors)(const struct device *dev, int errors);
  void (*uart_emul_set_release_buffer_on_timeout)(const struct device *dev,
                                                  bool release_on_timeout);
};

/******************************************************************************
 API
 ******************************************************************************/

const struct ftest_entity_api *ftest_entity_api_get(void);