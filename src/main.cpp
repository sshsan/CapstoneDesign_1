#include <Arduino.h>
#include <Pins.h>
#include <Config.h>
#include <LED.h>
#include <Sensors.h>
#include <Detection.h>
#include <Comm.h>
#include "esp_sleep.h"

enum DeviceState {
    STATE_BOOT = 0,
    STATE_IDLE,
    STATE_BLE_WAIT,
    STATE_FIRST_SCAN,
    STATE_SECOND_SCAN,
    STATE_RESULT,
    STATE_SLEEP
};

static DeviceState currentState = STATE_BOOT;

static unsigned long g_idleEnteredAt = 0;
static unsigned long g_resultEnteredAt = 0;

static float g_imageScore = 0.0f;
static float g_thermalScore = 0.0f;
static float g_finalScore = 0.0f;
static bool g_suspicious = false;

// 현재는 GPS 모듈이 없으므로 더미값 사용
static float g_latitude = 37.566500f;
static float g_longitude = 126.978000f;

// 앱 표시용 BLE 상태 문자열
static const char* g_bleStatusText = "연결됨";

static void userLedOn(bool on) {
    pinMode(PIN_USER_LED, OUTPUT);
    digitalWrite(PIN_USER_LED, on ? HIGH : LOW);
}

static void rgbOff() {
    digitalWrite(PIN_LED_R, LOW);
    digitalWrite(PIN_LED_G, LOW);
    digitalWrite(PIN_LED_B, LOW);
}

static void blinkDanger(int count, int delayMs) {
    for (int i = 0; i < count; i++) {
        setLEDDanger();
        userLedOn(true);
        delay(delayMs);

        rgbOff();
        userLedOn(false);
        delay(delayMs);
    }
}

static void setLEDWarning() {
    digitalWrite(PIN_LED_R, HIGH);
    digitalWrite(PIN_LED_G, HIGH);
    digitalWrite(PIN_LED_B, LOW);
}

static const char* getDetectionStatusText(float score) {
    if (score >= DANGER_THRESHOLD) {
        return "위험";
    } else if (score >= WARNING_THRESHOLD) {
        return "주의";
    } else {
        return "안전";
    }
}

static void applyResultLeds(float finalScore) {
    if (finalScore >= DANGER_THRESHOLD) {
        g_suspicious = true;
        setLEDDanger();
        userLedOn(true);
    } else if (finalScore >= WARNING_THRESHOLD) {
        g_suspicious = true;
        setLEDWarning();
        userLedOn(true);
    } else {
        g_suspicious = false;
        setLEDSafe();
        userLedOn(false);
    }
}

static void sendCurrentStatus(float score) {
    const char* detectionStatus = getDetectionStatusText(score);

    sendStatus(
        g_bleStatusText,
        detectionStatus,
        score,
        g_latitude,
        g_longitude
    );

    if (score >= DANGER_THRESHOLD) {
        sendAlert(
            detectionStatus,
            score,
            g_latitude,
            g_longitude
        );
    }
}

static void goDeepSleep() {
    Serial.println("[POWER] Entering deep sleep...");

    stopIR();
    rgbOff();
    userLedOn(false);

    esp_sleep_enable_ext0_wakeup(GPIO_NUM_1, 1);
    delay(100);
    esp_deep_sleep_start();
}

static void enterState(DeviceState next) {
    currentState = next;

    switch (currentState) {
        case STATE_IDLE:
            g_idleEnteredAt = millis();
            setLEDIdle();
            userLedOn(false);
            Serial.println("[STATE] IDLE");

            // 대기 상태일 때 앱에도 기본 상태 전송
            sendCurrentStatus(0.0f);
            break;

        case STATE_BLE_WAIT:
            setLEDScanning();
            Serial.println("[STATE] BLE_WAIT");
            break;

        case STATE_FIRST_SCAN:
            setLEDScanning();
            Serial.println("[STATE] FIRST_SCAN");
            break;

        case STATE_SECOND_SCAN:
            setLEDScanning();
            Serial.println("[STATE] SECOND_SCAN");
            break;

        case STATE_RESULT:
            g_resultEnteredAt = millis();
            Serial.println("[STATE] RESULT");
            break;

        case STATE_SLEEP:
            Serial.println("[STATE] SLEEP");
            break;

        case STATE_BOOT:
        default:
            break;
    }
}

void setup() {
    Serial.begin(115200);
    delay(1200);

    pinMode(PIN_BUTTON, INPUT);
    pinMode(PIN_IR_LED, OUTPUT);
    pinMode(PIN_USER_LED, OUTPUT);

    digitalWrite(PIN_IR_LED, LOW);

    initLED();
    initSensors();
    initBLE();

    Serial.println();
    Serial.println("===================================");
    Serial.println(" Owl Guard - XIAO ESP32-S3 Sense");
    Serial.println("===================================");

    // 앱 초기 화면용 상태 전송
    sendStatus(g_bleStatusText, "안전", 0.0f, g_latitude, g_longitude);

    enterState(STATE_IDLE);
}

void loop() {
    switch (currentState) {
        case STATE_IDLE: {
            if (isButtonPressed()) {
                enterState(STATE_BLE_WAIT);
                return;
            }

            if (millis() - g_idleEnteredAt >= IDLE_TO_SLEEP_MS) {
                enterState(STATE_SLEEP);
                return;
            }

            delay(10);
            break;
        }

        case STATE_BLE_WAIT: {
            delay(BLE_WAIT_MS);
            enterState(STATE_FIRST_SCAN);
            break;
        }

        case STATE_FIRST_SCAN: {
            startIR();
            delay(IR_STABLE_MS);

            captureImage();
            g_imageScore = analyzeData();

            stopIR();

            Serial.printf("[SCAN1] imageScore = %.3f\n", g_imageScore);

            if (g_imageScore < IMAGE_SCORE_SKIP_THERMAL) {
                g_thermalScore = 0.0f;
                g_finalScore = g_imageScore;

                applyResultLeds(g_finalScore);
                sendCurrentStatus(g_finalScore);

                enterState(STATE_RESULT);
            } else {
                enterState(STATE_SECOND_SCAN);
            }

            break;
        }

        case STATE_SECOND_SCAN: {
            g_thermalScore = analyzeDataPrecision();
            g_finalScore = (g_imageScore * 0.65f) + (g_thermalScore * 0.35f);

            if (g_finalScore < 0.0f) g_finalScore = 0.0f;
            if (g_finalScore > 1.0f) g_finalScore = 1.0f;

            Serial.printf("[SCAN2] thermalScore = %.3f\n", g_thermalScore);
            Serial.printf("[FUSE ] finalScore   = %.3f\n", g_finalScore);

            applyResultLeds(g_finalScore);
            sendCurrentStatus(g_finalScore);

            enterState(STATE_RESULT);
            break;
        }

        case STATE_RESULT: {
            if (g_suspicious) {
                blinkDanger(2, 140);
                applyResultLeds(g_finalScore);
            }

            if (millis() - g_resultEnteredAt >= RESULT_SHOW_MS) {
                enterState(STATE_SLEEP);
            } else {
                delay(20);
            }
            break;
        }

        case STATE_SLEEP: {
            goDeepSleep();
            break;
        }

        case STATE_BOOT:
        default:
            enterState(STATE_IDLE);
            break;
    }
}