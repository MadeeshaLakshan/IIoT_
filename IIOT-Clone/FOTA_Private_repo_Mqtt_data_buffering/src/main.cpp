/*
ESP32 FOTA Update Using GitHub Private repo + Data buffering 

Author  : Madeesha Lakshan
Date    : 01/02/2024
Org     : SLT Digital Lab
Features : FOTA update using Github private repo
           Save wifi crentials to the Flash(littlefs)
           Revert back to the previous version using external triggering
           save certification files to the flash(littlefs)
           Sending data via MQTT
           Bufered data in no connection
           soft and hard eset(avoid debouncing)
           birth abd last will messages
           subscribe to a topic and receive and display messages
           retained msg for all topics
Remarks  : The certification is valid until 2038. Please verify the `cert.h` file to establish the connection between the ESP32 and GitHub.
           Generate your personal access token(fine-grained)
           First time you compile the code uncomment saveCredentials and saveRootCACertificate in the setup function
           Enter your credetials,links and PAT to the config.h file 
           Ensure the JSON and binary (.bin) files are stored in a private repository and can be accessed by copying and pasting the URLs into your browser.
           
*/

/*
Networking Features:
WiFi connectivity with automatic reconnection
MQTT communication using AsyncMqttClient
HTTPS support with secure client
OTA (Over-The-Air) firmware updates from GitHub
Secure communications using SSL/TLS certificates


Storage Management:
LittleFS file system for storing:
WiFi credentials
Root CA certificates
NVS (Non-Volatile Storage) support
Buffered data storage for offline scenarios


MQTT Functionality:
Asynchronous MQTT communication
Last Will Testament (LWT) for offline status
Birth message for online status
Message publish/subscribe capabilities
JSON message parsing
QoS level 2 for reliable messaging
Automatic reconnection handling


Update & Recovery Features:
OTA firmware updates with version checking
Firmware rollback capability
Partition management and information
Update verification and error handling
Secure firmware downloads using GitHub token


Reset & Recovery Options:
Soft reset: Erases WiFi credentials
Hard reset: Complete system wipe including:

LittleFS format
NVS erase
WiFi credentials removal


Button control with:
Double press detection
Long press detection
Debouncing

Task Management:
FreeRTOS implementation
Multiple timers for:

MQTT reconnection
WiFi reconnection
Counter/sensor data publishing
Watchdog Timer (WDT) control


Debug Features:
Conditional debug printing
Detailed error reporting
Partition information printing
Connection status monitoring


Security Features:
SSL/TLS support
Certificate management
Secure storage of credentials
GitHub token authentication for updates


Data Management:
JSON parsing for messages
Data buffering for offline scenarios
Sensor data publishing
Counter implementation


Error Handling:
Connection failure recovery
Memory allocation checks
JSON parsing error handling
File system error management
Update failure recovery


Hardware Integration:
Button interrupt handling
Pin configuration
Reset button functionality
Rollback pin support
*/

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include "LittleFS.h"
#include <ArduinoJson.h>
#include <AsyncMqttClient.h>      // Non-blocking MQTT library for ESP32
#include "cert.h"
#include "config.h"  // Include the config file
#include "esp_partition.h"  // Include ESP-IDF partition header
#include "esp_ota_ops.h"    // Include ESP-IDF OTA operations header
#include "freertos/FreeRTOS.h"  // FreeRTOS library for task scheduling and timers
#include "freertos/timers.h"    // FreeRTOS timers
#include "esp_task_wdt.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_task_wdt.h" // Include WDT control


#define DEBUG_PRINT(x) if(DEBUG_PRINTS) Serial.print(x)
#define DEBUG_PRINTLN(x) if(DEBUG_PRINTS) Serial.println(x)
#define DEBUG_PRINTF(format, ...) if(DEBUG_PRINTS) Serial.printf(format, __VA_ARGS__)


