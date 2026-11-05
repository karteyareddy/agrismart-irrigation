/*
 * ESP32 - Smart Irrigation System (STABLE VERSION)
 * Uses polling instead of streaming to avoid SSL issues
 */

#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// ===== WiFi Credentials =====
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// ===== Firebase Credentials =====
#define API_KEY "YOUR_FIREBASE_WEB_API_KEY"
#define DATABASE_URL "https://YOUR-PROJECT-default-rtdb.firebaseio.com/"

// ===== Hardware Pins =====
const int SOIL_SENSOR_PIN = 34;
const int STATUS_LED_PIN = 2;

// ===== Serial Ports =====
#define DEBUG_SERIAL Serial
#define ARDUINO_SERIAL Serial2

// ===== Firebase Objects =====
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ===== Device Configuration =====
const String DEVICE_ID = "device_001";
const String BASE_PATH = "/devices/" + DEVICE_ID;

// ===== System State =====
struct SystemState {
  int moisture;
  String pumpStatus;
  String pumpMode;
  bool autoModeEnabled;
  int thresholdLow;
  int thresholdHigh;
  bool isReady;
  unsigned long lastSensorRead;
  unsigned long lastFirebaseRead;
  unsigned long lastFirebaseWrite;
  unsigned long lastAutoCheck;
  unsigned long lastWiFiCheck;
} state;

// ===== Timing Constants =====
const unsigned long SENSOR_READ_INTERVAL = 4000;
const unsigned long FIREBASE_READ_INTERVAL = 6000;   // Poll Firebase every 5 seconds
const unsigned long FIREBASE_WRITE_INTERVAL = 4000;  // Write to Firebase every 5 seconds
const unsigned long AUTO_CHECK_INTERVAL = 5000;
const unsigned long WIFI_CHECK_INTERVAL = 15000;

void setup() {
  // USB Serial for debugging
  DEBUG_SERIAL.begin(115200);
  
  // Serial2 for Arduino communication
  ARDUINO_SERIAL.begin(9600, SERIAL_8N1, 16, 17);
  
  delay(2000);
  
  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(SOIL_SENSOR_PIN, INPUT);
  
  // Initialize state
  state.moisture = 0;
  state.pumpStatus = "OFF";
  state.pumpMode = "AUTO";
  state.autoModeEnabled = true;
  state.thresholdLow = 30;
  state.thresholdHigh = 70;
  state.isReady = false;
  state.lastSensorRead = 0;
  state.lastFirebaseRead = 0;
  state.lastFirebaseWrite = 0;
  state.lastAutoCheck = 0;
  state.lastWiFiCheck = 0;
  
  DEBUG_SERIAL.println("\n========================================");
  DEBUG_SERIAL.println("  ESP32 IRRIGATION - STABLE MODE");
  DEBUG_SERIAL.println("  (Polling instead of streaming)");
  DEBUG_SERIAL.println("========================================\n");
  
  blinkLED(3);
  
  // Connect WiFi
  DEBUG_SERIAL.print("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 30) {
    delay(500);
    DEBUG_SERIAL.print(".");
    wifiAttempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    DEBUG_SERIAL.println("\n✓ WiFi Connected!");
    DEBUG_SERIAL.print("IP: ");
    DEBUG_SERIAL.println(WiFi.localIP());
    DEBUG_SERIAL.print("RSSI: ");
    DEBUG_SERIAL.println(WiFi.RSSI());
    blinkLED(2);
  } else {
    DEBUG_SERIAL.println("\n✗ WiFi Failed!");
    blinkLED(10);
    while(1) { delay(1000); }
  }
  
  // Configure Firebase
  DEBUG_SERIAL.println("\nInitializing Firebase...");
  
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  
  // Anonymous sign-up
  Firebase.signUp(&config, &auth, "", "");
  
  config.token_status_callback = tokenStatusCallback;
  
  // Simplified timeout settings
  config.timeout.serverResponse = 10000;
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  // Wait for Firebase
  DEBUG_SERIAL.print("Waiting for Firebase");
  int fbAttempts = 0;
  while (!Firebase.ready() && fbAttempts < 30) {
    delay(1000);
    DEBUG_SERIAL.print(".");
    fbAttempts++;
  }
  
  if (Firebase.ready()) {
    DEBUG_SERIAL.println("\n✓ Firebase Connected!");
    state.isReady = true;
    blinkLED(2);
    
    // Initialize device
    initializeDevice();
    
    // Load settings
    loadSettings();
    
  } else {
    DEBUG_SERIAL.println("\n✗ Firebase Failed!");
    blinkLED(8);
    while(1) { delay(1000); }
  }
  
  DEBUG_SERIAL.println("\n========================================");
  DEBUG_SERIAL.println("  SYSTEM READY");
  DEBUG_SERIAL.println("========================================\n");
  
  longBlinkLED(2);
  
  sendPumpCommand(state.pumpStatus);
}

