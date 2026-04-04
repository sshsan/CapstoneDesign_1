#ifndef COMM_H
#define COMM_H

#include <Arduino.h>

void initBLE();
void sendAlert(float score);
void sendBleResult(float score);
void sendImage(uint8_t* img, int size);

#endif