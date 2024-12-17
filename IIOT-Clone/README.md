# ESP32 FOTA Update using GitHub

## Author
Madeesha Lakshan

## Date
12/17/2024

## Organization
SLT Digital Lab

## Purpose
Perform Firmware Over The Air (FOTA) update using a GitHub public repository.

## Remarks
This certificate is only valid until 2038. Please verify the `cert.h` file to establish the connection between ESP32 and GitHub. Enter your Wi-Fi credentials (SSID and password) in the code.

## Table of Contents
1. [Introduction](#introduction)
2. [How It Works](#how-it-works)
3. [Get Certificate File](#get-certificate-file)
4. [Setup](#setup)
5. [Usage](#usage)

## Introduction
This project demonstrates how to perform a Firmware Over The Air (FOTA) update on an ESP32 using a firmware binary hosted on a GitHub public repository. The ESP32 checks for updates at regular intervals and updates its firmware if a new version is available.

## How It Works
1. **Initialization**: The ESP32 initializes and connects to the specified Wi-Fi network.
2. **Periodic Check for Updates**: The ESP32 periodically checks a file on GitHub (`version.txt`) to see if a newer firmware version is available.
3. **Firmware Version Check**: If a new firmware version is detected, the ESP32 downloads the new firmware binary from GitHub and updates its firmware.
4. **Firmware Update**: The ESP32 uses the GitHub public repository's root CA certificate to establish a secure connection and download the new firmware binary.

## Get Certificate File
Ensure you have the root CA certificate file (`cert.h`) for GitHub. This file is necessary to establish a secure connection between the ESP32 and GitHub. The certificate is valid until 2038.

## Setup
1. **Wi-Fi Credentials**: Enter your Wi-Fi SSID and password in the code.
    ```cpp
    const char* ssid = "SSID";
    const char* wifiPassword = "PASSWORD";
    ```

2. **Firmware Version URL**: Specify the URL of the `version.txt` file in your GitHub repository.
    ```cpp
    #define URL_fw_Version "https://raw.githubusercontent.com/MadeeshaLakshan/ESP32_OTA_Github/refs/heads/main/version.txt"
    ```

3. **Firmware Binary URL**: Specify the URL of the firmware binary file in your GitHub repository.
    ```cpp
    #define URL_fw_Bin "https://raw.githubusercontent.com/MadeeshaLakshan/ESP32_OTA_Github/main/build/esp32.esp32.esp32/ESP32_FinalCodes_https.ino.bin"
    ```

4. **Include Certificate File**: Ensure the `cert.h` file is included in your project.
    ```cpp
    #include "cert.h"
    ```

## Usage
1. **Connect to Wi-Fi**: The ESP32 will attempt to connect to the specified Wi-Fi network.
    ```cpp
    WiFi.begin(ssid, wifiPassword);
    ```

2. **Check for Updates**: The ESP32 will periodically check for firmware updates.
    ```cpp
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        checkForUpdate();
    }
    ```

3. **Firmware Version Check**: The `FirmwareVersionCheck` function compares the current firmware version with the version specified in `version.txt`.
    ```cpp
    int FirmwareVersionCheck() {
        ...
        if (payload.equals(FirmwareVer)) {
            Serial.printf("\nDevice is already on the latest firmware version: %s\n", FirmwareVer.c_str());
            return 0;
        } else {
            Serial.println(payload);
            Serial.println("New Firmware Detected");
            return 1;
        }
    }
    ```

4. **Firmware Update**: The `firmwareUpdate` function downloads and applies the new firmware binary.
    ```cpp
    void firmwareUpdate() {
        WiFiClientSecure client;
        client.setCACert(rootCACertificate);
        t_httpUpdate_return ret = httpUpdate.update(client, URL_fw_Bin);
        ...
    }
    ```

Follow these steps to ensure your ESP32 can perform FOTA updates using a GitHub public repository.

