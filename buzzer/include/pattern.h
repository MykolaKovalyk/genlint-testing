
#pragma once

#include "zephyr/sys/util_macro.h"
#include <stdint.h>

/******************************************************************************
 Macros
 ******************************************************************************/

#define BUZZER_STEP(...) (__VA_ARGS__)

#define BUZZER_STEP_UNWRAP_IMPL(...) __VA_ARGS__
#define BUZZER_STEP_UNWRAP(...) BUZZER_STEP_UNWRAP_IMPL __VA_ARGS__

#define BUZZER_PATTERN(...)                                                    \
  {                                                                            \
      .step_count = NUM_VA_ARGS(__VA_ARGS__),                                  \
      .steps =                                                                 \
          {                                                                    \
              FOR_EACH(BUZZER_STEP_UNWRAP, (, ), __VA_ARGS__),                 \
          },                                                                   \
  }

/******************************************************************************
 Structures
 ******************************************************************************/

typedef struct {
  uint32_t duration_ms;
  uint32_t loudness;
} buzzer_step_t;

struct buzzer_pattern {
  uint8_t step_count;
  buzzer_step_t steps[UINT8_MAX];
};

/******************************************************************************
 API
 ******************************************************************************/

int buzzer_pattern_start(const struct buzzer_pattern *pattern);

/******************************************************************************
 Patterns
 ******************************************************************************/

extern const struct buzzer_pattern pattern_silence;

extern const struct buzzer_pattern pattern_beep_beep;

extern const struct buzzer_pattern pattern_beeeeeeee;
