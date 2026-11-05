/*
 * ESP32 Smart Irrigation System - FIXED VERSION
 * Two-way communication with real-time sync
 * Converted from ESP8266 to ESP32
 */

#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

// ===== CONFIGURATION =====
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define API_KEY "YOUR_FIREBASE_WEB_API_KEY"
#define DATABASE_URL "https://YOUR-PROJECT-default-rtdb.firebaseio.com/"
#define DEVICE_ID "device_001"

// ===== PIN CONFIGURATION (ESP32) =====
const int SOIL_SENSOR_PIN = 34;  // ADC1_CH6 (VP pin) - analog input
const int PUMP_RELAY_PIN = 25;   // GPIO25 - relay control
const int STATUS_LED_PIN = 2;    // GPIO2 - built-in LED on most ESP32 boards

// ===== TIMING CONSTANTS =====
const unsigned long SENSOR_READ_INTERVAL = 5000;    // 5 seconds
const unsigned long FIREBASE_SYNC_INTERVAL = 3000;  // 3 seconds
const unsigned long HEARTBEAT_INTERVAL = 30000;     // 30 seconds
const unsigned long PUMP_DEBOUNCE_TIME = 2000;      // 2 seconds - prevent rapid toggling

// ===== GLOBAL OBJECTS =====
FirebaseData fbdo;
FirebaseData streamData;
FirebaseAuth auth;
FirebaseConfig config;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 60000); // IST offset

// ===== STATE VARIABLES =====
struct DeviceState {
  int moisture;
  String pumpStatus;
  String pumpMode;
  int thresholdLow;
  int thresholdHigh;
  bool autoMode;
  unsigned long lastSensorRead;
  unsigned long lastFirebaseSync;
  unsigned long lastHeartbeat;
  unsigned long lastPumpChange;
  bool firebaseInitialized;
} state;

bool signupOK = false;

// ===== FUNCTION DECLARATIONS =====
void connectWiFi();
void initFirebase();
void setupFirebaseStream();
void readInitialState();
void streamCallback(FirebaseStream data);
void streamTimeoutCallback(bool timeout);
String getTimestamp();
void readSoilMoisture();
void controlPump();
void setPumpState(String newStatus, String trigger);
void syncToFirebase();
void sendHeartbeat();
void logToHistory(String type, String value);
void blinkStatusLED(int times);

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Initialize pins - ENSURE PUMP IS OFF ON BOOT
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(PUMP_RELAY_PIN, LOW);  // CRITICAL: Start with pump OFF
  digitalWrite(STATUS_LED_PIN, LOW);
  
  // Configure ADC for soil sensor (ESP32 specific)
  analogReadResolution(12);  // ESP32 has 12-bit ADC (0-4095)
  analogSetAttenuation(ADC_11db);  // Full range: 0-3.3V
  
  // Initialize state with safe defaults
  state.moisture = 0;
  state.pumpStatus = "OFF";
  state.pumpMode = "AUTO";
  state.thresholdLow = 30;
  state.thresholdHigh = 70;
  state.autoMode = true;
  state.lastSensorRead = 0;
  state.lastFirebaseSync = 0;
  state.lastHeartbeat = 0;
  state.lastPumpChange = 0;
  state.firebaseInitialized = false;
  
  Serial.println("\n========================================");
  Serial.println("  Smart Irrigation System - ESP32");
  Serial.println("========================================\n");
  
  connectWiFi();
  timeClient.begin();
  initFirebase();
  
  // Read initial state from Firebase before starting stream
  readInitialState();
  
  setupFirebaseStream();
  
  Serial.println("✅ System Ready!\n");
  Serial.println("📊 Initial State:");
  Serial.println("   Pump: " + state.pumpStatus);
  Serial.println("   Mode: " + state.pumpMode);
  Serial.println("   Auto: " + String(state.autoMode ? "ON" : "OFF"));
  Serial.println("   Thresholds: " + String(state.thresholdLow) + "% - " + String(state.thresholdHigh) + "%\n");
  
  blinkStatusLED(3);
}

