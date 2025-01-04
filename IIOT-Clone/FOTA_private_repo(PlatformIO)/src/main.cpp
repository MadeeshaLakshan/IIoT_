/*
ESP32 FOTA Update Using GitHub Private repo

Author  : Madeesha Lakshan
Date    : 12/20/2024
Org     : SLT Digital Lab
Features : FOTA update using Github private repo
           Save wifi crentials to the Flash(littlefs)
           Revert back to the previous version using external triggering
           save certification files to the flash(littlefs)
Remarks  : The certification is valid until 2038. Please verify the `cert.h` file to establish the connection between the ESP32 and GitHub.
           Generate your personal access token(fine-grained)
           First time you compile the code uncomment saveCredentials and saveRootCACertificate in the setup function
           Enter your credetials,links and PAT to the config.h file 
           Ensure the JSON and binary (.bin) files are stored in a private repository and can be accessed by copying and pasting the URLs into your browser.
           
*/
#include<Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "cert.h"
#include "config.h" 

#include "esp_partition.h" 
#include "esp_ota_ops.h"   

unsigned long previousMillis = 0;  // will store last time update was checked
const long interval = 5000;        // interval at which to check for updates (milliseconds)
String newFirmwareURL = "";        // Variable to store new firmware URL

// Function declarations
void initFileSystem();
void saveCredentials(const char* ssid, const char* password);
bool loadCredentials(String &ssid, String &password);
void connectToWiFi(const String& ssid, const String& password);
void printPartitionInfo();
void rollbackToPreviousFirmware();
void checkForUpdate();
void firmwareUpdate();
int FirmwareVersionCheck();
void saveRootCACertificate(const char* rootCA);
bool loadRootCACertificate(String &rootCA);

void setup() {
    Serial.begin(115200);
    pinMode(ROLLBACK_PIN, INPUT_PULLUP);  // Set up the rollback pin as an input with an internal pull-up resistor
    initFileSystem();
    printPartitionInfo();

    // Save the root CA certificate to LittleFS
    //saveRootCACertificate(rootCACertificate);

    // Save WiFi credentials
    //saveCredentials(ssid, wifiPassword);
    String ssid, password, rootCA;
    // Load WiFi credentials and connect to WiFi
    if (loadCredentials(ssid, password)) {
        Serial.println("Credentials loaded, attempting to connect");
        connectToWiFi(ssid,password);
    } else {
        Serial.println("Failed to load WiFi credentials");
    }

    // Load Root CA certificate
    if (loadRootCACertificate(rootCA)) {
        Serial.println("Root CA loaded successfully");
    } else {
        Serial.println("Failed to load Root CA");
    }

    // Uncomment the following line if you want to print partition info at startup
    // printPartitionInfo();  // Print partition info at startup
}



void loop() {
    unsigned long currentMillis = millis();
    if (digitalRead(ROLLBACK_PIN) == LOW) {  // Check if the rollback pin is triggered (assuming active low)
        Serial.println("Rollback pin triggered. Rolling back to previous firmware.");
        rollbackToPreviousFirmware();
    }

    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        checkForUpdate();
    }
}

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

void printPartitionInfo() {
    const esp_partition_t* running = esp_ota_get_running_partition();
    const esp_partition_t* next = esp_ota_get_next_update_partition(NULL);
    const esp_partition_t* previous = esp_ota_get_last_invalid_partition();

    Serial.println("Running Partition:");
    Serial.printf("Type: %d, Subtype: %d, Address: 0x%08x, Size: 0x%08x, Label: %s\n", 
                  running->type, running->subtype, running->address, running->size, running->label);

    Serial.println("Next Partition:");
    Serial.printf("Type: %d, Subtype: %d, Address: 0x%08x, Size: 0x%08x, Label: %s\n", 
                  next->type, next->subtype, next->address, next->size, next->label);

    Serial.println("Previous Invalid Partition:");
    if (previous != NULL) {
        Serial.printf("Type: %d, Subtype: %d, Address: 0x%08x, Size: 0x%08x, Label: %s\n", 
                      previous->type, previous->subtype, previous->address, previous->size, previous->label);
    } else {
        Serial.println("None");
    }
}

