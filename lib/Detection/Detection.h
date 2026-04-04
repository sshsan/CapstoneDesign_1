#ifndef DETECTION_H
#define DETECTION_H

#include <Arduino.h>

float analyzeData();
float analyzeDataPrecision();

float analyzeIRReflection(uint8_t* img, int size);
float analyzeTemperature(float temp);

#endif