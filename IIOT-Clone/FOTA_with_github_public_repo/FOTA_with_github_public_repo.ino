/*
ESp32 FOTA update using Github

Autor : Madeesha Lakshan
Date  : 12/17/2024
Org   : SLT Digital Lab 
Purpose : Perfome Firmware Over The Air update with github public repo 
Remarks : This certification only valid until 2038. please verfiy cert.h file to establish the connection between esp32 and github
          Enter your wifi credentials ssid and wifipassword
          Make sure to store json and bin file in a public repo and check the resourse can be accessible, by just copy and paste in your browser
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h> //v 7.2.1
#include "cert.h"
#include "config.h"  // Include the new configuration header file

int status = WL_IDLE_STATUS;

String newFwVersion = "";
String newFwBinURL = "";

unsigned long previousMillis = 0;
const long interval = 5000;

void setup() {
    Serial.begin(115200);
    Serial.print("Active Firmware Version: ");
    Serial.println(FirmwareVer);

    WiFi.begin(ssid, wifiPassword);

    Serial.print("Connecting to WiFi");
    int i = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        if (i++ == 10) {
            ESP.restart();
        }
    }
    Serial.println("\nConnected To WiFi");
}

void loop() {
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        checkForUpdate();
    }

    if (WiFi.status() != WL_CONNECTED) {
        reconnect();
    }
}

void checkForUpdate() {
    Serial.print("Checking for firmware updates... Current version: ");
    Serial.println(FirmwareVer);

    if (FirmwareVersionCheck()) {
        Serial.println("New firmware detected. Updating...");
        firmwareUpdate();
    } else {
        Serial.println("No new firmware available.");
    }
}

void reconnect() {
    int i = 0;
    status = WiFi.status();
    if (status != WL_CONNECTED) {
        WiFi.begin(ssid, wifiPassword);
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
            if (i++ == 10) {
                ESP.restart();
            }
        }
        Serial.println("\nConnected to AP");
    }
}

void firmwareUpdate() {
    WiFiClientSecure client;
    client.setCACert(rootCACertificate);
    t_httpUpdate_return ret = httpUpdate.update(client, newFwBinURL);

    switch (ret) {
        case HTTP_UPDATE_FAILED:
            Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
            break;

        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("HTTP_UPDATE_NO_UPDATES");
            break;

        case HTTP_UPDATE_OK:
            Serial.println("HTTP_UPDATE_OK");
            break;
    }
}

int FirmwareVersionCheck() {
    String payload;
    int httpCode;
    String FirmwareURL = URL_fw_JSON;
    FirmwareURL += "?";
    FirmwareURL += String(rand());

    WiFiClientSecure* client = new WiFiClientSecure;

    if (client) {
        client->setCACert(rootCACertificate);
        HTTPClient https;

        if (https.begin(*client, FirmwareURL)) {
            Serial.print("[HTTPS] GET...\n");
            delay(100);
            httpCode = https.GET();
            delay(100);
            if (httpCode == HTTP_CODE_OK) {
                payload = https.getString();
            } else {
                Serial.print("Error Occurred During Version Check: ");
                Serial.println(httpCode);
            }
            https.end();
        }
        delete client;
    }

    if (httpCode == HTTP_CODE_OK) {
        payload.trim();
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (error) {
            Serial.print("JSON deserialization failed: ");
            Serial.println(error.c_str());
            return 0;
        }

        newFwVersion = doc["version"].as<String>();
        newFwBinURL = doc["bin_url"].as<String>();

        if (newFwVersion.equals(FirmwareVer)) {
            Serial.printf("\nDevice is already on the latest firmware version: %s\n", FirmwareVer.c_str());
            return 0;
        } else {
            Serial.println(newFwVersion);
            Serial.println("New Firmware Detected");
            return 1;
        }
    }
    return 0;
}
