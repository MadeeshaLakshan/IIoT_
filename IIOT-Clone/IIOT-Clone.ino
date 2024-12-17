/*
ESp32 FOTA update using Github

Autor : Madeesha Lakshan
Date  : 12/17/2024
Org   : SLT Digital Lab 
Purpose : Perfome Firmware Over The Air update with github public repo 
Remarks : This certification only valid until 2038. please verfiy cert.h file to establish the connection between esp32 and github
          Enter your wifi credentials ssid and wifipassword
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include "cert.h"

const char* ssid = "SSID";
const char* wifiPassword = "PASSWORD";
int status = WL_IDLE_STATUS;

String FirmwareVer = "1.0.0";
#define URL_fw_Version "https://raw.githubusercontent.com/MadeeshaLakshan/ESP32_OTA_Github/refs/heads/main/version.txt"
#define URL_fw_Bin "https://raw.githubusercontent.com/MadeeshaLakshan/ESP32_OTA_Github/main/build/esp32.esp32.esp32/ESP32_FinalCodes_https.ino.bin"

unsigned long previousMillis = 0;  // will store last time update was checked
const long interval = 5000;        // interval at which to check for updates (milliseconds)

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
    t_httpUpdate_return ret = httpUpdate.update(client, URL_fw_Bin);

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
    String FirmwareURL = URL_fw_Version;
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
        if (payload.equals(FirmwareVer)) {
            Serial.printf("\nDevice is already on the latest firmware version: %s\n", FirmwareVer.c_str());
            return 0;
        } else {
            Serial.println(payload);
            Serial.println("New Firmware Detected");
            return 1;
        }
    }
    return 0;
}