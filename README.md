# FlipFocus IoT - Physical Productivity Tracker

[![GitHub Repo](https://img.shields.io/badge/GitHub-Repository-blue?logo=github)](https://github.com/scriptsledge/flipfocus)

## Project Overview
FlipFocus is a physical cube designed to track focus time without phone distractions. By flipping the cube to a specific face, an ESP32 detects the orientation via an MPU6050 accelerometer and syncs the task timer to a cloud dashboard.

## Folder Structure
- `firmware/`: ESP32 source code (C++/Arduino).
- `hardware/`: 3D models (STL/STEP) and wiring diagrams.
- `docs/`: Technical specifications and project logs.
- `web/`: Professional documentation and dashboard website (deployed on Vercel).

## Technical Stack
- **Hardware**: ESP32, MPU6050, Li-ion battery (TP4056).
- **Communication**: WiFi (MQTT or REST API).
- **Web**: TailwindCSS, Chart.js (Vercel deployment).

## Setup
1. **Firmware**: Open `firmware/` in PlatformIO or Arduino IDE.
2. **Web**: Run a local server in `web/` or push to GitHub for automatic Vercel deployment.
   - *Tip*: On Vercel, set the **Root Directory** to `web/`.
