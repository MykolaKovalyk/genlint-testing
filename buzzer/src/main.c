#include "command_processor.h"
#include "pattern.h"
#include "zephyr/device.h"
#include "zephyr/drivers/gpio.h"
#include "zephyr/drivers/gpio/gpio_emul.h"
#include "zephyr/logging/log.h"
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(buzzer_main);

#if CONFIG_GPIO_EMUL

static const struct gpio_dt_spec buzzer =
    GPIO_DT_SPEC_GET(DT_ALIAS(buzzer_pin), gpios);

static void logger(void *unused0, void *unused1, void *unused2) {
  while (true) {
    LOG_WRN("Buzzer pin state: %d",
            gpio_emul_output_get(buzzer.port, buzzer.pin));
    k_msleep(10);
  }
}

K_THREAD_DEFINE(logger_thread, 1024, logger, NULL, NULL, NULL,
                K_PRIO_PREEMPT(10), 0, 0);

#endif // CONNFIG_GPIO_EMUL

int play_buzzer_short_beep(void) {
  LOG_INF("Playing short beep");
  buzzer_pattern_start(&pattern_beep_beep);
  return 0;
}

int play_buzzer_long_beep(void) {
  LOG_INF("Playing long beep");
  buzzer_pattern_start(&pattern_beeeeeeee);
  return 0;
}

int stop_buzzer(void) {
  LOG_INF("Stopping buzzer");
  buzzer_pattern_start(&pattern_silence);
  return 0;
}

int main(void) {

  command_processor_register_handler('b', play_buzzer_short_beep);
  command_processor_register_handler('l', play_buzzer_long_beep);
  command_processor_register_handler('s', stop_buzzer);
  return 0;
}
