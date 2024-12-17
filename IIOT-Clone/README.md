<<<<<<< HEAD
# ESP32 FOTA Update Using GitHub
=======
# ESP32 FOTA Update Using GitHub Public Repo
>>>>>>> 304c1ba811e77c4ea2c5e688bdeec7398a4df22e

## Author: Madeesha Lakshan
## Date: 12/17/2024
## Organization: SLT Digital Lab

### Purpose
<<<<<<< HEAD
This project demonstrates how to perform a Firmware Over The Air (FOTA) update on an ESP32 using a GitHub public repository. The process includes checking for new firmware versions and updating the firmware if a new version is available.
=======
This project demonstrates how to perform a Firmware Over The Air (FOTA) update on an ESP32 using a GitHub public repository. The process includes checking for new firmware versions and updating the firmware if a new version is available using json file.
>>>>>>> 304c1ba811e77c4ea2c5e688bdeec7398a4df22e

### Requirements
- ESP32 development board
- Arduino IDE
- WiFi network
- GitHub account
- ArduinoJson library
- HTTPClient library
- WiFiClientSecure library

### Setting Up the Project

1. **Install Required Libraries**
   - Open the Arduino IDE.
   - Go to `Sketch` -> `Include Library` -> `Manage Libraries...`.
   - Install the following libraries:
<<<<<<< HEAD
     - `ArduinoJson` by Benoit Blanchon v7.2.1
=======
     - `ArduinoJson` by Benoit Blanchon v 7.2.1
>>>>>>> 304c1ba811e77c4ea2c5e688bdeec7398a4df22e
     - `HTTPClient`
     - `WiFiClientSecure`

2. **Create a JSON File for Version Check**
   - Create a `version.json` file in your GitHub repository with the following content:
     ```json
     {
       "version": "1.0.1",
       "bin_url": "https://raw.githubusercontent.com/YourUsername/YourRepository/main/your_firmware.bin"
     }
     ```
   - Replace `1.0.1` with the new firmware version.
   - Replace `https://raw.githubusercontent.com/YourUsername/YourRepository/main/your_firmware.bin` with the URL of your compiled firmware binary file.

3. **Upload Files to GitHub**
   - Upload the `version.json` file to your GitHub repository.
   - Upload your compiled firmware binary file (`.bin`) to the same repository.

4. **Get Accurate Root CA Certificate**
   - To securely connect to GitHub over HTTPS, you need the correct root CA certificate.
     - Create a new file named `cert.h` in your Arduino project and paste the copied content as follows:
       ```cpp
       const char* rootCACertificate = R"EOF(
       -----BEGIN CERTIFICATE-----
       MIIF...
       -----END CERTIFICATE-----
       )EOF";
       ```

5. **Update Your ESP32 Code**
   - Update the SSID and WiFi password in your code:
     ```cpp
     const char* ssid = "Your_SSID";
     const char* wifiPassword = "Your_Password";
     ```
<<<<<<< HEAD
   - Update the URL for the JSON file:
=======
   - Update the URL for the JSON file (Go to the Raw version and get Url: 
>>>>>>> 304c1ba811e77c4ea2c5e688bdeec7398a4df22e
     ```cpp
     #define URL_fw_JSON "https://raw.githubusercontent.com/YourUsername/YourRepository/main/version.json"
     ```

<<<<<<< HEAD
=======
6. **Compile and Upload Your Code**
   - Connect your ESP32 to your computer.
   - Open the Arduino IDE and select the correct board and port.
   - Compile and upload your code to the ESP32.


>>>>>>> 304c1ba811e77c4ea2c5e688bdeec7398a4df22e

