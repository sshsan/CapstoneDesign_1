#include <Arduino.h>
#include <Comm.h>

void initBLE() {
    Serial.println("[COMM] BLE init complete");
}

void sendAlert(float score) {
    Serial.print("[COMM] ALERT score = ");
    Serial.println(score, 3);
}

void sendBleResult(float score) {
    Serial.print("[COMM] Result score = ");
    Serial.println(score, 3);
}

void sendImage(uint8_t* img, int size) {
    Serial.print("[COMM] Image reserved, size = ");
    Serial.println(size);
}