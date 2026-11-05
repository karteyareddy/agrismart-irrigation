#define BLYNK_TEMPLATE_ID "YOUR_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "Smart Home"
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_AUTH_TOKEN"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "YOUR_WIFI_SSID";     // 🔹 Your WiFi Name
char pass[] = "YOUR_WIFI_PASSWORD";         // 🔹 Your WiFi Password

#define DHTPIN D4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Relay Pins
#define RelayPin1 D1
#define RelayPin2 D2
#define RelayPin3 D5
#define RelayPin4 D6

// Switch Pins
#define SwitchPin1 D7
#define SwitchPin2 D3
#define SwitchPin3 D8
#define SwitchPin4 RX

#define wifiLed D0  // WiFi status LED

// Virtual Pins (Blynk)
#define VPIN_BUTTON_1 V1
#define VPIN_BUTTON_2 V2
#define VPIN_BUTTON_3 V3
#define VPIN_BUTTON_4 V4
#define VPIN_BUTTON_C V5
#define VPIN_TEMPERATURE V6
#define VPIN_HUMIDITY V7

// States
bool toggleState_1 = LOW;
bool toggleState_2 = LOW;
bool toggleState_3 = LOW;
bool toggleState_4 = LOW;

bool SwitchState_1 = LOW;
bool SwitchState_2 = LOW;
bool SwitchState_3 = LOW;
bool SwitchState_4 = LOW;

float temperature1 = 0;
float humidity1 = 0;
int wifiFlag = 0;
bool fetch_blynk_state = true;

BlynkTimer timer;

// -------------------- FUNCTION DEFINITIONS --------------------

void all_SwitchOff() {
  toggleState_1 = 0; digitalWrite(RelayPin1, HIGH); Blynk.virtualWrite(VPIN_BUTTON_1, toggleState_1);
  toggleState_2 = 0; digitalWrite(RelayPin2, HIGH); Blynk.virtualWrite(VPIN_BUTTON_2, toggleState_2);
  toggleState_3 = 0; digitalWrite(RelayPin3, HIGH); Blynk.virtualWrite(VPIN_BUTTON_3, toggleState_3);
  toggleState_4 = 0; digitalWrite(RelayPin4, HIGH); Blynk.virtualWrite(VPIN_BUTTON_4, toggleState_4);
}

void checkBlynkStatus() {
  bool isconnected = Blynk.connected();
  if (!isconnected) {
    wifiFlag = 1;
    digitalWrite(wifiLed, HIGH);
    Serial.println("🔴 Blynk Disconnected");
  } else {
    wifiFlag = 0;
    digitalWrite(wifiLed, LOW);
  }
}

void readSensor() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) {
    Serial.println("⚠ Failed to read from DHT sensor!");
    return;
  }
  humidity1 = h;
  temperature1 = t;
}

void sendSensor() {
  readSensor();
  Blynk.virtualWrite(VPIN_HUMIDITY, humidity1);
  Blynk.virtualWrite(VPIN_TEMPERATURE, temperature1);
}

// -------------------- BLYNK WRITE FUNCTIONS --------------------
BLYNK_WRITE(VPIN_BUTTON_1) {
  toggleState_1 = param.asInt();
  digitalWrite(RelayPin1, !toggleState_1);
}

BLYNK_WRITE(VPIN_BUTTON_2) {
  toggleState_2 = param.asInt();
  digitalWrite(RelayPin2, !toggleState_2);
}

BLYNK_WRITE(VPIN_BUTTON_3) {
  toggleState_3 = param.asInt();
  digitalWrite(RelayPin3, !toggleState_3);
}

BLYNK_WRITE(VPIN_BUTTON_4) {
  toggleState_4 = param.asInt();
  digitalWrite(RelayPin4, !toggleState_4);
}

BLYNK_WRITE(VPIN_BUTTON_C) {
  all_SwitchOff();
}

BLYNK_CONNECTED() {
  if (fetch_blynk_state) {
    Blynk.syncVirtual(VPIN_BUTTON_1);
    Blynk.syncVirtual(VPIN_BUTTON_2);
    Blynk.syncVirtual(VPIN_BUTTON_3);
    Blynk.syncVirtual(VPIN_BUTTON_4);
  }
  Blynk.syncVirtual(VPIN_TEMPERATURE);
  Blynk.syncVirtual(VPIN_HUMIDITY);
  Serial.println("🟢 Blynk Connected Successfully!");
}

