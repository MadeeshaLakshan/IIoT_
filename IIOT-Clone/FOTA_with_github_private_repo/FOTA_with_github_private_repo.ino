#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "cert.h"
#include "config.h"  // Include the config file

unsigned long previousMillis = 0;  // will store last time update was checked
const long interval = 5000;        // interval at which to check for updates (milliseconds)
String newFirmwareURL = "";        // Variable to store new firmware URL

// Function to initialize File System
void initFileSystem() {
    if (!LittleFS.begin()) {
        Serial.println("Failed to mount LittleFS, formatting...");
        if (LittleFS.format()) {
            Serial.println("LittleFS formatted successfully");
            if (!LittleFS.begin()) {
                Serial.println("Failed to mount LittleFS after formatting");
                return;
            }
        } else {
            Serial.println("Failed to format LittleFS");
            return;
        }
    }
}

// Function to save WiFi credentials
void saveCredentials(const char* ssid, const char* password) {
    File file = LittleFS.open("/wificredentials.txt", "w");
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }
    file.println(ssid);
    file.println(password);
    file.close();
    Serial.println("Credentials saved");
}

// Function to load WiFi credentials
bool loadCredentials(String &ssid, String &password) {
    File file = LittleFS.open("/wificredentials.txt", "r");
    if (!file) {
        Serial.println("Failed to open file for reading");
        return false;
    }

    ssid = file.readStringUntil('\n');
    password = file.readStringUntil('\n');
    file.close();

    ssid.trim();
    password.trim();

    if (ssid.length() == 0 || password.length() == 0) {
        return false;
    }

    return true;
}

// Function to connect to WiFi
void connectToWiFi(const String& ssid, const String& password) {
    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
        Serial.print(".");
        delay(100);
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Failed to connect to WiFi");
    } else {
        Serial.println("Connected to WiFi");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    }
}

void setup() {
    Serial.begin(115200);
    initFileSystem();

    // Save the defined credentials
    // Comment out this line after initial run to avoid overwriting credentials
    //saveCredentials(ssid, wifiPassword);

    // Load and connect to WiFi
    String ssid, password;
    if (loadCredentials(ssid, password)) {
        Serial.println("Credentials loaded, attempting to connect");
        connectToWiFi(ssid, password);
    } else {
        Serial.println("Failed to load WiFi credentials");
    }
}

void loop() {
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        checkForUpdate();
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

void firmwareUpdate() {
    WiFiClientSecure client;
    client.setCACert(rootCACertificate);
    httpUpdate.setAuthorization("token", GITHUB_TOKEN); // Set the authorization header

    t_httpUpdate_return ret = httpUpdate.update(client, newFirmwareURL);

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
            https.addHeader("Authorization", "token " + String(GITHUB_TOKEN)); // Add the authorization header
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
        // Parse JSON payload
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload);
        if (error) {
            Serial.print("JSON deserialization failed: ");
            Serial.println(error.f_str());
            return 0;
        }

        String newVersion = doc["version"].as<String>();
        newFirmwareURL = doc["bin_url"].as<String>();
        newVersion.trim();
        newFirmwareURL.trim();

        if (newVersion.equals(FirmwareVer)) {
            Serial.printf("\nDevice is already on the latest firmware version: %s\n", FirmwareVer.c_str());
            return 0;
        } else {
            Serial.println(newVersion);
            Serial.println("New Firmware Detected");
            return 1;
        }
    }
    return 0;
}
