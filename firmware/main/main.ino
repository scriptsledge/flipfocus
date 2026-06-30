#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Firebase_ESP_Client.h>
#include <Preferences.h>
#include <time.h>

// NTP Settings for IST (India Standard Time: UTC+5:30)
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 19800; // 5.5 * 3600
const int   daylightOffset_sec = 0;

// Secret credentials - THIS FILE IS NOT COMMITTED TO GITHUB
#include "arduino_secrets.h"

// Provide useful error messages if secrets are missing
#ifndef WIFI_SSID
  #error "Please create an arduino_secrets.h file with your WIFI_SSID, WIFI_PASSWORD, API_KEY and DATABASE_URL"
#endif

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

LiquidCrystal_I2C lcd(0x27, 16, 2);
Preferences preferences;

const int potPin = 34;    
const int buzzerPin = 13; 
const int ledPin = 14;    

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
FirebaseJson json; // Used for batch updates

unsigned long accumulatedTime[5] = {0, 0, 0, 0, 0}; 
unsigned long zoneStartTime = 0;
int currentZone = 0;
int lastZone = -1;
int lastResetDay = 0;
bool isOfflineMode = false;

// Non-blocking timers
unsigned long lastSyncTime = 0;
const unsigned long syncInterval = 3000; 
unsigned long lastNVSSave = 0;           // Timer for flash memory protection
const unsigned long nvsSaveInterval = 300000; // Save to flash once every 5 minutes (300000ms)
unsigned long lastTimeCheck = 0; // For NTP check
unsigned long buzzerStartTime = 0;
bool buzzerActive = false;

int currentPotValue = 0;

int readPotentiometer() {
  long sum = 0;
  const int samples = 64;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(potPin);
    delayMicroseconds(50);
  }
  return sum / samples;
}

int getZoneFromADC(int rawValue, int currentActiveZone) {
  if (currentActiveZone == 0) {
    if (rawValue < 700) return 1;
    if (rawValue < 2380) return 2;
    if (rawValue < 4050) return 3;
    return 4;
  }

  // Hysteresis logic to prevent jitter
  switch (currentActiveZone) {
    case 1:
      if (rawValue > 775) return 2;
      break;
    case 2:
      if (rawValue < 625) return 1;
      if (rawValue > 2455) return 3;
      break;
    case 3:
      if (rawValue < 2305) return 2;
      if (rawValue > 4050) return 4;
      break;
    case 4:
      if (rawValue < 4000) return 3;
      break;
  }
  return currentActiveZone;
}

// Custom LCD Emojis/Characters (5x8 Bitmaps)
byte wifiConnected[8] = {
  B00000,
  B00001,
  B00001,
  B00011,
  B00011,
  B00111,
  B00111,
  B11111
};

byte wifiDisconnected[8] = {
  B10001,
  B01010,
  B00100,
  B01010,
  B10001,
  B00000,
  B11111,
  B00000
};

byte lockChar[8] = {
  B01110,
  B10001,
  B10001,
  B11111,
  B11011,
  B11011,
  B11111,
  B00000
};

byte breakChar[8] = {
  B01010,
  B01010,
  B11110,
  B10001,
  B10001,
  B11110,
  B00000,
  B00000
};

