# 🎯 FlipFocus
> **A Physical IoT Productivity Tracker & Real-Time Visualization Ecosystem**

[![GitHub Repo](https://img.shields.io/badge/GitHub-Repository-blue?logo=github)](https://github.com/scriptsledge/flipfocus)
[![Vercel Deployment](https://img.shields.io/badge/Vercel-Deployed-brightgreen?logo=vercel)](https://flipfocus.vercel.app)

FlipFocus is a physical-first user interface (PUI) designed to automate focus logging and minimize screen-based distractions. By mapping physical states on a desk accessory directly to productivity profiles, it acts as a tactile anchor for attention, helping users enter and maintain deep focus.

---

## ✨ Key Features
* 🎛️ **Tactile Control**: Map focus states (Assignment, Exam Prep, Core Coding, Break) with a physical potentiometer dial or orientation sensors.
* 📶 **Dual Telemetry Pipeline**:
  * **Cloud Sync Mode**: Real-time websocket synchronization to a Firebase database.
  * **Offline Local Mode**: High-speed, offline data logging using the browser **Web Serial API** over USB.
* 🧠 **Signal Debouncing**: Non-blocking 64-sample averaging filter combined with a dynamic hysteresis transition algorithm to eliminate analog noise and flickering.
* 💾 **NVS Wear Protection**: Batching, dirty-bit checks, and duration verification algorithms that reduce ESP32 flash write cycles by over 98% to extend hardware life.
* 📊 **Blueprint Dashboard**: Minimalist dark visualizer utilizing **Chart.js** for data analysis and the **Web Audio API** for synthesized Pomodoro alerts.

---

## 🛠️ Repository Structure
```text
├── firmware/
│   └── main/
│       ├── main.ino            # ESP32 C++ firmware execution loop
│       └── arduino_secrets.h   # Local Wi-Fi & Firebase credentials (ignored)
├── web/
│   ├── index.html              # Cloud-synchronized dashboard (deployed on Vercel)
│   └── local.html              # Offline Web Serial USB dashboard
└── docs/
    ├── report-flipfocus.pdf    # Final minor project report (PDF)
    ├── report-flipfocus.docx   # Final minor project report (Word)
    ├── presentation.pptx       # Minor project presentation slide deck
    └── synosis.pdf             # Project synopsis
```

---

## 🚀 Quick Setup

### 1. Hardware Firmware Configuration
1. Open the [firmware/main/main.ino](file:///home/pyzard/projects/flipfocus/firmware/main/main.ino) file in the Arduino IDE.
2. Create a file named `arduino_secrets.h` inside the `firmware/main/` folder:
   ```cpp
   #define WIFI_SSID "Your_WiFi_SSID"
   #define WIFI_PASSWORD "Your_WiFi_Password"
   #define API_KEY "Your_Firebase_API_Key"
   #define DATABASE_URL "Your_Firebase_Database_URL"
   ```
3. Compile and flash the code to your ESP32 board.

### 2. Frontend Dashboards
* **Cloud Mode**: Open [web/index.html](file:///home/pyzard/projects/flipfocus/web/index.html) or visit the live Vercel deployment.
* **Local Mode**: Connect your ESP32 via USB, open [web/local.html](file:///home/pyzard/projects/flipfocus/web/local.html) in Chrome/Edge, click **Connect USB**, and select the ESP32 serial port.

---

## 🔬 Core Technology Stack
* **Microcontroller**: ESP32 (NodeMCU)
* **Embedded Libraries**: Preferences (NVS), Wire (I2C), LiquidCrystal_I2C, Firebase ESP Client
* **Cloud Infrastructure**: Firebase Realtime Database (RTDB)
* **Frontend Technologies**: HTML5 (Web Serial, Web Audio APIs), Vanilla CSS, JavaScript (ES6), Chart.js
