#ifndef PINS_H
#define PINS_H

#include <Arduino.h>

static const int PIN_BUTTON  = D0;
static const int PIN_IR_LED  = D1;
static const int PIN_LED_R   = D2;
static const int PIN_LED_G   = D3;
static const int PIN_I2C_SDA = D4;
static const int PIN_I2C_SCL = D5;
static const int PIN_LED_B   = D6;

static const int PIN_USER_LED = 21;

// XIAO ESP32-S3 Sense camera
#define CAM_PIN_PWDN   -1
#define CAM_PIN_RESET  -1
#define CAM_PIN_XCLK   10
#define CAM_PIN_SIOD   40
#define CAM_PIN_SIOC   39

#define CAM_PIN_D7     48
#define CAM_PIN_D6     11
#define CAM_PIN_D5     12
#define CAM_PIN_D4     14
#define CAM_PIN_D3     16
#define CAM_PIN_D2     18
#define CAM_PIN_D1     17
#define CAM_PIN_D0     15
#define CAM_PIN_VSYNC  38
#define CAM_PIN_HREF   47
#define CAM_PIN_PCLK   13

#endif