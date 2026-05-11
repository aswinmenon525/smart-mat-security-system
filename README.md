# Smart Mat Security System

An ESP32-based smart door access system that uses FSR pressure sensors to detect footsteps and triggers browser-based face recognition for access control. Sends Gmail alerts for every access event via Google Apps Script.

Built as a personal project during 2nd year ECE at VIT Vellore.

---

## Features

- Pressure-sensitive mat using 4x FSR sensors detects when someone steps on it
- Browser-based face recognition using face-api.js (no camera module needed on ESP32)
- Animated door opening UI served directly from ESP32 web server
- Gmail alerts sent for both known and unknown persons via Google Apps Script
- Buzzer feedback tones for access granted / denied
- PIR motion sensor support
- Real-time mat status polling every 1 second

---

## Hardware

| Component | Quantity |
|-----------|----------|
| ESP32 Dev Board | 1 |
| FSR (Force Sensitive Resistor) | 4 |
| PIR Motion Sensor | 1 |
| Buzzer | 1 |
| Jumper Wires | As needed |

### Pin Connections

| Component | ESP32 Pin |
|-----------|-----------|
| FSR1 | GPIO 34 |
| FSR2 | GPIO 35 |
| FSR3 | GPIO 32 |
| FSR4 | GPIO 33 |
| PIR | GPIO 25 |
| Buzzer | GPIO 14 |

---

## Software & Libraries

- Arduino IDE
- ESP32 Board Package
- WiFi.h (built-in)
- WebServer.h (built-in)
- HTTPClient.h (built-in)
- [face-api.js](https://github.com/justadudewhohacks/face-api.js) (loaded via CDN in browser)

---

## Setup Instructions

### 1. Configure credentials

```bash
cp config.h.example config.h
```

Edit `config.h` and fill in:
```cpp
#define WIFI_SSID      "your_wifi_name"
#define WIFI_PASSWORD  "your_wifi_password"
#define SCRIPT_URL     "your_google_apps_script_url"
#define OWNER_PHOTO    "url_to_your_face_photo"
```

> `config.h` is in `.gitignore` — your credentials will never be uploaded to GitHub.

### 2. Set up Google Apps Script

- Go to [script.google.com](https://script.google.com)
- Create a new project and deploy as a Web App
- Paste your script that sends Gmail on GET request
- Copy the deployment URL into `config.h` as `SCRIPT_URL`

### 3. Upload to ESP32

- Open `smart_mat_security_system.ino` in Arduino IDE
- Select your ESP32 board and COM port
- Upload the code
- Open Serial Monitor at **115200 baud**

### 4. Run

- Note the IP address shown in Serial Monitor
- Open `http://<IP>/scan` in your browser on the same WiFi network
- Keep the browser open — face scanning runs in the browser
- Step on the mat to trigger face recognition

---

## How It Works

```
Person steps on mat
        ↓
FSR sensors detect pressure → ESP32 sets matTriggered = true
        ↓
Browser polls /status every 1 second → detects TRIGGERED
        ↓
Browser starts camera → face-api.js loads models
        ↓
Face compared against owner photo
        ↓
Match (distance < 0.5) → /open → door animation + Gmail alert
No match              → /deny → access denied + Gmail alert
```

---

## Project Structure

```
smart-mat-security-system/
├── smart_mat_security_system.ino   # Main Arduino sketch
├── config.h.example                # Credentials template
├── config.h                        # Your credentials (gitignored)
├── .gitignore
└── README.md
```

---

## Future Improvements

- Add multiple authorized faces
- Store access logs to SD card
- Add LCD display for local status
- Pressure pattern biometrics using FSR array (unique footprint per person)
- MQTT integration for smart home systems

---

## Author

Aswin — ECE, VIT Vellore (Batch 2024)  
Registration No: 24BEC0525