void triggerPingResponse() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("PING RECEIVED!  ");
  lcd.setCursor(0, 1);
  lcd.print("by team: overAI ");

  // Play tu-tu-tuuu melody
  // tu
  digitalWrite(buzzerPin, HIGH);
  digitalWrite(ledPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
  digitalWrite(ledPin, LOW);
  delay(80);

  // tu
  digitalWrite(buzzerPin, HIGH);
  digitalWrite(ledPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
  digitalWrite(ledPin, LOW);
  delay(80);

  // tuuu
  digitalWrite(buzzerPin, HIGH);
  digitalWrite(ledPin, HIGH);
  delay(350);
  digitalWrite(buzzerPin, LOW);
  digitalWrite(ledPin, LOW);

  delay(2000); // Let user read screen
  lcd.clear();
  updateLCD();
}

void triggerAlarmResponse() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  TIME'S UP!    ");
  lcd.setCursor(0, 1);
  lcd.print("by team: overAI ");

  // Play alarm buzzer tune: 4 short, 1 long
  for (int i = 0; i < 4; i++) {
    digitalWrite(buzzerPin, HIGH);
    digitalWrite(ledPin, HIGH);
    delay(80);
    digitalWrite(buzzerPin, LOW);
    digitalWrite(ledPin, LOW);
    delay(50);
  }
  digitalWrite(buzzerPin, HIGH);
  digitalWrite(ledPin, HIGH);
  delay(350);
  digitalWrite(buzzerPin, LOW);
  digitalWrite(ledPin, LOW);

  delay(2000); // Let user read screen
  lcd.clear();
  updateLCD();
}

void setup() {
  Serial.begin(115200);
  
  // Load saved times and last reset date from Non-Volatile Storage
  preferences.begin("pomodoro", false);
  accumulatedTime[1] = preferences.getUInt("p1", 0);
  accumulatedTime[2] = preferences.getUInt("p2", 0);
  accumulatedTime[3] = preferences.getUInt("p3", 0);
  accumulatedTime[4] = preferences.getUInt("p4", 0);
  lastResetDay = preferences.getInt("lrd", 0);
  preferences.end();
  Serial.println("Previous progress loaded from Flash.");

  pinMode(potPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);

  lcd.init();
  lcd.backlight();

  // Register custom characters
  lcd.createChar(0, wifiConnected);
  lcd.createChar(1, wifiDisconnected);
  lcd.createChar(2, lockChar);
  lcd.createChar(3, breakChar);

  lcd.setCursor(0, 0);
  lcd.print("CONNECTING WIFI ");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  // 4-Second Timeout Loop
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 4000) {
    delay(200);
    Serial.print(".");
  }
  
  lcd.clear();
  if (WiFi.status() == WL_CONNECTED) {
    lcd.print("WIFI CONNECTED!");
    isOfflineMode = false;
    
    // Initialize Time and Firebase ONLY if connected
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;
    Firebase.signUp(&config, &auth, "", "");
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
  } else {
    lcd.print("OFFLINE MODE ON ");
    isOfflineMode = true;
    
    // Just minimal config for later
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
  }
  delay(1000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  FLIPFOCUS v1  ");
  lcd.setCursor(0, 1);
  lcd.print(isOfflineMode ? " LOCAL TRACKING" : " CLOUD SYNC LIVE");
  delay(1500);
  lcd.clear();
  
  zoneStartTime = millis(); 
  updateLCD();
}

