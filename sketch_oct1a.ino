#include <WiFi.h>
#include <Wire.h>
#include <MPU6050.h>
#include <WebSocketsServer.h>

// ==== WiFi Config ====
const char *ssid = "SLT_FIBRE";
const char *password = "aa8888aa";

// ==== I2C Pins for ESP32-CAM ====
#define I2C_SDA 15
#define I2C_SCL 14

// ==== Two MPU6050 objects (one at 0x68, one at 0x69) ====
MPU6050 mpu1(0x68);
MPU6050 mpu2(0x69);

// ==== WebSocket Server on port 81 ====
WebSocketsServer webSocket = WebSocketsServer(81);

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("ESP32-CAM IP Address: ");
  Serial.print("ws://");
  Serial.print(WiFi.localIP());
  Serial.println(":81/");

  // Init I2C
  Wire.begin(I2C_SDA, I2C_SCL);

  // Init MPU1 (0x68)
  mpu1.initialize();
  if (mpu1.testConnection()) {
    Serial.println("MPU6050 (0x68) connection successful");
  } else {
    Serial.println("MPU6050 (0x68) connection failed");
  }

  // Init MPU2 (0x69)
  mpu2.initialize();
  if (mpu2.testConnection()) {
    Serial.println("MPU6050 (0x69) connection successful");
  } else {
    Serial.println("MPU6050 (0x69) connection failed");
  }

  // Start WebSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  webSocket.loop();

  int16_t ax1, ay1, az1, gx1, gy1, gz1;
  int16_t ax2, ay2, az2, gx2, gy2, gz2;

  // Read from sensor 1 (0x68)
  mpu1.getMotion6(&ax1, &ay1, &az1, &gx1, &gy1, &gz1);

  // Read from sensor 2 (0x69)
  mpu2.getMotion6(&ax2, &ay2, &az2, &gx2, &gy2, &gz2);

  // Create JSON string with both sensors
  String json = "{";
  json += "\"mpu1\":{";
  json += "\"ax\":" + String(ax1) + ",";
  json += "\"ay\":" + String(ay1) + ",";
  json += "\"az\":" + String(az1) + ",";
  json += "\"gx\":" + String(gx1) + ",";
  json += "\"gy\":" + String(gy1) + ",";
  json += "\"gz\":" + String(gz1);
  json += "},";

  json += "\"mpu2\":{";
  json += "\"ax\":" + String(ax2) + ",";
  json += "\"ay\":" + String(ay2) + ",";
  json += "\"az\":" + String(az2) + ",";
  json += "\"gx\":" + String(gx2) + ",";
  json += "\"gy\":" + String(gy2) + ",";
  json += "\"gz\":" + String(gz2);
  json += "}}";

  // Send data to all WebSocket clients
  webSocket.broadcastTXT(json);

  delay(10); // ~10Hz update
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_CONNECTED) {
    Serial.printf("Client %u connected\n", num);
  } else if (type == WStype_DISCONNECTED) {
    Serial.printf("Client %u disconnected\n", num);
  }
}