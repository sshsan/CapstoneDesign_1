#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>

void initSensors();

void startIR();
void stopIR();

void captureImage();
uint8_t* getImageBuffer();
int getImageSize();

float readTemperature();

bool isButtonPressed();

#endif