// Function declarations
void initFileSystem();
void printPartitionInfo();
void rollbackToPreviousFirmware();
void checkForUpdate();
void firmwareUpdate();
int FirmwareVersionCheck();
void saveRootCACertificate(const char* rootCA);
bool loadRootCACertificate(String &rootCA);
void connectToWifi();
void connectToMqtt();
void WiFiEvent(WiFiEvent_t event);
void onMqttConnect(bool sessionPresent);
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
void processBufferedData();
void publishSensorData(void* parameter);
void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
void saveCredentials(const char* ssid, const char* password);
bool loadCredentials(String &ssid, String &password);
void IRAM_ATTR buttonISR();
void handleSoftReset();
void handleHardReset();

volatile unsigned long pressStartTime = 0;
volatile unsigned long lastPressTime = 0;
volatile int pressCount = 0;
volatile bool softResetFlag = false;
volatile bool hardResetFlag = false;
const int MAX_PROCESS_PER_CALL = 5;     

// MQTT and FreeRTOS objects
AsyncMqttClient mqttClient;       // MQTT client for non-blocking communication
TimerHandle_t mqttReconnectTimer; // Timer for reconnecting to MQTT broker
TimerHandle_t wifiReconnectTimer; // Timer for reconnecting to Wi-Fi
TimerHandle_t counterTimer;       // Timer for publishing counter data periodically

// Global variables
unsigned long counter = 0;        // Counter variable for publishing
String bufferedData = "";         // Buffer for storing unsent data

unsigned long previousMillis = 0;  // will store last time update was checked
const long interval = 5000;        // interval at which to check for updates (milliseconds)
String newFirmwareURL = "";        // Variable to store new firmware URL

void setup() {
    Serial.begin(115200);
    pinMode(RESET_BUTTON, INPUT_PULLUP);
    pinMode(ROLLBACK_PIN, INPUT_PULLUP);  // Set up the rollback pin as an input with an internal pull-up resistor
    initFileSystem();

    // Save the root CA certificate to LittleFS
    saveRootCACertificate(rootCACertificate);
    saveCredentials(ssid,password);
    printPartitionInfo();
    String rootCA;
    String ssid , password ;
    // Load wifi credentials
    if (loadCredentials(ssid,password)) {
        DEBUG_PRINTLN("Credentials loaded successfully");
    } else {
        DEBUG_PRINTLN("Failed to load credentials");
    }
    // Load Root CA certificate
    if (loadRootCACertificate(rootCA)) {
        DEBUG_PRINTLN("Root CA loaded successfully");
    } else {
        DEBUG_PRINTLN("Failed to load Root CA");
    }

    mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
    wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(15000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));
    counterTimer = xTimerCreate("counterTimer", pdMS_TO_TICKS(5000), pdTRUE, (void*)0, [](TimerHandle_t xTimer) { publishSensorData(nullptr); });

    // Register Wi-Fi event handler
    WiFi.onEvent(WiFiEvent);

    // Register MQTT client callbacks
    mqttClient.onConnect(onMqttConnect);
    mqttClient.onDisconnect(onMqttDisconnect);
    mqttClient.onMessage(onMqttMessage);
    mqttClient.setServer(MQTT_HOST, MQTT_PORT); // Set MQTT broker
    // Configure Last Will (Testament) Message
    mqttClient.setWill(LAST_WILL_TOPIC, 2, true, "{\"status\":\"offline\", \"deviceId\":\"" DEVICE_ACCESS_TOKEN "\"}");

    connectToWifi();             // Start Wi-Fi connection
    xTimerStart(counterTimer, 0); // Start counter timer for publishing data

    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    attachInterrupt(digitalPinToInterrupt(RESET_BUTTON), buttonISR, CHANGE);
}

