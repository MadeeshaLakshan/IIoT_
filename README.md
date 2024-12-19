# IIOT-Clone
clone the IIOT repo from https://github.com/RUM35H/IIoT
# ESP32 OTA Firmware Update with GitHub

## Introduction

This project demonstrates how to perform Over-the-Air (OTA) firmware updates on an ESP32 using files hosted on GitHub. The ESP32 checks for firmware updates periodically and downloads and installs the new firmware if a newer version is available. The firmware version information is stored in a JSON file on GitHub, and the ESP32 uses this file to determine if an update is necessary.

## How It Works

1. **Initialize File System**: The ESP32 initializes the LittleFS file system to store WiFi credentials.
2. **Load WiFi Credentials**: The ESP32 loads saved WiFi credentials from the file system and attempts to connect to the WiFi network.
3. **Check for Updates**: Periodically, the ESP32 checks a JSON file on GitHub to see if a newer firmware version is available.
4. **Firmware Update**: If a new firmware version is detected, the ESP32 downloads the new firmware binary file from GitHub and updates itself.

## Get Certificate File

To securely connect to GitHub and download files, you need the GitHub SSL certificate. Here's how to get it:

1. **Download Certificate**: Download the GitHub SSL certificate from a reliable source or extract it from your browser.
2. **Save Certificate**: Save the certificate as `cert.h` and include it in your project directory.

The `cert.h` file should look something like this:

```cpp
// cert.h
const char* rootCACertificate = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDdzCCAl+gAwIBAgIEb8bmVzANBgkqhkiG9w0BAQsFADBoMQswCQYDVQQGEwJV\n" \
...
"-----END CERTIFICATE-----\n";
