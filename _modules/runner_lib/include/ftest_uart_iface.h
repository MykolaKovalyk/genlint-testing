#pragma once
#include "ftest_dev_iface.h"
#include "zephyr/device.h"
#include "zephyr/drivers/serial/uart_emul.h"

/******************************************************************************
 API
 ******************************************************************************/

static inline void ftest_uart_emul_callback_tx_data_ready_set(
    const struct device *dev, uart_emul_callback_tx_data_ready_t cb,
    void *user_data) {
  struct ftest_remote_dev_iface api = ftest_device_iface_get_remote(dev);

  api.entity_api->uart_emul_callback_tx_data_ready_set(api.remote_dev, cb,
                                                       user_data);
}

static inline uint32_t ftest_uart_emul_put_rx_data(const struct device *dev,
                                                   const uint8_t *data,
                                                   size_t size) {
  struct ftest_remote_dev_iface api = ftest_device_iface_get_remote(dev);
  return api.entity_api->uart_emul_put_rx_data(api.remote_dev, data, size);
}

static inline uint32_t ftest_uart_emul_get_tx_data(const struct device *dev,
                                                   uint8_t *data, size_t size) {
  struct ftest_remote_dev_iface api = ftest_device_iface_get_remote(dev);
  return api.entity_api->uart_emul_get_tx_data(api.remote_dev, data, size);
}

static inline uint32_t ftest_uart_emul_flush_rx_data(const struct device *dev) {
  struct ftest_remote_dev_iface api = ftest_device_iface_get_remote(dev);
  return api.entity_api->uart_emul_flush_rx_data(api.remote_dev);
}

static inline uint32_t ftest_uart_emul_flush_tx_data(const struct device *dev) {
  struct ftest_remote_dev_iface api = ftest_device_iface_get_remote(dev);
  return api.entity_api->uart_emul_flush_tx_data(api.remote_dev);
}

static inline void ftest_uart_emul_set_errors(const struct device *dev,
                                              int errors) {
  struct ftest_remote_dev_iface api = ftest_device_iface_get_remote(dev);
  api.entity_api->uart_emul_set_errors(api.remote_dev, errors);
}

static inline void
ftest_uart_emul_set_release_buffer_on_timeout(const struct device *dev,
                                              bool release_on_timeout) {
  struct ftest_remote_dev_iface api = ftest_device_iface_get_remote(dev);
  api.entity_api->uart_emul_set_release_buffer_on_timeout(api.remote_dev,
                                                          release_on_timeout);
}
