#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>
#include <CertStoreBearSSL.h>
#include <ArduinoJson.h>

BearSSL::CertStore certStore;
#include <time.h>

const String FirmwareVer = "1.0.0";  // Current firmware version of the ESP8266
#define URL_fw_Version "/MadeeshaLakshan/ESP8266_FOTA_PUBLIC/refs/heads/main/version.json"
const char* host = "raw.githubusercontent.com";
const int httpsPort = 443;

// DigiCert High Assurance EV Root CA
const char trustRoot[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP
BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV
5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY
1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4
NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG
Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91
8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe
pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl
MrY=
-----END CERTIFICATE-----
)EOF";
X509List cert(trustRoot);

extern const unsigned char caCert[] PROGMEM;
extern const unsigned int caCertLen;

const char* ssid = "Dialog 4G 044";
const char* password = "c0Deb7c5";

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
    if ((currentMillis - previousMillis) >= interval) {
        previousMillis = currentMillis;
        setClock();
        FirmwareUpdate();
    }

    if ((currentMillis - previousMillis_2) >= mini_interval) {
        static int idle_counter = 0;
        previousMillis_2 = currentMillis;
        Serial.print(" Active fw version:");
        Serial.println(FirmwareVer);
        if (WiFi.status() != WL_CONNECTED)
            connect_wifi();
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("");
    Serial.println("Start");
    WiFi.mode(WIFI_STA);
    connect_wifi();
    setClock();
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
    repeatedCall();
}
