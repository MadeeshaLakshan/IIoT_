# ESP32 FOTA Update Using GitHub Public Repo

### Purpose
This project demonstrates how to perform a Firmware Over The Air (FOTA) update on an ESP32 using a GitHub public repository. The process includes checking for new firmware versions and updating the firmware if a new version is available using json file.

### Setting Up the Project

1. **Create a JSON File for Version Check**
   - Create a `version.json` file in your GitHub repository with the following content:
     ```json
     {
       "version": "1.0.1",
       "bin_url": "https://raw.githubusercontent.com/YourUsername/YourRepository/main/your_firmware.bin"
     }
     ```
   - Replace `1.0.1` with the new firmware version.
   - Replace `https://raw.githubusercontent.com/YourUsername/YourRepository/main/your_firmware.bin` with the URL of your compiled firmware binary file.

2. **Upload Files to GitHub**
   - Upload the `version.json` file to your GitHub repository.
   - Upload your compiled firmware binary file (`.bin`) to the same repository.

3. **Get Accurate Root CA Certificate**
   - To securely connect to GitHub over HTTPS, you need the correct root CA certificate.
     - Create a new file named `cert.h` in your Arduino project and paste the copied content as follows:
       ```cpp
       const char * rootCACertificate = \
          "-----BEGIN CERTIFICATE-----\n"
         "MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh\n"
          ...................................................................
         "pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl\n"
         "MrY=\n"
"-----END CERTIFICATE-----\n";

4. **Update Your Config.h ESP32 Code**
   - Update the SSID and WiFi password in your code:
     ```cpp
     const char* ssid = "Your_SSID";
     const char* wifiPassword = "Your_Password";
     ```
   - Update the URL for the JSON file (Go to the Raw version and get Url: 
     ```cpp
     #define URL_fw_JSON "https://raw.githubusercontent.com/YourUsername/YourRepository/main/version.json"
     ```



