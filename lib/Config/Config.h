#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

static const unsigned long DEBOUNCE_MS          = 50;
static const unsigned long BLE_WAIT_MS          = 2500;
static const unsigned long IR_STABLE_MS         = 250;
static const unsigned long RESULT_SHOW_MS       = 5000;
static const unsigned long IDLE_TO_SLEEP_MS     = 20000;

static const float IMAGE_SCORE_SKIP_THERMAL = 0.30f;
static const float WARNING_THRESHOLD        = 0.45f;
static const float DANGER_THRESHOLD         = 0.75f;

#endif