void loop() {
    
    if (softResetFlag) {
        softResetFlag = false;
        handleSoftReset();
    }

    if (hardResetFlag) {
        hardResetFlag = false;
        handleHardReset();
    }

    vTaskDelay(pdMS_TO_TICKS(10));  // FreeRTOS friendly delay


    unsigned long currentMillis = millis();
    if (digitalRead(ROLLBACK_PIN) == LOW) {  // Check if the rollback pin is triggered (assuming active low)
        DEBUG_PRINTLN("Rollback pin triggered. Rolling back to previous firmware.");
        rollbackToPreviousFirmware();
    }

    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        checkForUpdate();
    }
}

void initFileSystem() {
    if (!LittleFS.begin()) {
        DEBUG_PRINTLN("Failed to mount LittleFS, formatting...");
        if (LittleFS.format()) {
            DEBUG_PRINTLN("LittleFS formatted successfully");
            if (!LittleFS.begin()) {
                DEBUG_PRINTLN("Failed to mount LittleFS after formatting");
                return;
            }
        } else {
            DEBUG_PRINTLN("Failed to format LittleFS");
            return;
        }
    }
}

void printPartitionInfo() {
    const esp_partition_t* running = esp_ota_get_running_partition();
    const esp_partition_t* next = esp_ota_get_next_update_partition(NULL);
    const esp_partition_t* previous = esp_ota_get_last_invalid_partition();

    DEBUG_PRINTLN("Running Partition:");
    DEBUG_PRINTF("Type: %d, Subtype: %d, Address: 0x%08x, Size: 0x%08x, Label: %s\n", 
                  running->type, running->subtype, running->address, running->size, running->label);

    DEBUG_PRINTLN("Next Partition:");
    DEBUG_PRINTF("Type: %d, Subtype: %d, Address: 0x%08x, Size: 0x%08x, Label: %s\n", 
                  next->type, next->subtype, next->address, next->size, next->label);

    DEBUG_PRINTLN("Previous Invalid Partition:");
    if (previous != NULL) {
        DEBUG_PRINTF("Type: %d, Subtype: %d, Address: 0x%08x, Size: 0x%08x, Label: %s\n", 
                      previous->type, previous->subtype, previous->address, previous->size, previous->label);
    } else {
        DEBUG_PRINTLN("None");
    }
}

void rollbackToPreviousFirmware() {
    const esp_partition_t* running = esp_ota_get_running_partition();
    const esp_partition_t* next = esp_ota_get_next_update_partition(NULL);
    const esp_partition_t* previous = (running == esp_ota_get_boot_partition()) ? next : esp_ota_get_boot_partition();

    DEBUG_PRINTLN("Running Partition:");
    DEBUG_PRINTF("Type: %d, Subtype: %d, Address: 0x%08x, Size: 0x%08x, Label: %s\n", 
                  running->type, running->subtype, running->address, running->size, running->label);

    if (previous != NULL && previous != running) {
        DEBUG_PRINTLN("Setting boot partition to the previous firmware.");
        esp_err_t err = esp_ota_set_boot_partition(previous);
        if (err != ESP_OK) {
            DEBUG_PRINTF("Failed to set boot partition: %s\n", esp_err_to_name(err));
        } else {
            DEBUG_PRINTLN("Boot partition set successfully. Rebooting...");
            ESP.restart();
        }
    } else {
        DEBUG_PRINTLN("No valid previous partition found or already running the previous firmware.");
    }
}

void checkForUpdate() {
    DEBUG_PRINT("Checking for firmware updates... Current version: ");
    DEBUG_PRINTLN(FirmwareVer);

    if (FirmwareVersionCheck()) {
        DEBUG_PRINTLN("New firmware detected. Updating...");
        firmwareUpdate();
    } else {
        DEBUG_PRINTLN("No new firmware available.");
    }
}