void loop() {
  // Update offline status dynamically
  bool wasOffline = isOfflineMode;
  isOfflineMode = (WiFi.status() != WL_CONNECTED);

  // If we just reconnected online, re-run NTP config to sync time
  if (wasOffline && !isOfflineMode) {
    Serial.println("Reconnected online. Initializing NTP...");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  }

  // Check for day change every 60 seconds if online
  if (!isOfflineMode && (millis() - lastTimeCheck > 60000)) {
    checkForDayReset();
    lastTimeCheck = millis();
  }

  currentPotValue = readPotentiometer();
  int newZone = getZoneFromADC(currentPotValue, currentZone);

  // Show the value of potentiometer in serial monitor reading to adjust values for each profile
  static unsigned long lastPotPrintTime = 0;
  if (millis() - lastPotPrintTime > 500) {
    Serial.print("POT_VAL: ");
    Serial.print(currentPotValue);
    Serial.print(" -> Active Zone: ");
    Serial.println(currentZone);
    lastPotPrintTime = millis();
  }

  if (newZone != currentZone) {
    unsigned long timeBefore = (currentZone >= 1 && currentZone <= 4) ? accumulatedTime[currentZone] : 0;
    updateAccumulatedTime(); // Move progress from current zone to array
    bool timeChanged = (currentZone >= 1 && currentZone <= 4) && (accumulatedTime[currentZone] != timeBefore);

    currentZone = newZone;
    zoneStartTime = millis();

    // Trigger non-blocking buzzer/LED
    digitalWrite(buzzerPin, HIGH);
    digitalWrite(ledPin, HIGH);
    buzzerStartTime = millis();
    buzzerActive = true;

    pushMetricsToCloud(); 
    if (timeChanged) {
      saveToNVS(); // Only write to flash if actual time (>= 1 second) was accumulated!
    }
  }

  // Handle non-blocking buzzer shutoff
  if (buzzerActive && (millis() - buzzerStartTime > 80)) {
    digitalWrite(buzzerPin, LOW);
    digitalWrite(ledPin, LOW);
    buzzerActive = false;
  }
  
  // Update LCD every 500ms for a "live" timer effect
  static unsigned long lastLCDUpdate = 0;
  if (millis() - lastLCDUpdate > 500) {
    updateLCD();
    lastLCDUpdate = millis();
  }

  // Periodic background sync (to Cloud & Serial)
  if (millis() - lastSyncTime > syncInterval) {
    pushMetricsToCloud();
    lastSyncTime = millis();
  }

  // Periodic flash memory save (every 60s)
  if (millis() - lastNVSSave > nvsSaveInterval) {
    updateAccumulatedTime(); // Transfer pending seconds before saving
    saveToNVS();
    lastNVSSave = millis();
  }

  // Check Serial commands (Local USB Mode)
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "ping") {
      triggerPingResponse();
    } else if (cmd == "alarm") {
      triggerAlarmResponse();
    }
  }

  // Check Firebase trigger (Cloud Mode)
  static unsigned long lastTriggerCheck = 0;
  if (!isOfflineMode && Firebase.ready() && (millis() - lastTriggerCheck > 1500)) {
    lastTriggerCheck = millis();
    int val = 0;
    if (Firebase.RTDB.getInt(&fbdo, "/profiles/triggerPing", &val)) {
      if (val == 1) {
        triggerPingResponse();
        Firebase.RTDB.setInt(&fbdo, "/profiles/triggerPing", 0);
      } else if (val == 2) {
        triggerAlarmResponse();
        Firebase.RTDB.setInt(&fbdo, "/profiles/triggerPing", 0);
      }
    }
  }
}

void checkForDayReset() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }

  int currentDay = timeinfo.tm_mday;
  
  // If lastResetDay is 0, this is likely the first time setting up
  if (lastResetDay == 0) {
    lastResetDay = currentDay;
    preferences.begin("pomodoro", false);
    preferences.putInt("lrd", lastResetDay);
    preferences.end();
    return;
  }

  // If the day has changed, reset everything
  if (currentDay != lastResetDay) {
    Serial.println("NEW DAY DETECTED! Resetting all profiles.");
    
    for (int i = 1; i <= 4; i++) {
      accumulatedTime[i] = 0;
    }
    
    lastResetDay = currentDay;
    zoneStartTime = millis();
    
    saveToNVS(); // Save the reset values and the new day
    pushMetricsToCloud(); // Sync the reset to Firebase immediately
    updateLCD();
  }
}

void saveToNVS() {
  preferences.begin("pomodoro", false);
  preferences.putUInt("p1", accumulatedTime[1]);
  preferences.putUInt("p2", accumulatedTime[2]);
  preferences.putUInt("p3", accumulatedTime[3]);
  preferences.putUInt("p4", accumulatedTime[4]);
  preferences.putInt("lrd", lastResetDay);
  preferences.end();
}

void updateAccumulatedTime() {
  if (currentZone != 0) {
    unsigned long currentTime = millis();
    unsigned long durationSpentSeconds = (currentTime - zoneStartTime) / 1000;
    
    if (durationSpentSeconds > 0) {
      accumulatedTime[currentZone] += durationSpentSeconds;
      // Subtract the whole seconds we just added from zoneStartTime
      // to keep the fractional milliseconds for the next check
      zoneStartTime += (durationSpentSeconds * 1000);
    }
  }
}

