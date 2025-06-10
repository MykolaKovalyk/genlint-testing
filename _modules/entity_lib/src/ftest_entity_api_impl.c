#include "ftest_entity_api.h"

#if CONFIG_GPIO_EMUL
#include "zephyr/drivers/gpio/gpio_emul.h"
#endif

#if CONFIG_UART_EMUL
#include "zephyr/drivers/serial/uart_emul.h"
#endif

#if CONFIG_ADC_EMUL
#include "zephyr/drivers/adc/adc_emul.h"
#endif

static struct ftest_entity_api ftest_entity_api_impl = {
    .initialized = true,
    .device_get_binding = device_get_binding,
#if CONFIG_GPIO_EMUL
    .gpio_emul_input_set = gpio_emul_input_set,
    .gpio_emul_input_set_masked = gpio_emul_input_set_masked,
    .gpio_emul_output_get = gpio_emul_output_get,
    .gpio_emul_output_get_masked = gpio_emul_output_get_masked,
    .gpio_emul_flags_get = gpio_emul_flags_get,
#endif

#if CONFIG_UART_EMUL
    .uart_emul_callback_tx_data_ready_set =
        uart_emul_callback_tx_data_ready_set,
    .uart_emul_put_rx_data = uart_emul_put_rx_data,
    .uart_emul_get_tx_data = uart_emul_get_tx_data,
    .uart_emul_flush_rx_data = uart_emul_flush_rx_data,
    .uart_emul_flush_tx_data = uart_emul_flush_tx_data,
    .uart_emul_set_errors = uart_emul_set_errors,
    .uart_emul_set_release_buffer_on_timeout =
        uart_emul_set_release_buffer_on_timeout,
#endif

#if CONFIG_ADC_EMUL
    .adc_emul_const_value_set = adc_emul_const_value_set,
    .adc_emul_const_raw_value_set = adc_emul_const_raw_value_set,
    .adc_emul_value_func_set = adc_emul_value_func_set,
    .adc_emul_raw_value_func_set = adc_emul_raw_value_func_set,
    .adc_emul_ref_voltage_set = adc_emul_ref_voltage_set,
#endif
};

extern const struct ftest_entity_api *ftest_entity_api;

__attribute__((constructor)) static void ftest_entity_api_initializer(void) {
  ftest_entity_api = &ftest_entity_api_impl;
}