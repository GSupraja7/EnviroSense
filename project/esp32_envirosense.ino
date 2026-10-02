/*
  ============================================================
  EnviroSense - IoT Environment Monitoring System
  ============================================================
  Board:    ESP32 (or ESP8266 with minor tweaks)
  Sensor:   DHT22 (Temperature + Humidity)
  Backend:  Firebase Realtime Database (REST API over HTTPS)

  WHAT THIS CODE DOES:
  1. Connects the ESP32 to Wi-Fi
  2. Reads temperature & humidity from the DHT22 every N seconds
  3. Sends the reading to Firebase via a simple HTTP PUT request
  4. Locally checks thresholds and flags an "alert" state
  5. Retries automatically on Wi-Fi/connection failure

  LIBRARIES NEEDED (Install via Arduino IDE Library Manager):
  - "DHT sensor library" by Adafruit
  - "Adafruit Unified Sensor"
  - Built-in: WiFi.h, HTTPClient.h (ESP32 core)

  WIRING:
  DHT22 PIN 1 (VCC)  -> 3.3V
  DHT22 PIN 2 (DATA) -> GPIO 4  (use a 10k pull-up resistor to VCC)
  DHT22 PIN 4 (GND)  -> GND
  ============================================================
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

// ---------------- CONFIG ----------------
const char* WIFI_SSID     = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// Firebase Realtime Database REST endpoint
// Format: https://<project-id>-default-rtdb.firebaseio.com/readings.json
const char* FIREBASE_HOST = "https://envirosense-XXXX-default-rtdb.firebaseio.com/readings.json";

#define DHTPIN   4
#define DHTTYPE  DHT22
DHT dht(DHTPIN, DHTTYPE);

// Alert thresholds - tune to your environment
const float TEMP_HIGH_THRESHOLD = 35.0;  // Celsius
const float HUMIDITY_HIGH_THRESHOLD = 80.0; // %

const unsigned long SEND_INTERVAL_MS = 10000; // send every 10 seconds
unsigned long lastSendTime = 0;

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  dht.begin();
  connectToWiFi();
}

void connectToWiFi() {
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected! IP address: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nFailed to connect. Will retry in loop().");
  }
}

// ---------------- MAIN LOOP ----------------
void loop() {
  // Reconnect Wi-Fi if dropped (important for interview talking point: resilience)
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }

  unsigned long now = millis();
  if (now - lastSendTime >= SEND_INTERVAL_MS) {
    lastSendTime = now;
    readAndSendData();
  }
}

void readAndSendData() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature(); // Celsius

  // DHT sensors occasionally return NaN - always validate before using
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor. Skipping this cycle.");
    return;
  }

  bool alert = (temperature > TEMP_HIGH_THRESHOLD) || (humidity > HUMIDITY_HIGH_THRESHOLD);

  Serial.printf("Temp: %.1f C | Humidity: %.1f %% | Alert: %s\n",
                temperature, humidity, alert ? "YES" : "no");

  sendToFirebase(temperature, humidity, alert);
}

void sendToFirebase(float temperature, float humidity, bool alert) {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(FIREBASE_HOST);
  http.addHeader("Content-Type", "application/json");

  // Using timestamp as the key means Firebase stores a history, not just the latest value
  unsigned long timestamp = millis(); // swap for real epoch time via NTP in production

  String payload = "{";
  payload += "\"temperature\":" + String(temperature, 1) + ",";
  payload += "\"humidity\":" + String(humidity, 1) + ",";
  payload += "\"alert\":" + String(alert ? "true" : "false") + ",";
  payload += "\"timestamp\":" + String(timestamp);
  payload += "}";

  int httpResponseCode = http.PUT(payload);

  if (httpResponseCode > 0) {
    Serial.println("Data sent successfully, response code: " + String(httpResponseCode));
  } else {
    Serial.println("Error sending data: " + http.errorToString(httpResponseCode));
  }

  http.end();
}
