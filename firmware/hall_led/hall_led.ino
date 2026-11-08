// ESP32 + Hall Effect Sensor -> AWS IoT Core (MQTT over TLS)

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#include "secrets.h"   // WiFi creds, AWS endpoint, certs -- see secrets.h.example

// ---------- Hardware pins ----------
const int hallSensorPin = 13;

// ---------- Spin detection ----------
const unsigned long STABLE_MS = 30;   // pin must be stable this long before counting; lower to 5-10 if wheel spins are missed

volatile unsigned long lastEdgeTime = 0;
volatile int rawEdgeCount = 0;

int spinCount = 0;
int lastDisplayedCount = 0;
int lastPublishedCount = 0;
bool armed = true;                    // true once the pin has been stably HIGH

// ---------- Connection retry ----------
const unsigned long RECONNECT_INTERVAL_MS = 5000;
unsigned long lastReconnectAttempt = 0;

WiFiClientSecure secureClient;
PubSubClient mqttClient(secureClient);

void IRAM_ATTR onEdge() {
  rawEdgeCount++;
  lastEdgeTime = millis();            // timestamp only; counting happens in loop()
}

void setup() {
  Serial.begin(115200);

  pinMode(hallSensorPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(hallSensorPin), onEdge, CHANGE);

  connectWiFi();

  secureClient.setCACert(AMAZON_ROOT_CA);
  secureClient.setCertificate(DEVICE_CERT);
  secureClient.setPrivateKey(DEVICE_PRIVATE_KEY);

  mqttClient.setServer(awsEndpoint, awsPort);

  connectAWS();
}

void loop() {
  updateSpinCount();      // always runs first so spins are never starved by networking
  maintainConnection();   // non-blocking reconnect

  if (mqttClient.connected()) {
    mqttClient.loop();    // must be called regularly to maintain the MQTT connection

    if (spinCount - lastPublishedCount >= 5) {
      if (publishSpinCount()) {
        lastPublishedCount = spinCount;
      }
    }
  }

  if (spinCount != lastDisplayedCount) {
    Serial.print("spinCount: ");
    Serial.print(spinCount);
    Serial.print(" | rawEdges: ");
    Serial.println(rawEdgeCount);
    lastDisplayedCount = spinCount;
  }
}

void updateSpinCount() {
  unsigned long sinceEdge = millis() - lastEdgeTime;

  if (sinceEdge > STABLE_MS) {
    int level = digitalRead(hallSensorPin);

    if (level == LOW && armed) {
      spinCount++;
      armed = false;
    } else if (level == HIGH && !armed) {
      armed = true;
    }
  }
}

void connectWiFi() {
  Serial.println("Starting WiFi connection attempt...");
  WiFi.disconnect(true);
  delay(1000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int attempts = 0;
  int maxAttempts = 40;    // 40 x 500ms = 20 seconds total

  while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
    delay(500);
    Serial.print("Attempt ");
    Serial.print(attempts);
    Serial.print(" | status code: ");
    Serial.println(WiFi.status());
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal strength (RSSI): ");
    Serial.println(WiFi.RSSI());
  } else {
    Serial.print("Failed to connect after ");
    Serial.print(maxAttempts);
    Serial.print(" attempts. Final status code: ");
    Serial.println(WiFi.status());
  }
}

// Blocking connect, used once at boot
void connectAWS() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to AWS IoT...");
    if (mqttClient.connect(mqttClientId)) {
      Serial.println(" connected");
    } else {
      Serial.print(" failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retrying in 5s");
      delay(5000);
    }
  }
}

// Non-blocking reconnect, used from loop()
void maintainConnection() {
  unsigned long now = millis();
  if (now - lastReconnectAttempt < RECONNECT_INTERVAL_MS) return;

  if (WiFi.status() != WL_CONNECTED) {
    lastReconnectAttempt = now;
    Serial.println("WiFi lost, reconnecting...");
    WiFi.reconnect();
    return;
  }

  if (!mqttClient.connected()) {
    lastReconnectAttempt = now;
    Serial.print("Reconnecting to AWS IoT...");
    if (mqttClient.connect(mqttClientId)) {
      Serial.println(" connected");
    } else {
      Serial.print(" failed, rc=");
      Serial.println(mqttClient.state());
    }
  }
}

bool publishSpinCount() {
  char payload[64];
  snprintf(payload, sizeof(payload), "{\"spinCount\": %d}", spinCount);
  bool ok = mqttClient.publish(mqttTopic, payload);
  Serial.print(ok ? "Published: " : "Publish failed: ");
  Serial.println(payload);
  return ok;
}