#include "pattern.h"
#include "zephyr/drivers/gpio.h"
#include "zephyr/kernel.h"
#include "zephyr/logging/log.h"
#include "zephyr/sys_clock.h"

/******************************************************************************
 Definitions
 ******************************************************************************/

LOG_MODULE_REGISTER(pattern_processor);

K_EVENT_DEFINE(pattern_event);

#define PATTERN_EVENT_START BIT(0)

/******************************************************************************
 Data
 ******************************************************************************/

static const struct buzzer_pattern *active_pattern = &pattern_silence;

static const struct gpio_dt_spec buzzer =
    GPIO_DT_SPEC_GET(DT_ALIAS(buzzer_pin), gpios);

/******************************************************************************
 API
 ******************************************************************************/

int buzzer_pattern_start(const struct buzzer_pattern *pattern) {
  active_pattern = pattern;
  k_event_post(&pattern_event, PATTERN_EVENT_START);

  return 0;
}

/******************************************************************************
 Processor thread
 ******************************************************************************/

static void pattern_processor(void *unused0, void *unused1, void *unused2) {
  int res = gpio_pin_configure_dt(&buzzer, GPIO_OUTPUT);

  if (res < 0) {
    LOG_ERR("Configuring buzzer pin failed");
    return;
  }

  k_timeout_t current_iter_duration = Z_FOREVER;
  unsigned current_pattern_step = 0;

  while (true) {
    uint32_t events = k_event_wait(&pattern_event, PATTERN_EVENT_START, false,
                                   current_iter_duration);

    if (events != 0) {
      k_event_clear(&pattern_event, PATTERN_EVENT_START);
      current_pattern_step = 0;

      LOG_INF("Pattern started: %p", active_pattern);
    }

    if (active_pattern == NULL) {
      LOG_ERR("Error - no pattern specified");
      current_iter_duration = Z_FOREVER;
      continue;
    }

    unsigned step_index = current_pattern_step % active_pattern->step_count;
    const buzzer_step_t *step = &active_pattern->steps[step_index];

    int ret = gpio_pin_set_dt(&buzzer, step->loudness > 0);

    if (ret < 0) {
      LOG_ERR("Unable to set output value for buzzer");
    }

    current_iter_duration = Z_TIMEOUT_MS(step->duration_ms);
    current_pattern_step++;
  }
}

K_THREAD_DEFINE(thread_pattern_processor, 2048, pattern_processor, NULL, NULL,
                NULL, K_PRIO_COOP(10), 0, 0);
