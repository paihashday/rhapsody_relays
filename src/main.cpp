#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>


void setup() {
  // Initialisation du port série
  Serial.begin(9600);
  delay(1000);
  Serial.println("Port série OK");

  // Initialisation du WiFI
  WiFiManager wifiManager;
  if(!wifiManager.autoConnect("rhapsody_relays_AP")) {
    Serial.println("Echec de la connexion ou de la création du point d'accès");
    ESP.reset();
    delay(1000);
  }
  Serial.println("Connecté au WiFi !");
  Serial.print("Adresse IP : ");
  Serial.println(WiFi.localIP());

  
}


void loop() {
  Serial.println("Bonjour tout le monde !");
}