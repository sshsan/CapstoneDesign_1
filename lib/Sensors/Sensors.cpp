#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include "esp_camera.h"

#include <Pins.h>
#include <Config.h>
#include <Sensors.h>

static Adafruit_MLX90614 g_mlx = Adafruit_MLX90614();

static bool g_cameraReady = false;
static bool g_thermalReady = false;
static camera_fb_t* g_fb = nullptr;

static bool g_lastButtonReading = LOW;
static bool g_buttonStableState = LOW;
static unsigned long g_lastDebounceTime = 0;

static bool initCameraInternal() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;

    config.pin_d0 = CAM_PIN_D0;
    config.pin_d1 = CAM_PIN_D1;
    config.pin_d2 = CAM_PIN_D2;
    config.pin_d3 = CAM_PIN_D3;
    config.pin_d4 = CAM_PIN_D4;
    config.pin_d5 = CAM_PIN_D5;
    config.pin_d6 = CAM_PIN_D6;
    config.pin_d7 = CAM_PIN_D7;

    config.pin_xclk = CAM_PIN_XCLK;
    config.pin_pclk = CAM_PIN_PCLK;
    config.pin_vsync = CAM_PIN_VSYNC;
    config.pin_href = CAM_PIN_HREF;
    config.pin_sccb_sda = CAM_PIN_SIOD;
    config.pin_sccb_scl = CAM_PIN_SIOC;

    config.pin_pwdn = CAM_PIN_PWDN;
    config.pin_reset = CAM_PIN_RESET;

    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;

    if (psramFound()) {
        config.frame_size = FRAMESIZE_QVGA;
        config.jpeg_quality = 12;
        config.fb_count = 2;
        config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
        config.frame_size = FRAMESIZE_QQVGA;
        config.jpeg_quality = 15;
        config.fb_count = 1;
        config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    }

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("[CAM] Init failed: 0x%x\n", err);
        return false;
    }

    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        s->set_brightness(s, 0);
        s->set_contrast(s, 1);
        s->set_saturation(s, 0);
    }

    Serial.println("[CAM] Init success");
    return true;
}

void initSensors() {
    pinMode(PIN_BUTTON, INPUT);
    pinMode(PIN_IR_LED, OUTPUT);
    digitalWrite(PIN_IR_LED, LOW);

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    g_thermalReady = g_mlx.begin();

    if (g_thermalReady) {
        Serial.println("[MLX90614] Init success");
    } else {
        Serial.println("[MLX90614] Init failed");
    }

    g_cameraReady = initCameraInternal();
}

void startIR() {
    digitalWrite(PIN_IR_LED, HIGH);
}

void stopIR() {
    digitalWrite(PIN_IR_LED, LOW);
}

void captureImage() {
    if (!g_cameraReady) {
        Serial.println("[CAM] Not ready");
        return;
    }

    if (g_fb != nullptr) {
        esp_camera_fb_return(g_fb);
        g_fb = nullptr;
    }

    g_fb = esp_camera_fb_get();
    if (!g_fb) {
        Serial.println("[CAM] Capture failed");
        return;
    }

    Serial.printf("[CAM] Captured: %u bytes, %dx%d\n", g_fb->len, g_fb->width, g_fb->height);
}

uint8_t* getImageBuffer() {
    if (g_fb == nullptr) return nullptr;
    return g_fb->buf;
}

int getImageSize() {
    if (g_fb == nullptr) return 0;
    return static_cast<int>(g_fb->len);
}

float readTemperature() {
    if (!g_thermalReady) return 0.0f;
    return g_mlx.readObjectTempC();
}

bool isButtonPressed() {
    bool reading = digitalRead(PIN_BUTTON);

    if (reading != g_lastButtonReading) {
        g_lastDebounceTime = millis();
    }

    if ((millis() - g_lastDebounceTime) > DEBOUNCE_MS) {
        if (reading != g_buttonStableState) {
            g_buttonStableState = reading;
            if (g_buttonStableState == HIGH) {
                g_lastButtonReading = reading;
                return true;
            }
        }
    }

    g_lastButtonReading = reading;
    return false;
}