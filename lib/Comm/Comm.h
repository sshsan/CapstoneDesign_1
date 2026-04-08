#ifndef COMM_H
#define COMM_H

#include <Arduino.h>

void initBLE();

void sendStatus(const char* bleStatus,
                const char* detectionStatus,
                float score,
                float lat,
                float lng);

void sendAlert(const char* detectionStatus,
               float score,
               float lat,
               float lng);

void sendImage(uint8_t* img, int size);

#endif