void firmwareUpdate() {
    WiFiClientSecure* client = new WiFiClientSecure;

    if (client) {
        String rootCA;
        if (loadRootCACertificate(rootCA)) {
            client->setCACert(rootCA.c_str());
        } else {
            DEBUG_PRINTLN("Using built-in root CA");
            client->setCACert(rootCACertificate);  // Fallback to the built-in root CA
        }

        HTTPClient https;

        if (https.begin(*client, newFirmwareURL)) { // Begin connection with firmware URL
            https.addHeader("Authorization", "token " + String(GITHUB_TOKEN)); // Add the authorization header

            DEBUG_PRINTLN("Starting firmware download...");
            int httpCode = https.GET(); // Perform GET request to download firmware binary

            if (httpCode == HTTP_CODE_OK) {
                // Stream the firmware directly to the update process
                WiFiClient* updateClient = https.getStreamPtr();
                size_t contentLength = https.getSize();

                if (contentLength > 0) {
                    DEBUG_PRINTF("Firmware size: %d bytes\n", contentLength);

                    // Start the OTA update
                    if (Update.begin(contentLength)) {
                        size_t written = Update.writeStream(*updateClient);
                        if (written == contentLength) {
                            DEBUG_PRINTLN("Firmware update successfully written!");
                        } else {
                            DEBUG_PRINTF("Firmware update failed! Written: %d of %d bytes\n", written, contentLength);
                        }

                        // End the update process
                        if (Update.end()) {
                            if (Update.isFinished()) {
                                DEBUG_PRINTLN("Update completed successfully. Rebooting...");
                                ESP.restart();
                            } else {
                                DEBUG_PRINTLN("Update not finished. Something went wrong!");
                            }
                        } else {
                            DEBUG_PRINTF("Update failed: %s\n", Update.errorString());
                        }
                    } else {
                        DEBUG_PRINTF("Not enough space for OTA update: %d bytes needed\n", contentLength);
                    }
                } else {
                    DEBUG_PRINTLN("Content length invalid or zero.");
                }
            } else {
                DEBUG_PRINTF("Firmware download failed, HTTP code: %d\n", httpCode);
            }

            https.end(); // End the HTTP connection
        } else {
            DEBUG_PRINTLN("HTTP client setup failed for firmware update.");
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
            DEBUG_PRINTLN("Using built-in root CA");
            client->setCACert(rootCACertificate);  // Fallback to the built-in root CA
        }

        HTTPClient https;

        if (https.begin(*client, FirmwareURL)) {
            https.addHeader("Authorization", "token " + String(GITHUB_TOKEN)); // Add the authorization header
            DEBUG_PRINT("[HTTPS] GET...\n");
            delay(100);
            httpCode = https.GET();
            delay(100);
            if (httpCode == HTTP_CODE_OK) {
                payload = https.getString();
            } else {
                DEBUG_PRINTLN("Error Occurred During Version Check: ");
                DEBUG_PRINTLN(httpCode);
            }
            https.end();
        }
        delete client;
    }

    if (httpCode == HTTP_CODE_OK) {
        // Parse JSON payload
        JsonDocument doc ;
        DeserializationError error = deserializeJson(doc, payload);
        if (error) {
            DEBUG_PRINT("JSON deserialization failed: ");
            DEBUG_PRINTLN(error.f_str());
            return 0;
        }

        String newVersion = doc["version"].as<String>();
        newFirmwareURL = doc["bin_url"].as<String>();
        newVersion.trim();
        newFirmwareURL.trim();

        if (newVersion.equals(FirmwareVer)) {
            DEBUG_PRINTF("\nDevice is already on the latest firmware version: %s\n", FirmwareVer.c_str());
            return 0;
        } else {
            DEBUG_PRINTLN(newVersion);
            DEBUG_PRINTLN("New Firmware Detected");
            return 1;
        }
    }
    return 0;
}

void saveRootCACertificate(const char* rootCA) {
    File file = LittleFS.open("/rootCA.pem", "w");
    if (!file) {
        DEBUG_PRINTLN("Failed to open file for writing root CA");
        return;
    }
    file.print(rootCA);
    file.close();
    DEBUG_PRINTLN("Root CA saved");
}

