#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"

// ==========================
// Wi-Fi 설정
// ==========================
const char* WIFI_SSID = "sms";
const char* WIFI_PASS = "03080308";

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

// 최신 촬영 사진을 보관할 버퍼
uint8_t* latestPhotoBuffer = nullptr;
size_t latestPhotoLen = 0;

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
    config.frame_size   = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count     = 2;
    config.grab_mode    = CAMERA_GRAB_LATEST;
    config.fb_location  = CAMERA_FB_IN_PSRAM;
  } else {
    config.frame_size   = FRAMESIZE_VGA;
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

  sensor_t *s = esp_camera_sensor_get();
  if (s) {
    s->set_brightness(s, 0);
    s->set_contrast(s, 0);
    s->set_saturation(s, 0);
  }

  Serial.println("카메라 초기화 완료");
  return true;
}

bool capturePhotoToMemory() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("사진 촬영 실패");
    return false;
  }

  uint8_t* newBuffer = (uint8_t*)malloc(fb->len);
  if (!newBuffer) {
    Serial.println("메모리 할당 실패");
    esp_camera_fb_return(fb);
    return false;
  }

  memcpy(newBuffer, fb->buf, fb->len);

  if (latestPhotoBuffer != nullptr) {
    free(latestPhotoBuffer);
    latestPhotoBuffer = nullptr;
  }

  latestPhotoBuffer = newBuffer;
  latestPhotoLen = fb->len;

  Serial.printf("사진 촬영 완료: %u bytes\n", (unsigned int)latestPhotoLen);

  esp_camera_fb_return(fb);
  return true;
}

bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  delay(1000);

  Serial.println("Wi-Fi 연결 시작");
  Serial.print("SSID: ");
  Serial.println(WIFI_SSID);

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
    Serial.print("IP 주소: http://");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("Wi-Fi 연결 실패");
    Serial.println("1. SSID / 비밀번호 확인");
    Serial.println("2. 2.4GHz Wi-Fi 확인");
    Serial.println("3. 공유기 거리 확인");
    return false;
  }
}

void handleRoot() {
  String html;
  html += "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>XIAO Camera</title></head><body>";
  html += "<h1>XIAO ESP32S3 Camera</h1>";
  html += "<p><a href='/capture'>사진 촬영</a></p>";
  html += "<p><a href='/latest.jpg' target='_blank'>최신 사진 보기</a></p>";
  html += "<p>최신 사진 미리보기:</p>";
  html += "<img src='/latest.jpg?ts=" + String(millis()) + "' style='max-width:100%;height:auto;border:1px solid #ccc;' />";
  html += "</body></html>";

  server.send(200, "text/html; charset=utf-8", html);
}

void handleCapture() {
  if (!capturePhotoToMemory()) {
    server.send(500, "text/plain; charset=utf-8", "사진 촬영 실패");
    return;
  }

  String html;
  html += "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Capture Done</title></head><body>";
  html += "<h2>촬영 완료</h2>";
  html += "<p><a href='/latest.jpg' target='_blank'>최신 사진 열기</a></p>";
  html += "<p><a href='/'>메인으로</a></p>";
  html += "<img src='/latest.jpg?ts=" + String(millis()) + "' style='max-width:100%;height:auto;border:1px solid #ccc;' />";
  html += "</body></html>";

  server.send(200, "text/html; charset=utf-8", html);
}

void handleLatestJpg() {
  if (latestPhotoBuffer == nullptr || latestPhotoLen == 0) {
    server.send(404, "text/plain; charset=utf-8", "아직 촬영된 사진이 없습니다. 먼저 /capture 실행");
    return;
  }

  server.send_P(200, "image/jpeg", (const char*)latestPhotoBuffer, latestPhotoLen);
}

void setup() {
  Serial.begin(115200);
  delay(3000);

  Serial.println();
  Serial.println("======================================");
  Serial.println("XIAO ESP32S3 Sense Camera + Web");
  Serial.println("======================================");

  if (!initCamera()) {
    Serial.println("카메라 초기화 실패로 중단");
    while (true) delay(1000);
  }

  if (!connectWiFi()) {
    Serial.println("Wi-Fi 연결 실패로 웹서버를 시작하지 않음");
    while (true) delay(1000);
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/capture", HTTP_GET, handleCapture);
  server.on("/latest.jpg", HTTP_GET, handleLatestJpg);

  server.begin();
  Serial.println("웹서버 시작 완료");
  Serial.println("브라우저에서 위 주소로 접속하세요.");
  Serial.println("처음에는 /capture 를 눌러야 사진이 생성됩니다.");
}

void loop() {
  server.handleClient();
}