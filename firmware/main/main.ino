#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Firebase_ESP_Client.h>

// Secret credentials - THIS FILE IS NOT COMMITTED TO GITHUB
#include "arduino_secrets.h"

// Provide useful error messages if secrets are missing
#ifndef WIFI_SSID
  #error "Please create an arduino_secrets.h file with your WIFI_SSID, WIFI_PASSWORD, API_KEY and DATABASE_URL"
#endif

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int potPin = 34;    
const int buzzerPin = 13; 
const int ledPin = 14;    

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long accumulatedTime[5] = {0, 0, 0, 0, 0}; 
unsigned long zoneStartTime = 0;
int currentZone = 0;
int lastZone = -1;
bool signupOK = false;

void setup() {
  Serial.begin(115200);
  
  pinMode(potPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("CONNECTING WIFI ");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }
  
  lcd.clear();
  lcd.print("WIFI CONNECTED!");
  delay(1000);

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase Auth Token Verification: SUCCESS");
    signupOK = true;
  } else {
    Serial.printf("Firebase Registration Error: %s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  FLIPFOCUS v1  ");
  lcd.setCursor(0, 1);
  lcd.print(" CLOUD SYNC LIVE");
  delay(2000);
  lcd.clear();
  
  zoneStartTime = millis(); 
}

void loop() {
  int rawValue = analogRead(potPin);
  
  if (rawValue < 1000) currentZone = 1;
  else if (rawValue >= 1000 && rawValue < 2000) currentZone = 2;
  else if (rawValue >= 2000 && rawValue < 3000) currentZone = 3;
  else currentZone = 4;

  if (currentZone != lastZone) {
    unsigned long currentTime = millis();
    
    if (lastZone != -1) {
      unsigned long durationSpentSeconds = (currentTime - zoneStartTime) / 1000;
      accumulatedTime[lastZone] += durationSpentSeconds;
    }

    zoneStartTime = currentTime;

    digitalWrite(buzzerPin, HIGH);
    digitalWrite(ledPin, HIGH);
    delay(60);
    digitalWrite(buzzerPin, LOW);
    digitalWrite(ledPin, LOW);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ACTIVE PROFILE:");
    lcd.setCursor(0, 1);

    // UPDATED PROFILE LABELS FOR THE HARDWARE LCD SCREEN
    switch (currentZone) {
      case 1: lcd.print("1. ASSIGNMENT"); break;
      case 2: lcd.print("2. EXAM PREP"); break;
      case 3: lcd.print("3. CORE CODING"); break;
      case 4: lcd.print("4. TOTAL BREAK"); break;
    }

    pushMetricsToCloud();
    lastZone = currentZone;
  }
  
  static unsigned long lastSyncTime = 0;
  if (millis() - lastSyncTime > 3000) {
    pushMetricsToCloud();
    lastSyncTime = millis();
  }
  
  delay(100); 
}

void pushMetricsToCloud() {
  if (Firebase.ready() && signupOK) {
    unsigned long currentTime = millis();
    unsigned long dynamicDuration = (currentTime - zoneStartTime) / 1000;
    
    unsigned long liveZoneTime[5];
    for(int i = 1; i <= 4; i++) {
      liveZoneTime[i] = accumulatedTime[i];
    }
    liveZoneTime[currentZone] += dynamicDuration;

    // Maps directly to our dashboard structure paths
    Firebase.RTDB.setInt(&fbdo, "/profiles/assignment", liveZoneTime[1]);
    Firebase.RTDB.setInt(&fbdo, "/profiles/examprep", liveZoneTime[2]);
    Firebase.RTDB.setInt(&fbdo, "/profiles/corecoding", liveZoneTime[3]);
    Firebase.RTDB.setInt(&fbdo, "/profiles/totalbreak", liveZoneTime[4]);
    Firebase.RTDB.setInt(&fbdo, "/profiles, activeZone", currentZone);
  }
}