// ===== MAIN LOOP =====
void loop() {
  unsigned long currentMillis = millis();
  
  // Read sensor periodically
  if (currentMillis - state.lastSensorRead >= SENSOR_READ_INTERVAL) {
    readSoilMoisture();
    state.lastSensorRead = currentMillis;
  }
  
  // Control pump based on mode (only if Firebase is initialized)
  if (state.firebaseInitialized) {
    controlPump();
  }
  
  // Sync to Firebase periodically
  if (currentMillis - state.lastFirebaseSync >= FIREBASE_SYNC_INTERVAL) {
    syncToFirebase();
    state.lastFirebaseSync = currentMillis;
  }
  
  // Send heartbeat
  if (currentMillis - state.lastHeartbeat >= HEARTBEAT_INTERVAL) {
    sendHeartbeat();
    state.lastHeartbeat = currentMillis;
  }
  
  // Update time
  timeClient.update();
  
  delay(100);
}

// ===== WIFI CONNECTION =====
void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.mode(WIFI_STA);  // ESP32 specific - set station mode
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal Strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("\n❌ WiFi Connection Failed!");
    ESP.restart();
  }
}

// ===== FIREBASE INITIALIZATION =====
void initFirebase() {
  Serial.println("\nInitializing Firebase...");
  
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  
  // Anonymous sign-in
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("✅ Firebase Authentication Successful");
    signupOK = true;
  } else {
    Serial.printf("❌ Firebase Auth Failed: %s\n", config.signer.signupError.message.c_str());
  }
  
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  // Wait for Firebase to be ready
  int retries = 0;
  while (!Firebase.ready() && retries < 30) {
    delay(100);
    retries++;
  }
  
  // Initialize device structure if not exists
  if (Firebase.ready() && signupOK) {
    String devicePath = "/devices/" + String(DEVICE_ID);
    
    // Set device info
    Firebase.RTDB.setString(&fbdo, devicePath + "/info/name", "Main Field Sensor");
    Firebase.RTDB.setString(&fbdo, devicePath + "/info/location", "Field A");
    Firebase.RTDB.setString(&fbdo, devicePath + "/info/device", "ESP32");
    Firebase.RTDB.setString(&fbdo, devicePath + "/info/lastSeen", getTimestamp());
    
    // Check if settings exist, if not initialize with defaults
    if (!Firebase.RTDB.getInt(&fbdo, devicePath + "/settings/thresholds/low")) {
      Firebase.RTDB.setInt(&fbdo, devicePath + "/settings/thresholds/low", state.thresholdLow);
    }
    if (!Firebase.RTDB.getInt(&fbdo, devicePath + "/settings/thresholds/high")) {
      Firebase.RTDB.setInt(&fbdo, devicePath + "/settings/thresholds/high", state.thresholdHigh);
    }
    if (!Firebase.RTDB.getBool(&fbdo, devicePath + "/settings/autoMode")) {
      Firebase.RTDB.setBool(&fbdo, devicePath + "/settings/autoMode", state.autoMode);
    }
    
    // Check if actuators exist, if not initialize
    if (!Firebase.RTDB.getString(&fbdo, devicePath + "/actuators/pump/status")) {
      Firebase.RTDB.setString(&fbdo, devicePath + "/actuators/pump/status", "OFF");
      Firebase.RTDB.setString(&fbdo, devicePath + "/actuators/pump/mode", "AUTO");
      Firebase.RTDB.setString(&fbdo, devicePath + "/actuators/pump/lastChanged", getTimestamp());
    }
    
    Serial.println("✅ Device structure initialized");
    state.firebaseInitialized = true;
  }
}

// ===== READ INITIAL STATE FROM FIREBASE =====
void readInitialState() {
  if (!Firebase.ready() || !signupOK) {
    Serial.println("⚠ Firebase not ready, using default state");
    return;
  }
  
  String devicePath = "/devices/" + String(DEVICE_ID);
  
  Serial.println("📥 Reading initial state from Firebase...");
  
  // Read pump status
  if (Firebase.RTDB.getString(&fbdo, devicePath + "/actuators/pump/status")) {
    state.pumpStatus = fbdo.stringData();
    digitalWrite(PUMP_RELAY_PIN, (state.pumpStatus == "ON") ? HIGH : LOW);
    Serial.println("   Pump Status: " + state.pumpStatus);
  }
  
  // Read pump mode
  if (Firebase.RTDB.getString(&fbdo, devicePath + "/actuators/pump/mode")) {
    state.pumpMode = fbdo.stringData();
    Serial.println("   Pump Mode: " + state.pumpMode);
  }
  
  // Read thresholds
  if (Firebase.RTDB.getInt(&fbdo, devicePath + "/settings/thresholds/low")) {
    state.thresholdLow = fbdo.intData();
    Serial.println("   Low Threshold: " + String(state.thresholdLow) + "%");
  }
  
  if (Firebase.RTDB.getInt(&fbdo, devicePath + "/settings/thresholds/high")) {
    state.thresholdHigh = fbdo.intData();
    Serial.println("   High Threshold: " + String(state.thresholdHigh) + "%");
  }
  
  // Read auto mode
  if (Firebase.RTDB.getBool(&fbdo, devicePath + "/settings/autoMode")) {
    state.autoMode = fbdo.boolData();
    Serial.println("   Auto Mode: " + String(state.autoMode ? "ON" : "OFF"));
  }
  
  Serial.println("✅ Initial state loaded");
}

