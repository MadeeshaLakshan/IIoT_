# ESP8266 OTA Firmware Update

This project demonstrates how to perform Over-the-Air (OTA) firmware updates on an ESP8266 device. The configuration parameters are separated into a `config.h` file for easier management.

## Configuration

### `config.h`

The `config.h` file contains all the configuration parameters required for the project. Update the parameters as needed.

#### WiFi Credentials

Set your WiFi SSID and password to connect the ESP8266 to your network.
```cpp
const char* ssid = "Your_SSID";
const char* password = "Your_PASSWORD";
