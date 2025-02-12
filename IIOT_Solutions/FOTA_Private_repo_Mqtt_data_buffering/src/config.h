#ifndef CONFIG_H
#define CONFIG_H

// WiFi Credentials 
const char* ssid = "Redmi Note 13"   ;
const char* password ="12345678910" ;

// MQTT broker configuration
#define MQTT_HOST "broker.hivemq.com" // Public MQTT broker
#define MQTT_PORT 1883

#define ROLLBACK_PIN 4  // GPIO4 pin
#define RESET_BUTTON 0     // GPIO0 button
#define SOFT_RESET_TIME 3000 // 3 seconds for soft reset
#define DOUBLE_PRESS_WINDOW 500 // 500ms for double press
#define DEBOUNCE_TIME 50  // Adjust based on the switch's bounce characteristics

// MQTT topic to publish the counter data
#define COUNTER_TOPIC "test/counter/data"
#define BUFFERED_DATA_TOPIC "test/counter/dataBuffered"
#define LAST_WILL_TOPIC "device/status"   // Topic for Last Will message
#define BIRTH_TOPIC "device/status"       // Topic for Birth message*/
#define SUBSCRIBE_TOPIC "test/counter/datasub" // Topic to subscribe to
#define DEVICE_ACCESS_TOKEN "ESP32" // Replace with your device's access token

#define DEBUG_PRINTS true
#define MAX_BUFFER_SIZE 10

// Firmware Version
String FirmwareVer = "1.0.1";

// GitHub Token
#define GITHUB_TOKEN "PAT" // Replace with your actual token

// URLs
#define URL_fw_Version "https://raw.githubusercontent.com/MadeeshaLakshan/ESP32_OTA_Github/main/version.json"

#endif // CONFIG_H