bool loadRootCACertificate(String &rootCA) {
    File file = LittleFS.open("/rootCA.pem", "r");
    if (!file) {
        DEBUG_PRINTLN("Failed to open file for reading root CA");
        return false;
    }

    rootCA = file.readString();
    file.close();

    if (rootCA.length() == 0) {
        return false;
    }

    return true;
}
void saveCredentials(const char* ssid, const char* password) {
    File file = LittleFS.open("/wificredentials.txt", "w");
    if (!file) {
        DEBUG_PRINTLN("Failed to open file for writing");
        return;
    }
    file.println(ssid);
    file.println(password);
    file.close();
    DEBUG_PRINTLN("Credentials saved");
}

bool loadCredentials(String &ssid, String &password) {
    File file = LittleFS.open("/wificredentials.txt", "r");
    if (!file) {
        DEBUG_PRINTLN("Failed to open file for reading");
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

void connectToWifi() {
  if (WiFi.status() != WL_CONNECTED) {
    DEBUG_PRINTLN("Connecting to Wi-Fi...");
    WiFi.begin(ssid, password);
  }
}

void connectToMqtt() {
  if (!mqttClient.connected()) {
    DEBUG_PRINTLN("Connecting to MQTT...");
    mqttClient.connect();
  }
}

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      DEBUG_PRINTLN("WiFi connected");
      DEBUG_PRINT("IP address: ");
      DEBUG_PRINTLN(WiFi.localIP());
      connectToMqtt();  // Connect to MQTT after getting an IP
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      DEBUG_PRINTLN("WiFi lost connection");
      xTimerStop(mqttReconnectTimer, 0);
      xTimerStart(wifiReconnectTimer, 0); // Start Wi-Fi reconnect timer
      break;
  }
}

void onMqttConnect(bool sessionPresent) {
  DEBUG_PRINTLN("Connected to MQTT.");
  mqttClient.subscribe(SUBSCRIBE_TOPIC, 2); // Subscribe to the counter topic
  // Publish birth messag
  String birthMessage = "{\"status\":\"online\", \"deviceId\":\"" DEVICE_ACCESS_TOKEN "\", \"ip\":\"" + WiFi.localIP().toString() + "\"}";
  mqttClient.publish(BIRTH_TOPIC, 2, true, birthMessage.c_str());
  processBufferedData();
}


void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  DEBUG_PRINTLN("Disconnected from MQTT.");
  if (WiFi.isConnected()) {
    xTimerStart(mqttReconnectTimer, 0); // Start MQTT reconnect timer
  }
}

void processBufferedData() {
  if (bufferedData.length() > 0) {
    if (mqttClient.publish(BUFFERED_DATA_TOPIC, 2, true, bufferedData.c_str())) {
      DEBUG_PRINTLN("Buffered data sent!");
      bufferedData = ""; // Clear buffer after successful transmission
    } else {
      DEBUG_PRINTLN("Failed to send buffered data!");
    }
  }
}