// ===== FIREBASE STREAM SETUP =====
void setupFirebaseStream() {
  if (!Firebase.ready() || !signupOK) return;
  
  String streamPath = "/devices/" + String(DEVICE_ID);
  
  if (Firebase.RTDB.beginStream(&streamData, streamPath.c_str())) {
    Serial.println("✅ Firebase Stream Started");
    Firebase.RTDB.setStreamCallback(&streamData, streamCallback, streamTimeoutCallback);
  } else {
    Serial.printf("❌ Stream Failed: %s\n", streamData.errorReason().c_str());
  }
}

// ===== STREAM CALLBACK =====
void streamCallback(FirebaseStream data) {
  String path = data.dataPath();
  
  // Handle pump control from dashboard
  if (path.indexOf("/actuators/pump/status") >= 0) {
    String newStatus = data.stringData();
    if (newStatus == "ON" || newStatus == "OFF") {
      setPumpState(newStatus, "MANUAL");
      Serial.println("📡 Pump status updated from dashboard: " + newStatus);
      blinkStatusLED(2);
    }
  }
  
  // Handle mode change
  if (path.indexOf("/actuators/pump/mode") >= 0) {
    String newMode = data.stringData();
    state.pumpMode = newMode;
    Serial.println("📡 Pump mode updated: " + newMode);
    
    // If switching to MANUAL mode, don't let auto-control interfere
    if (newMode == "MANUAL") {
      state.autoMode = false;
    } else if (newMode == "AUTO") {
      state.autoMode = true;
    }
  }
  
  // Handle settings changes
  if (path.indexOf("/settings/thresholds/low") >= 0) {
    state.thresholdLow = data.intData();
    Serial.println("📡 Low threshold updated: " + String(state.thresholdLow));
  }
  
  if (path.indexOf("/settings/thresholds/high") >= 0) {
    state.thresholdHigh = data.intData();
    Serial.println("📡 High threshold updated: " + String(state.thresholdHigh));
  }
  
  if (path.indexOf("/settings/autoMode") >= 0) {
    state.autoMode = data.boolData();
    Serial.println("📡 Auto mode updated: " + String(state.autoMode ? "ON" : "OFF"));
  }
}

void streamTimeoutCallback(bool timeout) {
  if (timeout) {
    Serial.println("⚠ Stream timeout, reconnecting...");
    setupFirebaseStream();
  }
}

// ===== TIMESTAMP HELPER =====
String getTimestamp() {
  timeClient.update();
  unsigned long epochTime = timeClient.getEpochTime();
  
  // Convert to ISO 8601 format
  time_t rawtime = epochTime;
  struct tm *timeinfo = gmtime(&rawtime);
  char buffer[25];
  strftime(buffer, 25, "%Y-%m-%dT%H:%M:%SZ", timeinfo);
  return String(buffer);
}

// ===== SENSOR READING (ESP32 ADC) =====
void readSoilMoisture() {
  // ESP32 ADC is 12-bit (0-4095) vs ESP8266's 10-bit (0-1023)
  int rawValue = analogRead(SOIL_SENSOR_PIN);
  
  // Map 12-bit value to percentage (inverted: higher reading = drier soil)
  state.moisture = map(rawValue, 4095, 0, 0, 100);
  state.moisture = constrain(state.moisture, 0, 100);
  
  Serial.print("💧 Soil Moisture: ");
  Serial.print(state.moisture);
  Serial.print("% (Raw ADC: ");
  Serial.print(rawValue);
  Serial.println(")");
  
  // Visual indicator on separate LED pin
  if (STATUS_LED_PIN != PUMP_RELAY_PIN) {
    digitalWrite(STATUS_LED_PIN, (state.moisture < state.thresholdLow) ? HIGH : LOW);
  }
}

