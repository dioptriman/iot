#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <Arduino_JSON.h>

#define timeSeconds 1

// Replace with your network credentials
const char* ssid = "YourSSID";
const char* password = "YourPassword";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

JSONVar readings;

const int PIN_TO_SENSOR = 16;
const int DOOR_SENSOR_PIN = 18;
const int relay = 36;

int doorState;
bool relayState = false; // Initialize relay state

String getJsonData() {
  readings["pir_sensor"] = String(digitalRead(PIN_TO_SENSOR));
  readings["door_sensor"] = String(digitalRead(DOOR_SENSOR_PIN));
  String jsonString = JSON.stringify(readings);
  return jsonString;
}

void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

void notifyClients(String jsonData) {
  ws.textAll(jsonData);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    char message[len + 1];
    memcpy(message, data, len);
    message[len] = '\0';
    if (strcmp(message, "toggle") == 0) {
      relayState = !relayState;
      digitalWrite(relay, relayState); // Toggle relay state
      notifyClients(getJsonData()); // Notify clients with updated sensor readings
    } else if (strcmp(message, "getReadings") == 0) {
      String jsonData = getJsonData();
      Serial.println(jsonData);
      notifyClients(jsonData);
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

String processor(const String& var) {
  Serial.println(var);
  if (var == "STATE") {
    if (relayState) {
      return "ON";
    } else {
      return "OFF";
    }
  }
  return String();
}

void setup() {
  Serial.begin(115200);
  pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);
  pinMode(PIN_TO_SENSOR, INPUT);
  pinMode(relay, OUTPUT);
  digitalWrite(relay, LOW);

  initWiFi();
  initSPIFFS();
  initWebSocket();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", "text/html", processor);
  });

  server.serveStatic("/", SPIFFS, "/");

  server.begin();
}

void loop() {
  doorState = digitalRead(DOOR_SENSOR_PIN);
  int pirState = digitalRead(PIN_TO_SENSOR);

  // Control relay based on sensor states
  if (doorState == HIGH && pirState == LOW) {
    relayState = true;
  } else if (doorState == LOW && pirState == HIGH) {
    relayState = false;
  }

  ws.cleanupClients(); // Cleanup WebSocket clients
  digitalWrite(relay, relayState);
}
