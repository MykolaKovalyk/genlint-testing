#pragma once
#include "ftest_dev_iface.h"
#include "ftest_entity_api.h"
#include "zephyr/device.h"
#include "zephyr/drivers/adc/adc_emul.h"

/******************************************************************************
 API
 ******************************************************************************/

static inline int ftest_adc_emul_const_value_set(const struct device *dev,
                                                 unsigned int chan,
                                                 uint32_t value) {
  struct ftest_remote_dev_iface api = ftest_device_iface_get_remote(dev);
  return api.entity_api->adc_emul_const_value_set(api.remote_dev, chan, value);
}

static inline int ftest_adc_emul_const_raw_value_set(const struct device *dev,
                                                     unsigned int chan,
                                                     uint32_t raw_value) {
  struct ftest_remote_dev_iface api = ftest_device_iface_get_remote(dev);
  return api.entity_api->adc_emul_const_raw_value_set(api.remote_dev, chan,
                                                      raw_value);
}

static inline int ftest_adc_emul_value_func_set(const struct device *dev,
                                                unsigned int chan,
                                                adc_emul_value_func func,
                                                void *data) {
  struct ftest_remote_dev_iface api = ftest_device_iface_get_remote(dev);
  return api.entity_api->adc_emul_value_func_set(api.remote_dev, chan, func,
                                                 data);
}

static inline int ftest_adc_emul_raw_value_func_set(const struct device *dev,
                                                    unsigned int chan,
                                                    adc_emul_value_func func,
                                                    void *data) {
  struct ftest_remote_dev_iface api = ftest_device_iface_get_remote(dev);
  return api.entity_api->adc_emul_raw_value_func_set(api.remote_dev, chan, func,
                                                     data);
}

static inline int ftest_adc_emul_ref_voltage_set(const struct device *dev,
                                                 enum adc_reference ref,
                                                 uint16_t value) {
  struct ftest_remote_dev_iface api = ftest_device_iface_get_remote(dev);
  return api.entity_api->adc_emul_ref_voltage_set(api.remote_dev, ref, value);
}