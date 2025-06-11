
#include "pattern.h"

const struct buzzer_pattern pattern_silence = BUZZER_PATTERN(BUZZER_STEP({
    .duration_ms = UINT32_MAX,
    .loudness = 0,
}));

const struct buzzer_pattern pattern_beep_beep =
    BUZZER_PATTERN(BUZZER_STEP({
                       .duration_ms = 100,
                       .loudness = 100,
                   }),
                   BUZZER_STEP({
                       .duration_ms = 100,
                       .loudness = 0,
                   }),
                   BUZZER_STEP({
                       .duration_ms = 100,
                       .loudness = 100,
                   }),
                   BUZZER_STEP({
                       .duration_ms = 100,
                       .loudness = 0,
                   }));

const struct buzzer_pattern pattern_beeeeeeee =
    BUZZER_PATTERN(BUZZER_STEP({
                       .duration_ms = 500,
                       .loudness = 100,
                   }),
                   BUZZER_STEP({
                       .duration_ms = UINT32_MAX,
                       .loudness = 0,
                   }));