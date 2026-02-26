#### Dual MPU6050 data WS streaming 

for bmi160
```cpp
#include <Wire.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <BMI160Gen.h>  // https://github.com/hanyazou/BMI160-Arduino

// ==== I2C Configuration ====
#define I2C_SDA 14
#define I2C_SCL 15
#define BMI160_I2C_ADDR 0x69  // Default BMI160 I2C address (can be 0x68 if SDO=LOW)

// ==== WiFi Config ====
const char *ssid = "SLT_FIBRE";
const char *password = "aa8888aa";

// ==== WebSocket Server ====
WebSocketsServer webSocket = WebSocketsServer(81);

// ==== Sampling Interval ====
const int SAMPLE_INTERVAL_MS = 100; // 100 ms

// ==== Function Declarations ====
void setup_bmi160();
void connectToWifi();
void serialPlotter(int ax, int ay, int az, int gx, int gy, int gz);
void webSocketPlotter(int ax, int ay, int az, int gx, int gy, int gz);
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);

// ==== Setup ====
void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize I2C
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000); // Faster I2C for better performance

  // Initialize BMI160
  setup_bmi160();

  // Connect to WiFi
  connectToWifi();
}

// ==== Main Loop ====
void loop() {
  webSocket.loop();

  static unsigned long prevMillis = 0;
  if (millis() - prevMillis >= SAMPLE_INTERVAL_MS) {
    prevMillis = millis();

    // Read sensor data
    int gx, gy, gz;  // Gyroscope
    int ax, ay, az;  // Accelerometer

    BMI160.readMotionSensor(ax, ay, az, gx, gy, gz);

    serialPlotter(ax, ay, az, gx, gy, gz);
    webSocketPlotter(ax, ay, az, gx, gy, gz);
  }
}

// ==== Initialize BMI160 ====
void setup_bmi160() {
  if (!BMI160.begin(BMI160GenClass::I2C_MODE, BMI160_I2C_ADDR)) {
    Serial.println("❌ BMI160 initialization failed! Check wiring and address.");
    while (1);
  }

  Serial.println("✅ BMI160 initialized successfully!");

  // Configure sensor range
  BMI160.setAccelerometerRange(2);  // ±2g
  BMI160.setGyroRange(250);         // ±250°/s

  Serial.println("Accelerometer: ±2g, Gyroscope: ±250°/s configured.");
  delay(100);
}

// ==== WiFi Connection ====
void connectToWifi() {
  Serial.printf("Connecting to WiFi: %s", ssid);
  WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();
  const unsigned long timeout = 15000; // 15s timeout

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Start WebSocket Server
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    Serial.print("🌐 WebSocket Server running at: ws://");
    Serial.print(WiFi.localIP());
    Serial.println(":81/");
  } else {
    Serial.println("\n❌ Failed to connect to WiFi. Restarting...");
    delay(3000);
    ESP.restart();
  }
}

// ==== WebSocket Event Handler ====
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.printf("🔗 Client [%u] connected\n", num);
      break;

    case WStype_DISCONNECTED:
      Serial.printf("❌ Client [%u] disconnected\n", num);
      break;

    case WStype_TEXT:
      Serial.printf("📩 Received from [%u]: %s\n", num, payload);
      break;
  }
}

// ==== Serial Plotter Output ====
void serialPlotter(int ax, int ay, int az, int gx, int gy, int gz) {
  // Convert raw to readable units
  float accScale = 16384.0; // for ±2g
  float gyroScale = 131.0;  // for ±250°/s

  float accelX = ax / accScale;
  float accelY = ay / accScale;
  float accelZ = az / accScale;

  float gyroX = gx / gyroScale;
  float gyroY = gy / gyroScale;
  float gyroZ = gz / gyroScale;

  // Print in Serial Plotter format
  Serial.print("AX:"); Serial.print(accelX); Serial.print("\t");
  Serial.print("AY:"); Serial.print(accelY); Serial.print("\t");
  Serial.print("AZ:"); Serial.print(accelZ); Serial.print("\t");
  Serial.print("AALL:"); Serial.print(sqrt(accelX * accelX + accelY * accelY + accelZ * accelZ) - 1); Serial.print("\t");
  Serial.print("GX:"); Serial.print(gyroX); Serial.print("\t");
  Serial.print("GY:"); Serial.print(gyroY); Serial.print("\t");
  Serial.print("GZ:"); Serial.print(gyroZ); Serial.print("\t");
  Serial.print("GALL:"); Serial.println(sqrt(gyroX * gyroX + gyroY * gyroY + gyroZ * gyroZ));
}

// ==== Send Data via WebSocket ====
void webSocketPlotter(int ax, int ay, int az, int gx, int gy, int gz) {
  String json = "{";
  json += "\"mpu\":{";
  json += "\"ax\":" + String(ax) + ",";
  json += "\"ay\":" + String(ay) + ",";
  json += "\"az\":" + String(az) + ",";
  json += "\"gx\":" + String(gx) + ",";
  json += "\"gy\":" + String(gy) + ",";
  json += "\"gz\":" + String(gz);
  json += "}}";

  webSocket.broadcastTXT(json);
}
```
