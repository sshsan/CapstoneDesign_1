#include <Arduino.h>
#include <Pins.h>
#include <LED.h>

void initLED() {
    pinMode(PIN_LED_R, OUTPUT);
    pinMode(PIN_LED_G, OUTPUT);
    pinMode(PIN_LED_B, OUTPUT);

    digitalWrite(PIN_LED_R, LOW);
    digitalWrite(PIN_LED_G, LOW);
    digitalWrite(PIN_LED_B, LOW);
}

void setLEDIdle() {
    digitalWrite(PIN_LED_R, LOW);
    digitalWrite(PIN_LED_G, LOW);
    digitalWrite(PIN_LED_B, HIGH);
}

void setLEDSafe() {
    digitalWrite(PIN_LED_R, LOW);
    digitalWrite(PIN_LED_G, HIGH);
    digitalWrite(PIN_LED_B, LOW);
}

void setLEDDanger() {
    digitalWrite(PIN_LED_R, HIGH);
    digitalWrite(PIN_LED_G, LOW);
    digitalWrite(PIN_LED_B, LOW);
}

void setLEDScanning() {
    digitalWrite(PIN_LED_R, HIGH);
    digitalWrite(PIN_LED_G, LOW);
    digitalWrite(PIN_LED_B, HIGH);
}