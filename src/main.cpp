#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "base64.hpp"
#include "NTPClient.h"
#include <WiFiUdp.h>

#define ss 0
#define rst 15
#define dio0 7

// WiFi credentials

/******PLANTA PILOTO******/
const char *ssid = "IOT";
const char *password = "6GLHBT4gre2j2QJ2";
/*-----------------------*/
/******HOTSPOT PABLO******/
// const char *ssid = "iPhone de Pablo";
// const char *password = "ensipensi";
/*-----------------------*/

// MQTT broker
const char *mqtt_server = "broker.emqx.io";
WiFiClient espClient;
PubSubClient client(espClient);

// Buffers
unsigned char base64_text[128];
unsigned char decoded_text[128];
char char_topic[64];

// Time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

// Strings for message processing
String checksum, equipo, mensaje, final_message, topic;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void mqtt_reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  LoRa.setPins(ss, rst, dio0);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa initialization failed");
    while (true);
  }

  Serial.println("LoRa receiver started");

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  timeClient.begin();
  timeClient.setTimeOffset(3600);
}

void loop() {
  int packetSize = LoRa.parsePacket();
  timeClient.update();

  if (packetSize) {
    int index = 0;
    while (LoRa.available() && index < sizeof(base64_text) - 1) {
      base64_text[index++] = (unsigned char)LoRa.read();
    }
    base64_text[index] = '\0';

    int decoded_length = decode_base64(base64_text, decoded_text);
    decoded_text[decoded_length] = '\0';
    String payload = String((char *)decoded_text);

    int slashIndex = payload.indexOf('/');
    int hashIndex = payload.indexOf('#');

    checksum = payload.substring(0, slashIndex);
    equipo = payload.substring(slashIndex + 1, hashIndex);
    mensaje = payload.substring(hashIndex + 1);

    time_t epochTime = timeClient.getEpochTime();
    struct tm *ptm = gmtime((time_t *)&epochTime);
    String formattedTime = timeClient.getFormattedTime();
    String currentDate = String(ptm->tm_mday) + "-" + String(ptm->tm_mon + 1) + "-" + String(ptm->tm_year + 1900) + " " + formattedTime + ",";

    final_message = currentDate + mensaje;
    topic = "Fabiano/" + equipo + "/t/RecSi";
    topic.toCharArray(char_topic, sizeof(char_topic));

    if (checksum == "PlantaPiloto") {
      if (!client.connected()) {
        mqtt_reconnect();
      }
      client.loop();
      client.publish(char_topic, final_message.c_str());
      Serial.print("Published to ");
      Serial.print(char_topic);
      Serial.print(": ");
      Serial.println(final_message);
      delay(500);
    }
  }
}
