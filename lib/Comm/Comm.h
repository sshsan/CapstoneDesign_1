#ifndef COMM_H
#define COMM_H

#include <Arduino.h>

// 초기화
void initComm();

// 기본 알림
void sendAlert(float score);

// 상세 데이터 전송
void sendDetailedResult(float score);

// 이미지 전송 (추후)
void sendImage(uint8_t* img, int size);

#endif