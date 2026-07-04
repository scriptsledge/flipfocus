# FlipFocus: Physical IoT Productivity Tracker

FlipFocus is a physical user interface (PUI) designed to automate focus logging and reduce screen-based distractions. By mapping physical states on a desk accessory directly to productivity profiles, it acts as a tactile anchor for attention, helping users enter and maintain deep focus.

The ecosystem integrates an ESP32 microcontroller with a web visualizer through both a real-time cloud sync database and a high-speed offline USB serial connection.

## Key Features

* **Tactile Focus Logging**: Map active states (Assignment, Exam Prep, Core Coding, Break) with a physical potentiometer dial or orientation sensors.
* **Dual Telemetry Pipeline**:
  * *Cloud Sync Mode*: Real-time synchronization to a Firebase Realtime Database using WebSocket streams.
  * *Offline Local Mode*: High-speed, offline data logging using the browser Web Serial API over USB.
* **Embedded Signal Processing**: Non-blocking 64-sample averaging filter combined with a dynamic hysteresis transition algorithm to eliminate analog noise and boundary flickering.
* **Flash Memory Wear Mitigation**: Local Non-Volatile Storage (NVS) wear protection that utilizes batching, dirty-bit checking, and duration verification to extend ESP32 lifespan.
* **Blueprint Web Dashboard**: Responsive visualization dashboard built with Chart.js and Web Audio API synthetic alerts.

## Repository Structure

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

## System Requirements

* **Microcontroller**: ESP32 (NodeMCU Development Board)
* **Peripherals**: Rotary Potentiometer (10k Ohm), MPU6050 IMU, I2C 16x2 LCD Display, Active Buzzer, LED
* **Dependencies**: Firebase ESP Client, Preferences, Wire, LiquidCrystal_I2C, Chart.js
* **Browser**: Chrome, Edge, or any browser supporting Web Serial API (for Local USB Mode)

## Getting Started

### 1. Firmware Configuration
1. Open [firmware/main/main.ino](file:///home/pyzard/projects/flipfocus/firmware/main/main.ino) in the Arduino IDE.
2. Create a file named `arduino_secrets.h` inside the `firmware/main/` directory:
   ```cpp
   #define WIFI_SSID "Your_WiFi_SSID"
   #define WIFI_PASSWORD "Your_WiFi_Password"
   #define API_KEY "Your_Firebase_API_Key"
   #define DATABASE_URL "Your_Firebase_Database_URL"
   ```
3. Compile and flash the firmware to your ESP32 board.

### 2. Frontend Dashboards
* **Cloud Mode**: Access the live visualizer deployed on Vercel at `https://flipfocusiot.vercel.app/` or run [web/index.html](file:///home/pyzard/projects/flipfocus/web/index.html) locally.
* **Local USB Mode**: Connect your ESP32 to your computer via USB. Open [web/local.html](file:///home/pyzard/projects/flipfocus/web/local.html) in Chrome or Edge, click **Connect USB**, and select the ESP32 serial port.
