#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"

// ==========================
// Wi-Fi 설정
// ==========================
const char* WIFI_SSID = "Hotspot1394";
const char* WIFI_PASS = "tjdgh125782@";

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
    config.jpeg_quality = 12;               // 숫자 낮을수록 화질 좋음
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

  sensor_t *s = esp_camera_sensor_get();
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
    Serial.print("메인 페이지: http://");
    Serial.println(WiFi.localIP());
    Serial.print("스트림 주소: http://");
    Serial.print(WiFi.localIP());
    Serial.println("/stream");
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
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>XIAO ESP32S3 Live Stream</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      background: #f5f5f5;
      margin: 0;
      padding: 20px;
    }
    h1 {
      font-size: 22px;
    }
    img {
      max-width: 100%;
      height: auto;
      border: 2px solid #444;
      background: white;
    }
    .box {
      max-width: 700px;
      margin: 0 auto;
      background: white;
      padding: 20px;
      border-radius: 12px;
    }
    .btn {
      display: inline-block;
      margin-top: 12px;
      padding: 10px 16px;
      background: #007aff;
      color: white;
      text-decoration: none;
      border-radius: 8px;
    }
  </style>
</head>
<body>
  <div class="box">
    <h1>XIAO ESP32S3 실시간 영상</h1>
    <p>아래 영상이 계속 재생됩니다.</p>
    <img id="stream" src="/stream" alt="Live Stream">
    <br>
    <a class="btn" href="/jpg" target="_blank">현재 프레임 한 장 보기</a>
  </div>

  <script>
    const img = document.getElementById('stream');

    img.onerror = function () {
      console.log("스트림 끊김, 재연결 시도");
      setTimeout(() => {
        img.src = "/stream?t=" + new Date().getTime();
      }, 1000);
    };
  </script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html; charset=utf-8", html);
}

void handleJpg() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    server.send(500, "text/plain; charset=utf-8", "프레임 캡처 실패");
    return;
  }

  WiFiClient client = server.client();

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: image/jpeg");
  client.print("Content-Length: ");
  client.println(fb->len);
  client.println("Cache-Control: no-cache");
  client.println("Connection: close");
  client.println();

  client.write(fb->buf, fb->len);
  esp_camera_fb_return(fb);
}

void handleStream() {
  WiFiClient client = server.client();

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
  client.println("Cache-Control: no-cache");
  client.println("Pragma: no-cache");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Connection: close");
  client.println();

  Serial.println("스트림 시작");

  while (client.connected()) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("프레임 캡처 실패");
      delay(30);
      continue;
    }

    client.println("--frame");
    client.println("Content-Type: image/jpeg");
    client.print("Content-Length: ");
    client.println(fb->len);
    client.println();

    client.write(fb->buf, fb->len);
    client.println();

    esp_camera_fb_return(fb);

    if (!client.connected()) {
      break;
    }

    delay(30);
  }

  Serial.println("스트림 종료");
}

void setup() {
  Serial.begin(115200);
  delay(3000);

  Serial.println();
  Serial.println("======================================");
  Serial.println("XIAO ESP32S3 Sense Camera Live Stream");
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
  server.on("/jpg", HTTP_GET, handleJpg);
  server.on("/stream", HTTP_GET, handleStream);

  server.begin();
  Serial.println("웹서버 시작 완료");
  Serial.println("브라우저에서 메인 페이지 주소로 접속하세요.");
}

void loop() {
  server.handleClient();
}