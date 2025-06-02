#include "ftest_entity_api.h"

#if CONFIG_GPIO_EMUL
#include "zephyr/drivers/gpio/gpio_emul.h"
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
};

extern const struct ftest_entity_api *ftest_entity_api;

__attribute__((constructor)) static void ftest_entity_api_initializer(void) {
  ftest_entity_api = &ftest_entity_api_impl;
}