void loop() {
  unsigned long now = millis();
  
  // Check WiFi
  if (now - state.lastWiFiCheck >= WIFI_CHECK_INTERVAL) {
    state.lastWiFiCheck = now;
    checkWiFi();
  }
  
  if (!state.isReady || WiFi.status() != WL_CONNECTED) {
    delay(100);
    return;
  }
  
  // Read sensor
  if (now - state.lastSensorRead >= SENSOR_READ_INTERVAL) {
    state.lastSensorRead = now;
    readSoilMoisture();
    quickBlink();
  }
  
  // Read from Firebase (polling)
  if (now - state.lastFirebaseRead >= FIREBASE_READ_INTERVAL) {
    state.lastFirebaseRead = now;
    readFromFirebase();
  }
  
  // Write to Firebase
  if (now - state.lastFirebaseWrite >= FIREBASE_WRITE_INTERVAL) {
    state.lastFirebaseWrite = now;
    writeToFirebase();
  }
  
  // Check AUTO mode
  if (now - state.lastAutoCheck >= AUTO_CHECK_INTERVAL) {
    state.lastAutoCheck = now;
    if (state.autoModeEnabled && state.pumpMode == "AUTO") {
      checkAutoMode();
    }
  }
  
  delay(10);
}

void initializeDevice() {
  DEBUG_SERIAL.println("Initializing device...");
  
  Firebase.RTDB.setString(&fbdo, (BASE_PATH + "/info/name").c_str(), "Field Sensor");
  Firebase.RTDB.setString(&fbdo, (BASE_PATH + "/info/device").c_str(), "ESP32");
  
  if (!Firebase.RTDB.getString(&fbdo, (BASE_PATH + "/actuators/pump/status").c_str())) {
    Firebase.RTDB.setString(&fbdo, (BASE_PATH + "/actuators/pump/status").c_str(), "OFF");
  }
  if (!Firebase.RTDB.getString(&fbdo, (BASE_PATH + "/actuators/pump/mode").c_str())) {
    Firebase.RTDB.setString(&fbdo, (BASE_PATH + "/actuators/pump/mode").c_str(), "AUTO");
  }
  if (!Firebase.RTDB.getBool(&fbdo, (BASE_PATH + "/settings/autoMode").c_str())) {
    Firebase.RTDB.setBool(&fbdo, (BASE_PATH + "/settings/autoMode").c_str(), true);
  }
  if (!Firebase.RTDB.getInt(&fbdo, (BASE_PATH + "/settings/thresholds/low").c_str())) {
    Firebase.RTDB.setInt(&fbdo, (BASE_PATH + "/settings/thresholds/low").c_str(), 30);
  }
  if (!Firebase.RTDB.getInt(&fbdo, (BASE_PATH + "/settings/thresholds/high").c_str())) {
    Firebase.RTDB.setInt(&fbdo, (BASE_PATH + "/settings/thresholds/high").c_str(), 70);
  }
  
  DEBUG_SERIAL.println("✓ Device initialized");
}

