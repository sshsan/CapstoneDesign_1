#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"

// ==========================
// Wi-Fi 설정
// ==========================
const char* WIFI_SSID = "Hotspot1394";
const char* WIFI_PASS = "tjdgh125782@";

// ==========================
// 사용자 연결 핀
// D0  = GPIO1  -> 버튼
// D7  = GPIO44 -> RGB R
// D8  = GPIO7  -> RGB G
// D9  = GPIO8  -> RGB B
// D10 = GPIO9  -> IR LED
// ==========================
#define BUTTON_PIN   1
#define RGB_R_PIN   44
#define RGB_G_PIN    7
#define RGB_B_PIN    8
#define IR_LED_PIN   9

// ==========================
// XIAO ESP32S3 Sense camera pin map
// ==========================
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  10
#define SIOD_GPIO_NUM  40
#define SIOC_GPIO_NUM  39

#define Y9_GPIO_NUM    48
#define Y8_GPIO_NUM    11
#define Y7_GPIO_NUM    12
#define Y6_GPIO_NUM    14
#define Y5_GPIO_NUM    16
#define Y4_GPIO_NUM    18
#define Y3_GPIO_NUM    17
#define Y2_GPIO_NUM    15
#define VSYNC_GPIO_NUM 38
#define HREF_GPIO_NUM  47
#define PCLK_GPIO_NUM  13

WebServer server(80);

// 최신 사진 1장만 RAM에 저장
uint8_t* latestJpg = nullptr;
size_t latestJpgLen = 0;

// 버튼 디바운스
bool lastRawButton = HIGH;
bool buttonHandled = false;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

void setRgb(bool r, bool g, bool b) {
  digitalWrite(RGB_R_PIN, r ? HIGH : LOW);
  digitalWrite(RGB_G_PIN, g ? HIGH : LOW);
  digitalWrite(RGB_B_PIN, b ? HIGH : LOW);
}

void allOff() {
  setRgb(false, false, false);
  digitalWrite(IR_LED_PIN, LOW);
}

bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;

  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;

  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;

  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size   = FRAMESIZE_QVGA;   // 320x240
    config.jpeg_quality = 12;
    config.fb_count     = 2;
    config.grab_mode    = CAMERA_GRAB_LATEST;
    config.fb_location  = CAMERA_FB_IN_PSRAM;
  } else {
    config.frame_size   = FRAMESIZE_QQVGA;  // 160x120
    config.jpeg_quality = 15;
    config.fb_count     = 1;
    config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location  = CAMERA_FB_IN_DRAM;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("카메라 초기화 실패: 0x%x\n", err);
    return false;
  }

  sensor_t* s = esp_camera_sensor_get();
  if (s) {
    s->set_brightness(s, 0);
    s->set_contrast(s, 0);
    s->set_saturation(s, 0);
  }

  Serial.println("카메라 초기화 완료");
  return true;
}

bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  delay(1000);

  Serial.println("Wi-Fi 연결 시작");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int retry = 0;
  const int maxRetry = 40;

  while (WiFi.status() != WL_CONNECTED && retry < maxRetry) {
    delay(500);
    Serial.print(".");
    retry++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi 연결 완료");
    Serial.print("메인 페이지: http://");
    Serial.println(WiFi.localIP());
    Serial.print("최신 사진: http://");
    Serial.print(WiFi.localIP());
    Serial.println("/latest.jpg");
    return true;
  }

  Serial.println("Wi-Fi 연결 실패");
  return false;
}

bool capturePhotoToMemory() {
  Serial.println("사진 촬영 시작");

  setRgb(true, false, false);   // 빨강
  digitalWrite(IR_LED_PIN, HIGH);
  delay(100);

  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("사진 촬영 실패");
    digitalWrite(IR_LED_PIN, LOW);
    setRgb(true, false, true);  // 보라
    delay(300);
    allOff();
    return false;
  }

  if (latestJpg) {
    free(latestJpg);
    latestJpg = nullptr;
    latestJpgLen = 0;
  }

  latestJpg = (uint8_t*)malloc(fb->len);
  if (!latestJpg) {
    Serial.println("메모리 할당 실패");
    esp_camera_fb_return(fb);
    digitalWrite(IR_LED_PIN, LOW);
    setRgb(true, false, true);
    delay(300);
    allOff();
    return false;
  }

  memcpy(latestJpg, fb->buf, fb->len);
  latestJpgLen = fb->len;

  Serial.println("사진 촬영 성공");
  Serial.printf("해상도: %d x %d\n", fb->width, fb->height);
  Serial.printf("크기: %u bytes\n", fb->len);

  esp_camera_fb_return(fb);

  digitalWrite(IR_LED_PIN, LOW);
  setRgb(false, true, false);   // 초록
  delay(300);
  allOff();

  return true;
}

void handleRoot() {
  String html;
  html += "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>XIAO ESP32S3 Camera</title></head><body>";
  html += "<h1>XIAO ESP32S3 Sense Camera</h1>";
  html += "<p>D0 버튼을 누르면 사진을 촬영합니다.</p>";
  html += "<p><a href='/capture'>브라우저에서 강제 촬영</a></p>";
  html += "<p><a href='/latest.jpg' target='_blank'>최신 사진 보기</a></p>";

  if (latestJpgLen > 0) {
    html += "<p>최신 사진 미리보기:</p>";
    html += "<img src='/latest.jpg?ts=" + String(millis()) + "' style='max-width:100%;height:auto;border:1px solid #ccc;' />";
  } else {
    html += "<p>아직 촬영된 사진이 없습니다.</p>";
  }

  html += "</body></html>";
  server.send(200, "text/html; charset=utf-8", html);
}

void handleCapture() {
  if (!capturePhotoToMemory()) {
    server.send(500, "text/plain; charset=utf-8", "촬영 실패");
    return;
  }

  server.sendHeader("Location", "/");
  server.send(303);
}

void handleLatestJpg() {
  if (!latestJpg || latestJpgLen == 0) {
    server.send(404, "text/plain; charset=utf-8", "사진 없음");
    return;
  }

  server.send_P(200, "image/jpeg", (const char*)latestJpg, latestJpgLen);
}

void checkButtonAndCapture() {
  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastRawButton) {
    lastDebounceTime = millis();
    lastRawButton = reading;
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // INPUT_PULLUP 기준: 평소 HIGH, 누르면 LOW
    if (reading == LOW && !buttonHandled) {
      Serial.println("버튼 눌림 감지");
      capturePhotoToMemory();
      buttonHandled = true;
    }

    if (reading == HIGH) {
      buttonHandled = false;
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(3000);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(RGB_R_PIN, OUTPUT);
  pinMode(RGB_G_PIN, OUTPUT);
  pinMode(RGB_B_PIN, OUTPUT);
  pinMode(IR_LED_PIN, OUTPUT);
  allOff();

  Serial.println();
  Serial.println("======================================");
  Serial.println("XIAO ESP32S3 Sense Button Camera");
  Serial.println("======================================");

  if (!initCamera()) {
    Serial.println("카메라 초기화 실패로 중단");
    while (true) delay(1000);
  }

  if (!connectWiFi()) {
    Serial.println("Wi-Fi 연결 실패로 중단");
    while (true) delay(1000);
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/capture", HTTP_GET, handleCapture);
  server.on("/latest.jpg", HTTP_GET, handleLatestJpg);

  server.begin();
  Serial.println("웹서버 시작");
  Serial.println("D0 버튼을 누르면 사진을 촬영합니다.");
}

void loop() {
  server.handleClient();
  checkButtonAndCapture();
}