void updateLCD() {
  unsigned long currentTime = millis();
  unsigned long sessionSeconds = (currentTime - zoneStartTime) / 1000;
  unsigned long totalSeconds = accumulatedTime[currentZone] + sessionSeconds;
  
  unsigned int hrs = totalSeconds / 3600;
  unsigned int mins = (totalSeconds % 3600) / 60;
  unsigned int secs = totalSeconds % 60;

  lcd.setCursor(0, 0);
  switch (currentZone) {
    case 1: lcd.print("1. ASSIGNMENT  "); break;
    case 2: lcd.print("2. EXAM PREP   "); break;
    case 3: lcd.print("3. CORE CODING "); break;
    case 4: lcd.print("4. TOTAL BREAK "); break;
    default: lcd.print("SELECT PROFILE "); break;
  }

  // Draw WiFi status at top-right (col 15, row 0)
  lcd.setCursor(15, 0);
  if (isOfflineMode) {
    lcd.write(1); // wifiDisconnected
  } else {
    lcd.write(0); // wifiConnected
  }

  lcd.setCursor(0, 1);
  lcd.print("T: ");
  if (hrs < 10) lcd.print("0");
  lcd.print(hrs);
  lcd.print(":");
  if (mins < 10) lcd.print("0");
  lcd.print(mins);
  lcd.print(":");
  if (secs < 10) lcd.print("0");
  lcd.print(secs);
  
  // Show lock or coffee cup icon at col 15, row 1
  lcd.setCursor(15, 1);
  if (currentZone == 4) {
    lcd.write(3); // coffee cup (break)
  } else if (currentZone >= 1 && currentZone <= 3) {
    lcd.write(2); // lock (focus)
  } else {
    lcd.print(" ");
  }
}

void pushMetricsToCloud() {
  if (Firebase.ready() && WiFi.status() == WL_CONNECTED) {
    unsigned long currentTime = millis();
    unsigned long dynamicDuration = (currentTime - zoneStartTime) / 1000;
    
    // Calculate live times for all zones
    unsigned long liveZoneTime[5];
    for(int i = 1; i <= 4; i++) {
      liveZoneTime[i] = accumulatedTime[i];
    }
    liveZoneTime[currentZone] += dynamicDuration;

    // BATCH UPDATE: Prepare all data in one JSON object
    json.clear();
    json.set("assignment", liveZoneTime[1]);
    json.set("examprep", liveZoneTime[2]);
    json.set("corecoding", liveZoneTime[3]);
    json.set("totalbreak", liveZoneTime[4]);
    json.set("activeZone", currentZone);

    // Send everything in ONE network request
    if (!Firebase.RTDB.updateNode(&fbdo, "/profiles", &json)) {
      Serial.println(fbdo.errorReason());
    }
  }

  // LOCAL USB FALLBACK: Always print data to Serial in JSON format
  // This allows the local-web.html dashboard to work without internet
  unsigned long currentTime = millis();
  unsigned long sessionSeconds = (currentTime - zoneStartTime) / 1000;
  
  String jsonOutput = "{";
  jsonOutput += "\"assignment\":" + String(accumulatedTime[1] + (currentZone == 1 ? sessionSeconds : 0)) + ",";
  jsonOutput += "\"examprep\":" + String(accumulatedTime[2] + (currentZone == 2 ? sessionSeconds : 0)) + ",";
  jsonOutput += "\"corecoding\":" + String(accumulatedTime[3] + (currentZone == 3 ? sessionSeconds : 0)) + ",";
  jsonOutput += "\"totalbreak\":" + String(accumulatedTime[4] + (currentZone == 4 ? sessionSeconds : 0)) + ",";
  jsonOutput += "\"activeZone\":" + String(currentZone) + ",";
  jsonOutput += "\"potentiometer\":" + String(currentPotValue);
  jsonOutput += "}";
  
  Serial.println(jsonOutput);
}
