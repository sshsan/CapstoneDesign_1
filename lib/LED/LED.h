#ifndef LED_H
#define LED_H

#include <Arduino.h>

// 초기화
void initLED();

// 상태 표시
void setLEDIdle();
void setLEDSafe();
void setLEDDanger();
void setLEDScanning();

#endif