void rollbackToPreviousFirmware() {
    const esp_partition_t* running = esp_ota_get_running_partition();
    const esp_partition_t* next = esp_ota_get_next_update_partition(NULL);
    const esp_partition_t* previous = (running == esp_ota_get_boot_partition()) ? next : esp_ota_get_boot_partition();

    Serial.println("Running Partition:");
    Serial.printf("Type: %d, Subtype: %d, Address: 0x%08x, Size: 0x%08x, Label: %s\n", 
                  running->type, running->subtype, running->address, running->size, running->label);

    if (previous != NULL && previous != running) {
        Serial.println("Setting boot partition to the previous firmware.");
        esp_err_t err = esp_ota_set_boot_partition(previous);
        if (err != ESP_OK) {
            Serial.printf("Failed to set boot partition: %s\n", esp_err_to_name(err));
        } else {
            Serial.println("Boot partition set successfully. Rebooting...");
            ESP.restart();
        }
    } else {
        Serial.println("No valid previous partition found or already running the previous firmware.");
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
    WiFiClientSecure* client = new WiFiClientSecure;

    if (client) {
        String rootCA;
        if (loadRootCACertificate(rootCA)) {
            client->setCACert(rootCA.c_str());
        } else {
            Serial.println("Using built-in root CA");
            client->setCACert(rootCACertificate);  // Fallback to the built-in root CA
        }

        HTTPClient https;

        if (https.begin(*client, newFirmwareURL)) { // Begin connection with firmware URL
            https.addHeader("Authorization", "token " + String(GITHUB_TOKEN)); // Add the authorization header

            Serial.println("Starting firmware download...");
            int httpCode = https.GET(); // Perform GET request to download firmware binary

            if (httpCode == HTTP_CODE_OK) {
                // Stream the firmware directly to the update process
                WiFiClient* updateClient = https.getStreamPtr();
                size_t contentLength = https.getSize();

                if (contentLength > 0) {
                    Serial.printf("Firmware size: %d bytes\n", contentLength);

                    // Start the OTA update
                    if (Update.begin(contentLength)) {
                        size_t written = Update.writeStream(*updateClient);
                        if (written == contentLength) {
                            Serial.println("Firmware update successfully written!");
                        } else {
                            Serial.printf("Firmware update failed! Written: %d of %d bytes\n", written, contentLength);
                        }

                        // End the update process
                        if (Update.end()) {
                            if (Update.isFinished()) {
                                Serial.println("Update completed successfully. Rebooting...");
                                ESP.restart();
                            } else {
                                Serial.println("Update not finished. Something went wrong!");
                            }
                        } else {
                            Serial.printf("Update failed: %s\n", Update.errorString());
                        }
                    } else {
                        Serial.printf("Not enough space for OTA update: %d bytes needed\n", contentLength);
                    }
                } else {
                    Serial.println("Content length invalid or zero.");
                }
            } else {
                Serial.printf("Firmware download failed, HTTP code: %d\n", httpCode);
            }

            https.end(); // End the HTTP connection
        } else {
            Serial.println("HTTP client setup failed for firmware update.");
        }

        delete client; // Clean up
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
        String rootCA;
        if (loadRootCACertificate(rootCA)) {
            client->setCACert(rootCA.c_str());
        } else {
            Serial.println("Using built-in root CA");
            client->setCACert(rootCACertificate);  // Fallback to the built-in root CA
        }

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
        JsonDocument doc;
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

void saveRootCACertificate(const char* rootCA) {
    File file = LittleFS.open("/rootCA.pem", "w");
    if (!file) {
        Serial.println("Failed to open file for writing root CA");
        return;
    }
    file.print(rootCA);
    file.close();
    Serial.println("Root CA saved");
}

bool loadRootCACertificate(String &rootCA) {
    File file = LittleFS.open("/rootCA.pem", "r");
    if (!file) {
        Serial.println("Failed to open file for reading root CA");
        return false;
    }

    rootCA = file.readString();
    file.close();

    if (rootCA.length() == 0) {
        return false;
    }

    return true;
}