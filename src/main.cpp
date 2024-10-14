#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>


// Pins relays
int pinsNumbers[8] = {5, 4, 0, 2, 14, 12, 13, 15};
int pinsStates[8] = {LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW};
String pinsNicknames[8] = {"relay1", "relay2", "relay3", "relay4", "relay5", "relay6", "relay7", "relay8"};

// WiFiManager
WiFiManager wifiManager;

// Server Web
ESP8266WebServer server(80);


// Retourne les informations de la carte et l'état des différents
// relais.
void getBoardInfos() {
  StaticJsonDocument<256> boardInfosJson;

  boardInfosJson["ssid"] = WiFi.SSID();
  boardInfosJson["ip_address"] = WiFi.localIP().toString();
  boardInfosJson["hostname"] = WiFi.hostname();
  boardInfosJson["chip_id"] = String(ESP.getChipId(), HEX);
  boardInfosJson["board_type"] = "rhapsody_relays";

  // Ajout de l'état des relays
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



// Récupère les données envoyées par le client et
// change l'état des relais en fonction
void updateRelayState() {

  // Si une requête autre que POST est envoyée sur cette route
  if(server.method() != HTTP_POST) {
    server.send(405, "application/json", "{\"error\"}:{\"Unauthorized method\"}");
    return;
  }

  String body = server.arg("plain");
  StaticJsonDocument<256> requestArgs;
  DeserializationError error = deserializeJson(requestArgs, body);

  if(error) {
    server.send(400, "application/json", "{\"error\": \"Invalid JSON body\"}");
    return;
  }

  for(int i; i < 8; i++) {
    if(requestArgs.containsKey(pinsNicknames[i])) {
      String relayState = requestArgs[pinsNicknames[i]];
      if(relayState == "ON") {
        pinsStates[i] = HIGH;
        digitalWrite(pinsNumbers[i], HIGH);
        Serial.println("Turning on" + pinsNicknames[i]);
      } else {
        pinsStates[i] = LOW;
        digitalWrite(pinsNumbers[i], LOW);
        Serial.println("Turning off" + pinsNicknames[i]);
      }
    }
    getBoardInfos();
  }
}


void editHostname() {
  // Si une requête autre que POST est envoyée sur cette route
  if(server.method() != HTTP_POST) {
    server.send(405, "application/json", "{\"error\":\"Unauthorized method\"}");
    return;
  }

  String body = server.arg("plain");
  StaticJsonDocument<256> requestArgs;
  DeserializationError error = deserializeJson(requestArgs, body);

  // Si il y a une erreur dans le fichier JSON empêchant sa déserialisation
  if(error) {
    server.send(400, "application/json", "\"error\":\"Invalid JSON body\"");
    return;
  }

  if(requestArgs.containsKey("hostname")) {
    String newHostname = requestArgs["hostname"];
    WiFi.hostname(newHostname.c_str());
  }

  getBoardInfos();
}



void setup() {
  // Initialisation du port série
  Serial.begin(9600);
  delay(1000);
  Serial.println("Port série OK");

  // Initialisation du WiFI
  if(!wifiManager.autoConnect("rhapsody_relays_AP")) {
    Serial.println("Echec de la connexion ou de la création du point d'accès");
    ESP.reset();
    delay(1000);
  }
  Serial.println("Connecté au WiFi !");
  Serial.print("Adresse IP : ");
  Serial.println(WiFi.localIP());

  // Configuration des pins
  for(int i = 0; i < 8; i++) {
    pinMode(pinsNumbers[i], OUTPUT);
  }

  // Définition des routes du serveur
  Serial.println("Démarrage du serveur");

  server.on("/infos", HTTP_GET, getBoardInfos);
  server.on("/control", HTTP_POST, updateRelayState);
  server.on("/hostname", HTTP_POST, editHostname);
  server.onNotFound([]() {
    server.send(404, "application/json", "{\"error\":\"Content not found\"}");
  });

  // Démarrage du serveur
  server.begin();

  Serial.println("Serveur démarré !");
}


void loop() {
  server.handleClient();
}