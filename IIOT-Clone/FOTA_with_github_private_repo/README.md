# ESP32 Firmware Update via OTA

This project demonstrates how to perform an Over-The-Air (OTA) firmware update for an ESP32 using a GitHub repository to host the firmware binary files.

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

### Step 4: Generate and Save the Token
- Click on `Generate token` at the bottom of the page.
- Copy the token to a secure location. **Note: You will not be able to see this token again once you leave this page.**

### Step 5: Use the Token in Your Project
- In your Arduino code, add the token where needed for authorization headers.

```cpp
#include "cert.h"  // Ensure you have the root certificate included
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// Define your GitHub token here
#define GITHUB_TOKEN "your_personal_access_token_here"

// Use the token in your HTTP requests
httpUpdate.setAuthorization("token", GITHUB_TOKEN);
