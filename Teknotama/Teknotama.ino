#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <Arduino_JSON.h>


#define timeSeconds 1


// Replace with your network credentials
const char* ssid = "Andika";
const char* password = "30032003";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

AsyncWebSocket ws("/ws");

// Json Variable to Hold Sensor Readings
JSONVar readings;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 30;

//PIR Sensor
const int PIN_TO_SENSOR = 16;

//Door Sensor
const int DOOR_SENSOR_PIN = 18;


//Relay
const int relay = 36;

//Door State
int doorState;

bool ledState = 0;



String getJsonData(){
  readings["pir_sensor"] = String(digitalRead(DOOR_SENSOR_PIN));
  readings["door_sensor"] = String(digitalRead(PIN_TO_SENSOR));
  String jsonString = JSON.stringify(readings);
  return jsonString;
}


// Initialize SPIFFS
void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

// Initialize WiFi
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

void notifyClients(String getJsonData) {
  ws.textAll(getJsonData);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    try {
      var myObj = JSON.parse(data);
      var keys = Object.keys(myObj);

      // Check for sensor data identifier (optional)
      if (strcmp((char*)data, "getReadings") == 0) {
        for (var i = 0; i < keys.length; i++) {
          var key = keys[i];
          document.getElementById(key).innerHTML = myObj[key];
        }
      } else {
        // Handle relay data (assuming a simple string like "1" or "0")
        console.log(data);
        var state;
        if (data == "1") {
          state = "ON";
        } else {
          state = "OFF";
        }
        document.getElementById('state').innerHTML = state;
      }
    } catch(error) {
      console.error("Error parsing data:", error);
      // Handle parsing error (optional)
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

String processor(const String& var){
  Serial.println(var);
  if(var == "STATE"){
    if (ledState){
      return "ON";
    }
    else{
      return "OFF";
    }
  }
  return String();
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);
  pinMode(PIN_TO_SENSOR, INPUT);
  pinMode(relay, OUTPUT);

  digitalWrite(relay, LOW);

  initWiFi();
  initSPIFFS();
  initWebSocket();

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.serveStatic("/", SPIFFS, "/");

  // Start server
  server.begin();
}

void loop() {
  doorState = digitalRead(DOOR_SENSOR_PIN);
  int pirState = digitalRead(PIN_TO_SENSOR);

  if((doorState == HIGH) and (pirState == LOW)){
    relayState = true;
    } 
    else if ((doorState == LOW) and (pirState == HIGH)){
     relayState = false;
    }

  ws.cleanupClients();

  digitalWrite(relay, relayState);

}
