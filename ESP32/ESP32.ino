/*
 * Arduino UNO - Smart Irrigation Pump Controller
 * Receives commands from ESP32 via Serial and controls relay/pump
 */

// ===== Hardware Pins =====
const int PUMP_RELAY_PIN = 7;      // Relay connected to Pin 7
const int STATUS_LED_PIN = 13;     // Built-in LED for status indication

// ===== System State =====
String currentPumpState = "OFF";

void setup() {
  // Initialize Serial communication with ESP32
  Serial.begin(9600);
  
  // Initialize pins
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);
  
  // Initialize relay OFF (active-low relay)
  digitalWrite(PUMP_RELAY_PIN, HIGH);  // HIGH = OFF
  digitalWrite(STATUS_LED_PIN, LOW);
  
  delay(1000);
  
  // Startup sequence
  Serial.println("\n========================================");
  Serial.println("  SMART IRRIGATION - ARDUINO");
  Serial.println("========================================");
  Serial.println("Status: READY");
  Serial.println("Pump: OFF");
  Serial.println("Waiting for ESP32 commands...");
  Serial.println("========================================\n");
  
  // Startup blink
  blinkStatusLED(3);
}

void loop() {
  // Check for incoming commands from ESP32
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command.length() > 0) {
      processCommand(command);
    }
  }
  
  delay(10);
}

void processCommand(String command) {
  Serial.print("[RECEIVED] ");
  Serial.println(command);
  
  // Parse command format: "PUMP:ON" or "PUMP:OFF"
  int colonIndex = command.indexOf(':');
  
  if (colonIndex > 0) {
    String device = command.substring(0, colonIndex);
    String action = command.substring(colonIndex + 1);
    
    device.trim();
    action.trim();
    
    if (device == "PUMP") {
      if (action == "ON") {
        turnPumpOn();
      } 
      else if (action == "OFF") {
        turnPumpOff();
      }
      else {
        Serial.println("[ERROR] Unknown action: " + action);
      }
    }
    else {
      Serial.println("[ERROR] Unknown device: " + device);
    }
  }
  else {
    Serial.println("[ERROR] Invalid command format");
  }
  
  Serial.println();
}

void turnPumpOn() {
  if (currentPumpState == "OFF") {
    digitalWrite(PUMP_RELAY_PIN, LOW);    // LOW = ON (active-low relay)
    digitalWrite(STATUS_LED_PIN, HIGH);   // LED ON
    currentPumpState = "ON";
    
    Serial.println("[ACTION] Pump turned ON");
    Serial.println("[STATUS] Pin 7 = LOW (Relay activated)");
    Serial.println("[STATUS] LED ON");
    
    blinkStatusLED(1);  // Quick confirmation blink
  } else {
    Serial.println("[INFO] Pump already ON");
  }
}

void turnPumpOff() {
  if (currentPumpState == "ON") {
    digitalWrite(PUMP_RELAY_PIN, HIGH);   // HIGH = OFF (active-low relay)
    digitalWrite(STATUS_LED_PIN, LOW);    // LED OFF
    currentPumpState = "OFF";
    
    Serial.println("[ACTION] Pump turned OFF");
    Serial.println("[STATUS] Pin 7 = HIGH (Relay deactivated)");
    Serial.println("[STATUS] LED OFF");
    
    blinkStatusLED(2);  // Double blink confirmation
  } else {
    Serial.println("[INFO] Pump already OFF");
  }
}

void blinkStatusLED(int times) {
  bool originalState = digitalRead(STATUS_LED_PIN);
  
  for (int i = 0; i < times; i++) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(100);
    digitalWrite(STATUS_LED_PIN, LOW);
    delay(100);
  }
  
  // Restore original state
  digitalWrite(STATUS_LED_PIN, originalState);
}

/*
 * ===== WIRING DIAGRAM =====
 * 
 * ESP32 to Arduino:
 * - ESP32 TX (GPIO1)  →  Arduino RX (Pin 0)
 * - ESP32 RX (GPIO3)  →  Arduino TX (Pin 1)
 * - ESP32 GND         →  Arduino GND
 * 
 * Relay Module to Arduino:
 * - VCC    →  Arduino 5V
 * - GND    →  Arduino GND
 * - IN     →  Arduino Pin 7
 * 
 * Pump/Motor to Relay:
 * - Power Source (+) → Relay COM (Common)
 * - Relay NO (Normally Open) → Pump/Motor (+)
 * - Pump/Motor (-) → Power Source (-)
 * 
 * ===== RELAY NOTES =====
 * Most relay modules are ACTIVE-LOW:
 * - Pin 7 = LOW  → Relay ON  → Pump ON
 * - Pin 7 = HIGH → Relay OFF → Pump OFF
 * 
 * If your relay is ACTIVE-HIGH, swap HIGH/LOW in the code.
 * 
 * ===== SERIAL COMMANDS =====
 * Expected format from ESP32:
 * - "PUMP:ON"   → Turn pump ON
 * - "PUMP:OFF"  → Turn pump OFF
 * 
 * ===== TESTING MANUALLY =====
 * 1. Upload this code to Arduino
 * 2. Open Serial Monitor (9600 baud)
 * 3. Set line ending to "Newline" or "Both NL & CR"
 * 4. Type: PUMP:ON  → Should turn pump ON
 * 5. Type: PUMP:OFF → Should turn pump OFF
 * 
 * ===== LED INDICATORS =====
 * - 3 blinks at startup = System ready
 * - 1 blink = Pump turned ON
 * - 2 blinks = Pump turned OFF
 * - Solid ON = Pump is currently running
 * - Solid OFF = Pump is currently stopped
 * 
 * ===== EXPECTED OUTPUT =====
 * 
 * ========================================
 *   SMART IRRIGATION - ARDUINO
 * ========================================
 * Status: READY
 * Pump: OFF
 * Waiting for ESP32 commands...
 * ========================================
 * 
 * [RECEIVED] PUMP:ON
 * [ACTION] Pump turned ON
 * [STATUS] Pin 7 = LOW (Relay activated)
 * [STATUS] LED ON
 * 
 * [RECEIVED] PUMP:OFF
 * [ACTION] Pump turned OFF
 * [STATUS] Pin 7 = HIGH (Relay deactivated)
 * [STATUS] LED OFF
 * 
 * ===== TROUBLESHOOTING =====
 * 
 * Problem: No serial output
 * Solution: Check baud rate is 9600, correct COM port selected
 * 
 * Problem: Relay not clicking
 * Solution: Check Pin 7 connection, verify relay power (5V), test relay separately
 * 
 * Problem: Pump runs but won't stop
 * Solution: Check relay wiring, may need to swap NO/NC terminals
 * 
 * Problem: Commands not received from ESP32
 * Solution: Verify RX/TX crossover wiring, check common GND, ensure ESP32 Serial.println()
 * 
 * Problem: Relay clicks but pump doesn't run
 * Solution: Check pump power supply, verify relay load capacity, test pump separately
 */