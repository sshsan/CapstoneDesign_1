#ifndef LED_H
#define LED_H

#include <Arduino.h>

void initLED();
void setLEDIdle();
void setLEDSafe();
void setLEDDanger();
void setLEDScanning();

#endif