void publishSensorData(void* parameter) {
  String sensorData = "Counter: " + String(counter);

  // Check if Wi-Fi and MQTT are connected before sending data
  if (WiFi.isConnected() && mqttClient.connected()) {
    if (mqttClient.publish(COUNTER_TOPIC, 2, true, sensorData.c_str())) {
      DEBUG_PRINTLN("Data published: " + sensorData);
    } else {
      DEBUG_PRINTLN("Failed to publish data!");
      bufferedData += sensorData + "\n"; // Buffer data if publish fails
    }
  } else {
    bufferedData += sensorData + "\n"; // Buffer data if no connection
    DEBUG_PRINTLN("Data buffered due to no connection");
  }

  counter++; // Increment the counter
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
    DEBUG_PRINTLN("Message received on topic: " + String(topic));
    
    // Create a null-terminated string from the payload
    char* message = new char[len + 1];
    memcpy(message, payload, len);
    message[len] = '\0';
    
    // Create JSON document
    JsonDocument doc;  
    DeserializationError error = deserializeJson(doc, message);
    
    // Check if parsing succeeded
    if (error) {
        DEBUG_PRINT("JSON parsing failed: ");
        DEBUG_PRINTLN(error.c_str());
    } else {
        // Assuming JSON format like {"state": "0"} or {"state": "1"}
        const char* state = doc["state"];
        
        if (String(state) == "0") {
            DEBUG_PRINTLN("off");
        } 
        else if (String(state) == "1") {
            DEBUG_PRINTLN("on");
        }
        else {
            DEBUG_PRINTLN("Unknown state: " + String(state));
        }
    }
    
    // Free the allocated memory
    delete[] message;
}
// **Interrupt Service Routine (ISR) for button**
void IRAM_ATTR buttonISR() {
    static unsigned long lastInterruptTime = 0;
    unsigned long currentTime = millis();
    
    // Debounce check
    if (currentTime - lastInterruptTime < DEBOUNCE_TIME) {
        return; // Ignore noise
    }
    lastInterruptTime = currentTime;

    bool buttonState = digitalRead(RESET_BUTTON);

    if (buttonState == LOW) { // Button pressed
        if (pressCount == 0) {
            pressStartTime = currentTime;
            lastPressTime = currentTime;
        } else if (pressCount == 1) {
            // Double press detected within time window
            if (currentTime - lastPressTime <= DOUBLE_PRESS_WINDOW) {
                hardResetFlag = true; // Set flag instead of direct function call
                pressCount = 0;
                return;
            }
        }
        pressCount++;
    } else { // Button released
        if (currentTime - pressStartTime >= SOFT_RESET_TIME) {
            softResetFlag = true; // Set flag instead of direct function call
            pressCount = 0;
            return;
        }

        if (pressCount == 1 && (currentTime - lastPressTime > DOUBLE_PRESS_WINDOW)) {
            pressCount = 0;
        }
    }

    if (pressCount > 2) {
        pressCount = 0;
    }
}


// **Soft Reset: Erase WiFi credentials**
void handleSoftReset() {
    DEBUG_PRINTLN("Performing Soft Reset...");
    delay(100);

    if (LittleFS.remove("/wifi_credentials.txt")) {
        DEBUG_PRINTLN("WiFi credentials erased");
    } else {
        DEBUG_PRINTLN("WiFi credentials file not found");
    }

    delay(500);
    ESP.restart();
}

// **Hard Reset: Erase all flash memory**
void handleHardReset() {
    DEBUG_PRINTLN("Performing Hard Reset...");

    // **Step 1: Disable the Watchdog Timer (WDT)**
    esp_task_wdt_deinit();  // Disable Task Watchdog Timer completely
    disableCore0WDT();      // Ensure core 0 WDT is disabled

    // **Step 2: Format LittleFS to erase any files stored**
    if (LittleFS.format()) {
        DEBUG_PRINTLN("LittleFS formatted successfully");
    } else {
        DEBUG_PRINTLN("LittleFS format failed");
    }

    // **Step 3: Erase NVS (Non-Volatile Storage)**
    ESP_ERROR_CHECK(nvs_flash_erase());
    ESP_ERROR_CHECK(nvs_flash_init());

    // **Step 4: Erase Wi-Fi Credentials (if stored in LittleFS)**
    if (LittleFS.remove("/wifi_credentials.txt")) {
        DEBUG_PRINTLN("Wi-Fi credentials removed.");
    } else {
        DEBUG_PRINTLN("No Wi-Fi credentials file found.");
    }

    // **Step 5: Ensure delay for tasks to finish**
    delay(2000);  // Allow time for cleanup

    // **Step 6: Put the ESP32 into deep sleep for a complete reset**
    DEBUG_PRINTLN("Entering deep sleep to ensure full reset...");
    esp_deep_sleep_start();  // Initiate deep sleep to shut down everything
}

