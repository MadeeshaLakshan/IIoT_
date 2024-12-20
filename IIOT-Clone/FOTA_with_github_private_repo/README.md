# ESP32 Firmware Update via OTA

Perform an Over-The-Air (OTA) firmware update for an ESP32 using a GitHub repository to host the firmware binary files. The project includes the following features:

- **OTA update using a GitHub private repository**: The ESP32 fetches the latest firmware from a private GitHub repository using a Personal Access Token (PAT) for authentication.
- **Save WiFi credentials to Flash (LittleFS)**: WiFi credentials are saved to the Flash filesystem (LittleFS) for persistent storage.
- **Revert to the previous firmware version using external triggering**: The firmware can be rolled back to a previous version when an external trigger (e.g., a button press) is detected.

## How to Get a Personal Access Token (PAT) from GitHub

To use this project, you will need a GitHub Personal Access Token (PAT) to authenticate API requests. Follow these steps to generate a PAT:

### Step 1: Log in to GitHub
Go to [GitHub](https://github.com) and log in to your account.

### Step 2: Navigate to Personal Access Tokens
- Click on your profile picture in the upper-right corner and select `Settings`.
- In the left sidebar, click on `Developer settings`.
- In the left sidebar of the developer settings, click on `Personal access tokens`.

### Step 3: Generate a New Token
- Click on `Generate new token`.
- Give your token a descriptive name.
- Select the scopes or permissions you want to grant this token. For this project, you will need at least `repo` access to read repository contents.