// -------------------- MANUAL SWITCH CONTROL --------------------
void manual_control() {
  if (digitalRead(SwitchPin1) == LOW && SwitchState_1 == LOW) {
    digitalWrite(RelayPin1, LOW);
    toggleState_1 = HIGH;
    SwitchState_1 = HIGH;
    Blynk.virtualWrite(VPIN_BUTTON_1, toggleState_1);
  }
  if (digitalRead(SwitchPin1) == HIGH && SwitchState_1 == HIGH) {
    digitalWrite(RelayPin1, HIGH);
    toggleState_1 = LOW;
    SwitchState_1 = LOW;
    Blynk.virtualWrite(VPIN_BUTTON_1, toggleState_1);
  }

  if (digitalRead(SwitchPin2) == LOW && SwitchState_2 == LOW) {
    digitalWrite(RelayPin2, LOW);
    toggleState_2 = HIGH;
    SwitchState_2 = HIGH;
    Blynk.virtualWrite(VPIN_BUTTON_2, toggleState_2);
  }
  if (digitalRead(SwitchPin2) == HIGH && SwitchState_2 == HIGH) {
    digitalWrite(RelayPin2, HIGH);
    toggleState_2 = LOW;
    SwitchState_2 = LOW;
    Blynk.virtualWrite(VPIN_BUTTON_2, toggleState_2);
  }

  if (digitalRead(SwitchPin3) == LOW && SwitchState_3 == LOW) {
    digitalWrite(RelayPin3, LOW);
    toggleState_3 = HIGH;
    SwitchState_3 = HIGH;
    Blynk.virtualWrite(VPIN_BUTTON_3, toggleState_3);
  }
  if (digitalRead(SwitchPin3) == HIGH && SwitchState_3 == HIGH) {
    digitalWrite(RelayPin3, HIGH);
    toggleState_3 = LOW;
    SwitchState_3 = LOW;
    Blynk.virtualWrite(VPIN_BUTTON_3, toggleState_3);
  }

  if (digitalRead(SwitchPin4) == LOW && SwitchState_4 == LOW) {
    digitalWrite(RelayPin4, LOW);
    toggleState_4 = HIGH;
    SwitchState_4 = HIGH;
    Blynk.virtualWrite(VPIN_BUTTON_4, toggleState_4);
  }
  if (digitalRead(SwitchPin4) == HIGH && SwitchState_4 == HIGH) {
    digitalWrite(RelayPin4, HIGH);
    toggleState_4 = LOW;
    SwitchState_4 = LOW;
    Blynk.virtualWrite(VPIN_BUTTON_4, toggleState_4);
  }
}

// -------------------- SETUP --------------------
void setup() {
  Serial.begin(9600);

  pinMode(RelayPin1, OUTPUT);
  pinMode(RelayPin2, OUTPUT);
  pinMode(RelayPin3, OUTPUT);
  pinMode(RelayPin4, OUTPUT);
  pinMode(wifiLed, OUTPUT);

  pinMode(SwitchPin1, INPUT_PULLUP);
  pinMode(SwitchPin2, INPUT_PULLUP);
  pinMode(SwitchPin3, INPUT_PULLUP);
  pinMode(SwitchPin4, INPUT_PULLUP);

  digitalWrite(RelayPin1, HIGH);
  digitalWrite(RelayPin2, HIGH);
  digitalWrite(RelayPin3, HIGH);
  digitalWrite(RelayPin4, HIGH);

  dht.begin();
  digitalWrite(wifiLed, HIGH);

  WiFi.begin(ssid, pass);
  Serial.print("📶 Connecting to WiFi");

  int wifiAttempt = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempt < 20) {
    delay(500);
    Serial.print(".");
    wifiAttempt++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    digitalWrite(wifiLed, LOW);
    Blynk.config(auth);
    Blynk.connect();
  } else {
    Serial.println("\n❌ WiFi Connection Failed! Running in Offline Mode");
  }

  timer.setInterval(2000L, checkBlynkStatus);
  timer.setInterval(3000L, sendSensor);
}

// -------------------- LOOP --------------------
void loop() {
  manual_control();

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, pass);
  }

  if (Blynk.connected()) {
    Blynk.run();
  }

  timer.run();
}
