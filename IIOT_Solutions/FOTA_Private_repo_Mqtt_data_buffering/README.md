# ESP32 FOTA Update System with MQTT Integration

An industrial IoT solution featuring secure Over-The-Air (FOTA) updates using GitHub private repositories, MQTT communication, and robust system management capabilities for ESP32 devices.

## Features

### 🔄 Firmware Management
- FOTA updates using GitHub private repositories
- Secure firmware deployment with personal access tokens
- Version control and automatic update detection
- Rollback capability to previous firmware versions
- Custom partition configuration for firmware management

### 📡 MQTT Communication
- Asynchronous MQTT client implementation
- QoS Level 2 support for guaranteed message delivery
- Data buffering for offline scenarios
- Last Will Testament (LWT) for device status monitoring
- Birth messages for online status notification
- JSON-based message formatting

### 💾 Storage Management
- LittleFS implementation for persistent storage
- Secure storage of WiFi credentials
- SSL/TLS certificate management
- NVS (Non-Volatile Storage) support

### ⚡ System Recovery
- Dual reset functionality (Soft/Hard reset)
- Debounced button handling
- WiFi credential management
- Complete system wipe capability
- Deep sleep implementation

### ⏱️ Task Management
- FreeRTOS implementation
- Multiple timer management
- MQTT reconnection handling
- WiFi connection monitoring
- Watchdog Timer (WDT) integration

## Prerequisites

- ESP32 Development Board
- Arduino IDE or PlatformIO
- GitHub Account (for hosting firmware files)
- MQTT Broker
- Required Libraries:
  - WiFi
  - HTTPClient
  - HTTPUpdate
  - WiFiClientSecure
  - LittleFS
  - ArduinoJson
  - AsyncMqttClient
  - FreeRTOS

## Installation

1. Clone this repository:
```bash
git clone https://github.com/yourusername/esp32-fota-project.git
```

2. Copy `config.example.h` to `config.h` and update with your credentials:
```cpp
// WiFi Configuration
#define ssid "Your_SSID"
#define password "Your_Password"

// MQTT Configuration
#define MQTT_HOST "Your_MQTT_Broker"
#define MQTT_PORT 1883

// GitHub Configuration
#define GITHUB_TOKEN "Your_GitHub_Token"
```

3. Create a private GitHub repository for hosting firmware files
4. Configure your Arduino IDE or PlatformIO environment for ESP32
5. Upload the code to your ESP32 device

## Configuration

### First-Time Setup
1. Uncomment the following lines in `setup()` for first-time configuration:
```cpp
saveRootCACertificate(rootCACertificate);
saveCredentials(ssid, password);
```

### GitHub Repository Setup
1. Create a JSON file (`version.json`) in your GitHub repository:
```json
{
    "version": "1.0.0",
    "bin_url": "https://raw.githubusercontent.com/username/repo/main/firmware.bin"
}
```

2. Generate a Personal Access Token (PAT) with repo scope
3. Update `config.h` with your GitHub token

## Hardware Setup

### Pin Configuration
- `RESET_BUTTON`: GPIO pin for reset functionality
- `ROLLBACK_PIN`: GPIO pin for firmware rollback trigger

### Reset Functionality
- Single long press: Soft reset (WiFi credentials reset)
- Double press: Hard reset (Complete system wipe)
- Rollback pin trigger: Revert to previous firmware

## Usage

### MQTT Topics
- Birth Topic: Device online status
- Last Will Topic: Device offline status
- Data Topic: Sensor/counter data
- Buffered Data Topic: Offline data storage

### System Monitoring
The system provides detailed debug output via Serial Monitor:
- WiFi connection status
- MQTT connection status
- Firmware update progress
- System reset status
- Partition information

### Error Handling
- Connection failure recovery
- JSON parsing error management
- Memory allocation checks
- File system error handling
- Update failure recovery

## Contributing
Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

## License
[MIT](https://choosealicense.com/licenses/mit/)

## Acknowledgments
- SLT Digital Lab for project guidance
- Contributors to the ESP32 community
- Arduino and ESP32 library maintainers

## Author
- Madeesha Lakshan
- Organization: SLT Digital Lab
- Date: 01/02/2024
