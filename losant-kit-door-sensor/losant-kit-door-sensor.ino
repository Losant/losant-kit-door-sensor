/**
 * Firmware for the Losant IoT Door Sensor Kit.
 *
 * Visit https://www.losant.io/kit for full instructions.
 *
 * Copyright (c) 2016 Losant IoT. All rights reserved.
 * https://www.losant.com
 */

#include <ESP8266WiFi.h>
#include <Losant.h>

// WiFi credentials.
const char* WIFI_SSID = "my-wifi-name";
const char* WIFI_PASS = "my-wifi-pass";

// Losant credentials.
const char* LOSANT_DEVICE_ID = "my-device-id";
const char* LOSANT_ACCESS_KEY = "my-access-key";
const char* LOSANT_ACCESS_SECRET = "my-access-secret";

#define DOOR_SENSOR_PIN 5

volatile bool doorStateChanged = false;

WiFiClientSecure wifiClient;

LosantDevice device(LOSANT_DEVICE_ID);

/**
 * Connects to WiFi and then to the Losant platform.
 */
void connect() {

  // Connect to Wifi.
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println();
  Serial.print("Connecting to Losant...");

  device.connectSecure(wifiClient, LOSANT_ACCESS_KEY, LOSANT_ACCESS_SECRET);

  unsigned long connectionStart = millis();
  while(!device.connected()) {
    delay(500);
    Serial.print(".");

    // If we can't connect after 5 seconds, restart the board.
    if(millis() - connectionStart > 5000) {
      Serial.println("Failed to connect to Losant, restarting board.");
      ESP.restart();
    }
  }

  Serial.println("Connected!");
  Serial.println("This device is now ready for use!");
}

/**
 * Called whenever the door sensor interrupt is triggered.
 */
void doorStateChangedHandler() {
  doorStateChanged = true;
}

void setup() {
  Serial.begin(115200);
  while(!Serial) { }
  
  pinMode(DOOR_SENSOR_PIN, INPUT);

  // Attach an interrupt to the door sensor input so we are
  // notified when it changes.
  attachInterrupt(digitalPinToInterrupt(DOOR_SENSOR_PIN), doorStateChangedHandler, CHANGE);

  connect();

  Serial.println();
  Serial.println("Running Door Sensor Firmware.");
}

/**
 * Reports state to Losant whenever the door is
 * opened or closed.
 */
void reportState(bool isOpen) {
   // Build the JSON payload and send to Losant.
  // { "open" : <bool> }
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["open"] = isOpen ? 1 : 0;
  device.sendState(root);

  Serial.println(isOpen ? "Door opened." : "Door closed.");
}

void loop() {
  
  bool toReconnect = false;

  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("Disconnected from WiFi");
    toReconnect = true;
  }

  if(!device.connected()) {
    Serial.println("Disconnected from MQTT");
    Serial.println(device.mqttClient.state());
    toReconnect = true;
  }

  if(toReconnect) {
    connect();
  }

  if(doorStateChanged) {
    bool value = digitalRead(DOOR_SENSOR_PIN);
    reportState(value == 0 ? true : false);
    doorStateChanged = false;
  }

  device.loop();
  
  delay(200);
}

