# AgriSmart: ESP32 Smart Irrigation

End-to-end smart irrigation system: an ESP32 reads soil moisture, an Arduino UNO drives the pump relay, Firebase Realtime Database carries state both ways, and a Streamlit dashboard shows live readings, history and lets you control the pump or set auto-mode thresholds from anywhere.

## How it works

```
soil sensor ─> ESP32 ──(Serial)──> Arduino UNO ──> relay ──> pump
                 │
                 └──(Wi-Fi)──> Firebase RTDB <──> Streamlit dashboard
```

- **ESP32** samples the capacitive soil sensor every 5 s, pushes moisture and pump status to `/devices/device_001`, listens for commands, and runs **auto mode** (pump ON below `thresholdLow`, OFF above `thresholdHigh`) with debounce so the relay never chatters. Heartbeats every 30 s let the dashboard show online/offline.
- **Arduino UNO** receives `PUMP_ON` / `PUMP_OFF` over Serial and switches an active-low relay, with a status LED.
- **Dashboard** (`Iot.py`) has Firebase email login, live gauge and condition label, manual pump toggle, auto-mode threshold sliders, 24 h Plotly history, pump-runtime stats, a simple rule-based watering recommendation and browser notifications when moisture crosses the thresholds.

## Firmware

| Folder | Board | Notes |
|---|---|---|
| `firmware/esp32_stream/` | ESP32 | Firebase stream (push) version with NTP timestamps |
| `firmware/esp32_polling/` | ESP32 | Stable polling version used in the final demo; talks to the UNO over `Serial2` |
| `firmware/arduino_uno_pump/` | Arduino UNO | Pump relay controller |
| `firmware/esp8266_blynk_smart_home/` | ESP8266 | Earlier Blynk-based 4-relay + DHT11 experiment |

Libraries (Arduino IDE Library Manager): `Firebase Arduino Client Library for ESP8266 and ESP32`, `NTPClient`, `ArduinoJson`, `DHT sensor library`, `Blynk`.

Before flashing, edit the `#define WIFI_SSID`, `WIFI_PASSWORD`, `API_KEY` and `DATABASE_URL` lines at the top of the sketch. Do not commit real values.

## Dashboard

```bash
python -m venv .venv && .venv\Scripts\activate
pip install -r requirements.txt
cp firebase_config.example.json firebase_config.json   # fill in apiKey, authDomain, databaseURL, storageBucket
streamlit run Iot.py
```

Enable **Email/Password** sign-in in Firebase Authentication and create the Realtime Database with rules that require auth.

## Repo contents

```
Iot.py                          Streamlit dashboard
firmware/                       ESP32, Arduino UNO and ESP8266 sketches
docs/AgriSmart-presentation.pptx project presentation
docs/sample_irrigation_log.csv  a logged session of moisture and pump events
```

Built as a college IoT project (Team 8).
