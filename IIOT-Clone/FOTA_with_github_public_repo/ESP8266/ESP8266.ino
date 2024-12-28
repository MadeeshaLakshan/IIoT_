#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>
#include <CertStoreBearSSL.h>
#include <ArduinoJson.h>
#include "config.h"

BearSSL::CertStore certStore;
#include <time.h>

X509List cert(trustRoot);

// Function to set the time
void setClock() {
    // Set time via NTP, as required for x.509 validation
    configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    Serial.print("Waiting for NTP time sync: ");
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    Serial.println("Time synchronized");
}

// Function to check and update firmware
void FirmwareUpdate() {
    WiFiClientSecure client;
    client.setTrustAnchors(&cert);

    if (!client.connect(host, httpsPort)) {
        Serial.println("Connection failed");
        return;
    }

    client.print(String("GET ") + URL_fw_Version + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "User-Agent: BuildFailureDetectorESP8266\r\n" +
                 "Connection: close\r\n\r\n");

    // Wait for the HTTP response
    String response = "";
    while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
            break; // End of headers
        }
        response += line; // Read the response headers
    }

    // Read the body (the payload)
    String payload = "";
    while (client.available()) {
        payload += client.readStringUntil('\n');
    }

    // Check if payload is empty
    if (payload.length() == 0) {
        Serial.println("Error: Received empty payload");
        return;
    }

    // Skip everything before the first '{' to get the JSON body
    int startIndex = payload.indexOf('{');
    if (startIndex != -1) {
        payload = payload.substring(startIndex); // Extract JSON part

        // Parse JSON
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
            Serial.print("Failed to parse JSON: ");
            Serial.println(error.c_str());
            return;
        }

        // Extract version and bin_url for esp8266 from JSON
        String newVersion = doc["esp8266"]["version"].as<String>(); // Extract esp8266 version
        String binUrl = doc["esp8266"]["bin_url"].as<String>();    // Extract esp8266 bin_url

        // Debugging extracted values
        Serial.println("Extracted version: " + newVersion);
        Serial.println("Extracted bin_url: " + binUrl);

        // Check for the version update
        if (newVersion.equals(FirmwareVer)) {
            Serial.println("Device already on latest firmware version");
        } else {
            Serial.println("New firmware detected: " + newVersion);
            Serial.println("Bin URL: " + binUrl);

            ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
            t_httpUpdate_return ret = ESPhttpUpdate.update(client, binUrl);

            switch (ret) {
                case HTTP_UPDATE_FAILED:
                    Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
                    break;

                case HTTP_UPDATE_NO_UPDATES:
                    Serial.println("HTTP_UPDATE_NO_UPDATES");
                    break;

                case HTTP_UPDATE_OK:
                    Serial.println("HTTP_UPDATE_OK");
                    Serial.println("Firmware updated to version: " + newVersion);
                    break;
            }
        }
    } else {
        Serial.println("Error: JSON body not found");
    }
}

void connect_wifi() {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print("O");
    }
    Serial.println("Connected to WiFi");
}

unsigned long previousMillis_2 = 0;
unsigned long previousMillis = 0;        // will store last time LED was updated
const long interval = 6000;
const long mini_interval = 1000;

void repeatedCall() {
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis_2 >= mini_interval) {
        previousMillis_2 = currentMillis;
        connect_wifi();
        delay(500);
        FirmwareUpdate();
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);
    connect_wifi();
    setClock();
    delay(500);
    FirmwareUpdate();
}

void loop() {
    repeatedCall();
    delay(30000);
}
