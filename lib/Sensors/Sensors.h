#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>

// 초기화
void initSensors();

// IR 제어
void startIR();
void stopIR();

// 카메라
void captureImage();
uint8_t* getImageBuffer();
int getImageSize();

// 온도
float readTemperature();

// 버튼
bool isButtonPressed();

#endif