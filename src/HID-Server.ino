#define ARDUHAL_LOG_LEVEL ARDUHAL_LOG_LEVEL_INFO
#include <Arduino.h>
#include <WiFi.h>
#include <esp32-hal-log.h>
#include "ArduinoWebsockets.h"
#include "BleMouse.h"

#define LED_PIN 8  // 定义LED连接的引脚为GPIO8

using namespace websockets;
String clientIp;
// ===========================
// Enter your WiFi credentials
// ===========================
const char *ssid = "Xiaomi13";
const char *password = "Bureaucrat1989";

WebsocketsServer server;
const byte maxClients = 1;
WebsocketsClient clients[maxClients];

BleMouse bleMouse = BleMouse();
#define BLE_HID_DELAY 500

void setup() {
  Serial.begin(115200);
  // set up mouse
  Serial.println("starting BLE work!");
  log_i("starting BLE work!");
  bleMouse.begin();

  // led
  pinMode(LED_PIN, OUTPUT);  // 设置GPIO8为输出

  // set up wifi
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    log_i("connenct wifi %s ...", ssid);
  }
  clientIp = WiFi.localIP().toString();
  log_i("wifi %s connected. local ip: %s", ssid, clientIp.c_str());
  // socket
  server.listen(80);
  log_i("is server live? %d", server.available());
}

void simClick(const WSString& message) {
  // 假设消息格式为 "click;x;y;"，其中click是按钮编号，x和y是坐标
  size_t firstIndex = message.find(";");
  size_t secondIndex = message.find(";", firstIndex + 1);
  
  int8_t clickBtn = 0;
  int16_t moveX = 0;
  int16_t moveY = 0;

  if (firstIndex != std::string::npos && secondIndex != std::string::npos) {
    // 截取点击按钮编号
    clickBtn = strtol(message.substr(0, firstIndex).c_str(), NULL, 10);
    // 截取x坐标
    moveX = strtol(message.substr(firstIndex + 1, secondIndex - firstIndex - 1).c_str(), NULL, 10);
    // 截取y坐标
    moveY = strtol(message.substr(secondIndex + 1).c_str(), NULL, 10);

    // 打印日志
    log_i("[message] -> click:%d, x:%d, y:%d.", clickBtn, moveX, moveY);
    if  (bleMouse.isConnected()) {
      // 鼠标移动
      bleMouse.move(moveX, moveY);
      delay(BLE_HID_DELAY);
      // 点击操作
      if (clickBtn > 0) {
        bleMouse.click(clickBtn);
        delay(BLE_HID_DELAY);
      }
    }
  } else {
    // 打印错误日志
    log_i("parse message %s error.", message.c_str());
  }
}

void blinkLed() {
  digitalWrite(LED_PIN, HIGH); 
  delay(100); 
  digitalWrite(LED_PIN, LOW);
  delay(50);
} 

int8_t getFreeClientIndex() { 
  for (byte i = 0; i < maxClients; i++) {
    if (!clients[i].available()) return i;
  }
  return -1;
}

void handleMessage(WebsocketsClient &client, WebsocketsMessage message) {
  blinkLed();
  if (message.isText()) {
    // extract data
    simClick(message.rawData());
    client.send("ok");
  } else {
    client.send("error: not text");
  }
}

void handleEvent(WebsocketsClient &client, WebsocketsEvent event, String data) {
  if (event == WebsocketsEvent::ConnectionClosed) {
    log_i("connection closed");
  }
}

void loop() {
  if (server.poll()) {
    int8_t freeIndex = getFreeClientIndex();
    if (freeIndex >= 0) {
      WebsocketsClient newClient = server.accept();
      log_i("accepted new websockets client at index %d", freeIndex);
      newClient.onMessage(handleMessage);
      newClient.onEvent(handleEvent);
      clients[freeIndex] = newClient;
    }
  }
  for (byte i = 0; i < maxClients; i++) {
    clients[i].poll();
  }
}