void loadSettings() {
  DEBUG_SERIAL.println("Loading settings...");
  
  if (Firebase.RTDB.getString(&fbdo, (BASE_PATH + "/actuators/pump/status").c_str())) {
    state.pumpStatus = fbdo.stringData();
    DEBUG_SERIAL.println("  Status: " + state.pumpStatus);
  }
  
  if (Firebase.RTDB.getString(&fbdo, (BASE_PATH + "/actuators/pump/mode").c_str())) {
    state.pumpMode = fbdo.stringData();
    DEBUG_SERIAL.println("  Mode: " + state.pumpMode);
  }
  
  if (Firebase.RTDB.getBool(&fbdo, (BASE_PATH + "/settings/autoMode").c_str())) {
    state.autoModeEnabled = fbdo.boolData();
    DEBUG_SERIAL.println("  Auto: " + String(state.autoModeEnabled ? "ON" : "OFF"));
  }
  
  if (Firebase.RTDB.getInt(&fbdo, (BASE_PATH + "/settings/thresholds/low").c_str())) {
    state.thresholdLow = fbdo.intData();
    DEBUG_SERIAL.println("  Low: " + String(state.thresholdLow) + "%");
  }
  
  if (Firebase.RTDB.getInt(&fbdo, (BASE_PATH + "/settings/thresholds/high").c_str())) {
    state.thresholdHigh = fbdo.intData();
    DEBUG_SERIAL.println("  High: " + String(state.thresholdHigh) + "%");
  }
  
  DEBUG_SERIAL.println("✓ Settings loaded");
}

void readFromFirebase() {
  static String lastStatus = "";
  static String lastMode = "";
  static bool lastAutoMode = true;
  
  // Read pump status
  if (Firebase.RTDB.getString(&fbdo, (BASE_PATH + "/actuators/pump/status").c_str())) {
    String newStatus = fbdo.stringData();
    if (newStatus != lastStatus) {
      DEBUG_SERIAL.println("\n[Firebase] Status changed: " + newStatus);
      lastStatus = newStatus;
      state.pumpStatus = newStatus;
      
      if (state.pumpMode == "MANUAL" || !state.autoModeEnabled) {
        sendPumpCommand(newStatus);
      }
    }
  }
  
  // Read pump mode
  if (Firebase.RTDB.getString(&fbdo, (BASE_PATH + "/actuators/pump/mode").c_str())) {
    String newMode = fbdo.stringData();
    if (newMode != lastMode) {
      DEBUG_SERIAL.println("[Firebase] Mode changed: " + newMode);
      lastMode = newMode;
      state.pumpMode = newMode;
      state.autoModeEnabled = (newMode == "AUTO");
      
      if (newMode == "MANUAL") {
        sendPumpCommand(state.pumpStatus);
      }
    }
  }
  
  // Read auto mode
  if (Firebase.RTDB.getBool(&fbdo, (BASE_PATH + "/settings/autoMode").c_str())) {
    bool newAutoMode = fbdo.boolData();
    if (newAutoMode != lastAutoMode) {
      DEBUG_SERIAL.println("[Firebase] AutoMode changed: " + String(newAutoMode ? "ON" : "OFF"));
      lastAutoMode = newAutoMode;
      state.autoModeEnabled = newAutoMode;
      state.pumpMode = newAutoMode ? "AUTO" : "MANUAL";
      
      if (!newAutoMode) {
        sendPumpCommand(state.pumpStatus);
      }
    }
  }
  
  // Read thresholds
  if (Firebase.RTDB.getInt(&fbdo, (BASE_PATH + "/settings/thresholds/low").c_str())) {
    state.thresholdLow = fbdo.intData();
  }
  
  if (Firebase.RTDB.getInt(&fbdo, (BASE_PATH + "/settings/thresholds/high").c_str())) {
    state.thresholdHigh = fbdo.intData();
  }
}

void writeToFirebase() {
  if (WiFi.status() != WL_CONNECTED || !Firebase.ready()) {
    return;
  }
  
  Firebase.RTDB.setInt(&fbdo, (BASE_PATH + "/sensors/moisture").c_str(), state.moisture);
  Firebase.RTDB.setString(&fbdo, (BASE_PATH + "/actuators/pump/status").c_str(), state.pumpStatus);
}

void checkWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    DEBUG_SERIAL.println("\n[WiFi] Connection LOST!");
    state.isReady = false;
    blinkLED(5);
    
    DEBUG_SERIAL.println("[WiFi] Reconnecting...");
    WiFi.disconnect();
    delay(1000);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      DEBUG_SERIAL.print(".");
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      DEBUG_SERIAL.println("\n[WiFi] ✓ Reconnected!");
      state.isReady = true;
      blinkLED(2);
    } else {
      DEBUG_SERIAL.println("\n[WiFi] ✗ Failed! Restarting...");
      delay(5000);
      ESP.restart();
    }
  }
}

void readSoilMoisture() {
  long sum = 0;
  for (int i = 0; i < 5; i++) {
    sum += analogRead(SOIL_SENSOR_PIN);
    delay(10);
  }
  int rawValue = sum / 5;
  
  state.moisture = map(rawValue, 4095, 0, 0, 100);
  state.moisture = constrain(state.moisture, 0, 100);
  
  DEBUG_SERIAL.println("[SENSOR] Moisture: " + String(state.moisture) + "%");
}

void checkAutoMode() {
  if (!state.autoModeEnabled || state.pumpMode != "AUTO") {
    return;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  
  if (state.moisture < state.thresholdLow && state.pumpStatus != "ON") {
    DEBUG_SERIAL.println("\n[AUTO] Moisture LOW → Turning pump ON");
    state.pumpStatus = "ON";
    sendPumpCommand("ON");
    Firebase.RTDB.setString(&fbdo, (BASE_PATH + "/actuators/pump/status").c_str(), "ON");
    blinkLED(1);
  }
  else if (state.moisture >= state.thresholdHigh && state.pumpStatus != "OFF") {
    DEBUG_SERIAL.println("\n[AUTO] Moisture HIGH → Turning pump OFF");
    state.pumpStatus = "OFF";
    sendPumpCommand("OFF");
    Firebase.RTDB.setString(&fbdo, (BASE_PATH + "/actuators/pump/status").c_str(), "OFF");
    blinkLED(2);
  }
}

void sendPumpCommand(String status) {
  ARDUINO_SERIAL.print("PUMP:");
  ARDUINO_SERIAL.println(status);
  DEBUG_SERIAL.println("→ Sending to Arduino: PUMP:" + status);
  delay(50);
}

void blinkLED(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(150);
    digitalWrite(STATUS_LED_PIN, LOW);
    delay(150);
  }
}

void longBlinkLED(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(500);
    digitalWrite(STATUS_LED_PIN, LOW);
    delay(500);
  }
}

void quickBlink() {
  digitalWrite(STATUS_LED_PIN, HIGH);
  delay(50);
  digitalWrite(STATUS_LED_PIN, LOW);
}

/*
 * ===== STABLE VERSION FEATURES =====
 * 
 * ✅ No Firebase Streaming (avoids SSL errors)
 * ✅ Simple polling every 5 seconds
 * ✅ Much more stable and reliable
 * ✅ Lower memory usage
 * ✅ Works with weak WiFi signals
 * ✅ Auto-restart on failures
 * 
 * ===== TRADE-OFFS =====
 * 
 * ⚠ 5 second delay for dashboard changes (vs instant with streaming)
 * ✅ But much more stable and no SSL errors!
 * 
 * ===== WIRING =====
 * 
 * ESP32 GPIO17 (TX2)  →  Arduino Pin 0 (RX)
 * ESP32 GPIO16 (RX2)  →  Arduino Pin 1 (TX)
 * ESP32 GND           →  Arduino GND
 * Sensor OUT          →  ESP32 GPIO34
 * 
 * ===== USAGE =====
 * 
 * Serial Monitor: 115200 baud
 * 
 * You should see:
 * [SENSOR] Moisture: 45%
 * → Sending to Arduino: PUMP:ON
 * [Firebase] Status changed: OFF
 */