// ===== CENTRALIZED PUMP STATE CONTROL =====
void setPumpState(String newStatus, String trigger) {
  if (state.pumpStatus == newStatus) {
    return; // No change needed
  }
  
  // Apply debounce - prevent rapid toggling
  unsigned long currentMillis = millis();
  if (currentMillis - state.lastPumpChange < PUMP_DEBOUNCE_TIME) {
    Serial.println("⏸ Pump change debounced (too soon)");
    return;
  }
  
  // Update state
  String previousStatus = state.pumpStatus;
  state.pumpStatus = newStatus;
  state.lastPumpChange = currentMillis;
  
  // Control physical relay
  digitalWrite(PUMP_RELAY_PIN, (newStatus == "ON") ? HIGH : LOW);
  
  // Log the change
  Serial.println("🔄 Pump " + previousStatus + " → " + newStatus + " (" + trigger + ")");
  
  // Update Firebase
  String path = "/devices/" + String(DEVICE_ID) + "/actuators/pump";
  Firebase.RTDB.setString(&fbdo, path + "/status", state.pumpStatus);
  Firebase.RTDB.setString(&fbdo, path + "/lastChanged", getTimestamp());
  
  // Log to history
  logToHistory("pump", newStatus);
  
  // Visual feedback
  blinkStatusLED(newStatus == "ON" ? 1 : 2);
}

// ===== PUMP CONTROL - FIXED LOGIC =====
void controlPump() {
  // ONLY run auto-control if:
  // 1. Auto mode is enabled
  // 2. Pump mode is set to AUTO
  if (!state.autoMode || state.pumpMode != "AUTO") {
    return; // Skip auto-control in manual mode
  }
  
  // Auto control logic with proper conditions
  if (state.moisture < state.thresholdLow && state.pumpStatus == "OFF") {
    // Soil is dry and pump is off -> Turn ON
    setPumpState("ON", "AUTO-LOW");
    Serial.println("🔵 AUTO: Pump turned ON (moisture: " + String(state.moisture) + "% < " + String(state.thresholdLow) + "%)");
  } 
  else if (state.moisture > state.thresholdHigh && state.pumpStatus == "ON") {
    // Soil is wet enough and pump is on -> Turn OFF
    setPumpState("OFF", "AUTO-HIGH");
    Serial.println("🟢 AUTO: Pump turned OFF (moisture: " + String(state.moisture) + "% > " + String(state.thresholdHigh) + "%)");
  }
  // If moisture is between thresholds, maintain current state (hysteresis)
}

// ===== FIREBASE SYNC =====
void syncToFirebase() {
  if (!Firebase.ready() || !signupOK) return;
  
  String basePath = "/devices/" + String(DEVICE_ID);
  String timestamp = getTimestamp();
  
  // Update sensor data
  Firebase.RTDB.setInt(&fbdo, basePath + "/sensors/moisture", state.moisture);
  Firebase.RTDB.setString(&fbdo, basePath + "/sensors/timestamp", timestamp);
  
  // Log to history every sync
  logToHistory("moisture", String(state.moisture));
}

// ===== HEARTBEAT =====
void sendHeartbeat() {
  if (!Firebase.ready() || !signupOK) return;
  
  String path = "/devices/" + String(DEVICE_ID) + "/info/lastSeen";
  Firebase.RTDB.setString(&fbdo, path, getTimestamp());
  Serial.println("💓 Heartbeat sent");
}

// ===== HISTORY LOGGING =====
void logToHistory(String type, String value) {
  if (!Firebase.ready() || !signupOK) return;
  
  String timestamp = getTimestamp();
  String historyPath = "/history/" + String(DEVICE_ID) + "/" + type;
  
  FirebaseJson json;
  json.set("value", value);
  json.set("timestamp", timestamp);
  
  if (type == "pump") {
    json.set("trigger", state.autoMode ? "AUTO" : "MANUAL");
  }
  
  Firebase.RTDB.pushJSON(&fbdo, historyPath.c_str(), &json);
}

// ===== STATUS LED BLINK =====
void blinkStatusLED(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(100);
    digitalWrite(STATUS_LED_PIN, LOW);
    delay(100);
  }
}