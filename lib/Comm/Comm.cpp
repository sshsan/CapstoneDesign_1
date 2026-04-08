#include <Arduino.h>
#include <Comm.h>

void initBLE() {
    Serial.println("[COMM] BLE init complete");
}

void sendStatus(const char* bleStatus,
                const char* detectionStatus,
                float score,
                float lat,
                float lng) {
    String payload = "{";
    payload += "\"type\":\"status\",";
    payload += "\"ble\":\"" + String(bleStatus) + "\",";
    payload += "\"detection\":\"" + String(detectionStatus) + "\",";
    payload += "\"score\":" + String(score, 3) + ",";
    payload += "\"lat\":" + String(lat, 6) + ",";
    payload += "\"lng\":" + String(lng, 6);
    payload += "}";

    Serial.println(payload);
}

void sendAlert(const char* detectionStatus,
               float score,
               float lat,
               float lng) {
    String payload = "{";
    payload += "\"type\":\"alert\",";
    payload += "\"detection\":\"" + String(detectionStatus) + "\",";
    payload += "\"score\":" + String(score, 3) + ",";
    payload += "\"lat\":" + String(lat, 6) + ",";
    payload += "\"lng\":" + String(lng, 6) + ",";
    payload += "\"emergency\":true";
    payload += "}";

    Serial.println(payload);
}

void sendImage(uint8_t* img, int size) {
    String payload = "{";
    payload += "\"type\":\"image\",";
    payload += "\"size\":" + String(size);
    payload += "}";

    Serial.println(payload);
}