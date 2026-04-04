#include <Arduino.h>
#include <Detection.h>
#include <Sensors.h>

static float clamp01(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

float analyzeIRReflection(uint8_t* img, int size) {
    if (img == nullptr || size <= 0) return 0.0f;

    float score = 0.0f;

    if (size > 9000)  score += 0.20f;
    if (size > 15000) score += 0.20f;
    if (size > 22000) score += 0.20f;

    score += 0.15f;

    return clamp01(score);
}

float analyzeTemperature(float temp) {
    float score = 0.0f;

    if (temp >= 28.0f) score = 0.25f;
    if (temp >= 30.0f) score = 0.45f;
    if (temp >= 33.0f) score = 0.70f;
    if (temp >= 36.0f) score = 1.00f;

    return clamp01(score);
}

float analyzeData() {
    uint8_t* img = getImageBuffer();
    int size = getImageSize();
    return clamp01(analyzeIRReflection(img, size));
}

float analyzeDataPrecision() {
    float sum = 0.0f;

    for (int i = 0; i < 5; i++) {
        float temp = readTemperature();
        sum += temp;
        Serial.printf("[TEMP] %d/5 : %.2f C\n", i + 1, temp);
        delay(120);
    }

    float avgTemp = sum / 5.0f;
    Serial.printf("[TEMP] Avg : %.2f C\n", avgTemp);

    return analyzeTemperature(avgTemp);
}