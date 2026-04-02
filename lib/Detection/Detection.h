#ifndef DETECTION_H
#define DETECTION_H

#include <Arduino.h>

// 1차 탐지
float analyzeData();

// 2차 정밀 탐지
float analyzeDataPrecision();

// 내부적으로 사용할 수도 있음
float analyzeIRReflection(uint8_t* img, int size);
float analyzeTemperature(float temp);

#endif