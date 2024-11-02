#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <EEPROM.h>

// Pins relays
int pinsNumbers[8] = {16, 5, 4, 0, 2, 14, 12, 13};
int pinsStates[8] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
String pinsNicknames[8] = {"relay1", "relay2", "relay3", "relay4", "relay5", "relay6", "relay7", "relay8"};

// WiFiManager
WiFiManager wifiManager;

// Web server
ESP8266WebServer server(80);


// Reads the StaticJsonDocument and toggles relays accordingly
void toggleRelays(StaticJsonDocument<256> newRelaysStates) {
  for(int i = 0; i < 8; i++) {
    if(newRelaysStates.containsKey(pinsNicknames[i])) {
      String relayState = newRelaysStates[pinsNicknames[i]];
      if(relayState == "ON") {
        pinsStates[i] =  HIGH;
        digitalWrite(pinsNumbers[i], HIGH);
        Serial.println("Turning on" + pinsNicknames[i]);
      } else if(relayState == "OFF") {
        pinsStates[i] = LOW;
        digitalWrite(pinsNumbers[i], LOW);
        Serial.println("Turning off" + pinsNicknames[i]);
      }
    }
  }
}


// Returns board informations such as IP address, SSID or relays states
void getBoardInfos() {
  StaticJsonDocument<256> boardInfosJson;

  boardInfosJson["ssid"] = WiFi.SSID();
  boardInfosJson["ip_address"] = WiFi.localIP().toString();
  boardInfosJson["hostname"] = WiFi.hostname();
  boardInfosJson["chip_id"] = String(ESP.getChipId(), HEX);
  boardInfosJson["board_type"] = "rhapsody_relays";

  // Add relays states
  JsonArray relays = boardInfosJson["relays"].to<JsonArray>();
  for(int i = 0; i < 8; i++) {
    if(pinsStates[i] == LOW) {
      relays.add("OFF");
    } else {
      relays.add("ON");
    }
  }

  String response;
  serializeJson(boardInfosJson, response);
  server.send(200, "application/json", response);
}


// Retrieves data sent by the client and changes
// relays states accordingly
void updateRelayState() {

  // If a request other than POST is sent to this route
  if(server.method() != HTTP_POST) {
    server.send(405, "application/json", "{\"error\"}:{\"Unauthorized method\"}");
    return;
  }

  String body = server.arg("plain");
  StaticJsonDocument<256> requestArgs;
  DeserializationError error = deserializeJson(requestArgs, body);

  // If there's an error in the JSON file
  if(error) {
    server.send(400, "application/json", "{\"error\": \"Invalid JSON body\"}");
    return;
  }

  // Toggles relays
  toggleRelays(requestArgs);

  // Provides new board's infos
  getBoardInfos();
}


void editHostname() {
  // If a request other than POST is sent to this route
  if(server.method() != HTTP_POST) {
    server.send(405, "application/json", "{\"error\":\"Unauthorized method\"}");
    return;
  }

  String body = server.arg("plain");
  StaticJsonDocument<256> requestArgs;
  DeserializationError error = deserializeJson(requestArgs, body);

  // If there's an error in the JSON file
  if(error) {
    server.send(400, "application/json", "\"error\":\"Invalid JSON body\"");
    return;
  }

  // Read the content of the h argument and changes the value
  if(requestArgs.containsKey("h")) {
    String newHostname = requestArgs["h"];
    WiFi.hostname(newHostname.c_str());
  }

  // Provides board's infos
  getBoardInfos();
}


void setup() {
  // Initialisation du port série
  Serial.begin(9600);
  delay(1000);
  Serial.println("Serial OK");

  // Initialisation du WiFI
  if(!wifiManager.autoConnect("rhapsody_relays_AP")) {
    Serial.println("Failed to connect or create an Access Point");
    ESP.reset();
    delay(1000);
  }
  WiFiClient wifiClient;
  Serial.println("Connected to WiFi!");
  Serial.print("IP address : ");
  Serial.println(WiFi.localIP());


  // Configuration des pins
  for(int i = 0; i < 8; i++) {
    pinMode(pinsNumbers[i], OUTPUT);
  }

  // Initialisation des relays
  for(int i = 0; i < 8; i++) {
    digitalWrite(pinsNumbers[i], HIGH);
  }

  // Récupération de l'état des relays
  HTTPClient http;
  String url = "http://switchs/states/" + String(ESP.getChipId(), HEX);
  Serial.print(url);
  http.begin(wifiClient, url);
  int httpCode = http.GET();
  if(httpCode > 0) {
    if(httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println("Received an answer from the server");
      Serial.println(payload);

      StaticJsonDocument<256> doc;

      DeserializationError error = deserializeJson(doc, payload);
      if(error) {
        Serial.print(F("Error while analyzing JSON body: "));
        Serial.println(error.f_str());
      } else {
        toggleRelays(doc);
      }
    } else {
      Serial.printf("Unexpected HTTP response code: %d\n", httpCode);
    }
  } else {
    Serial.printf("HTTP request failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();

  // Définition des routes du serveur
  Serial.println("Starting server");

  server.on("/infos", HTTP_GET, getBoardInfos);
  server.on("/control", HTTP_POST, updateRelayState);
  server.on("/hostname", HTTP_POST, editHostname);
  server.onNotFound([]() {
    server.send(404, "application/json", "{\"error\":\"Content not found\"}");
  });

  // Démarrage du serveur
  server.begin();

  Serial.println("Server started!");
}


void loop() {
  server.handleClient();
}