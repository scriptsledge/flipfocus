/*
 * FlipFocus Firmware
 * Target: ESP32 DevKit
 * Sensors: MPU6050
 */

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

Adafruit_MPU6050 mpu;

void setup() {
  Serial.begin(115200);
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) yield();
  }
  Serial.println("MPU6050 Found!");
}

void loop() {
  // Logic from Blueprint index.html
  checkOrientation();
  delay(500);
}

void checkOrientation() {
  sensors_event_t a, g, t;
  mpu.getEvent(&a, &g, &t);

  String state = "";
  if (a.acceleration.z > 9.5) state = "SIDE 1: WORK";
  else if (a.acceleration.z < -9.5) state = "SIDE 2: REST";
  else if (a.acceleration.y > 9.5) state = "SIDE 3: CODE";
  else if (a.acceleration.y < -9.5) state = "SIDE 4: READ";
  else if (a.acceleration.x > 9.5) state = "SIDE 5: DRAW";
  else if (a.acceleration.x < -9.5) state = "SIDE 6: MAIL";

  if (state != "") {
    Serial.println("Active: " + state);
    // updateCloud(state);
  }
}
