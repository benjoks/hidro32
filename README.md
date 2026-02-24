# Hidro32 - Smart Hydroponics Monitoring System

An IoT solution developed for **ESP32** using **PlatformIO** and **Arduino framework**. This system monitors water quality parameters (pH and TDS/PPM) in real-time and synchronizes data with a cloud broker via **MQTT**.

## 🚀 Key Features

* **Real-time Monitoring**: Precise acquisition of pH and TDS (Total Dissolved Solids) data using analog sensors.
* **Signal Processing**: Implementation of median filtering algorithms to stabilize noisy analog sensor readings.
* **Hybrid Communication**: Uses **HTTP POST** requests to fetch dynamic MQTT credentials via webhooks.
* **MQTT Integration**: Reliable data transmission to a broker with automatic reconnection logic for both WiFi and MQTT sessions.
* **JSON Serialization**: Efficient data packaging using **ArduinoJson** for structured telemetry.

## 🛠️ Tech Stack

* **Hardware**: ESP32 (NodeMCU-32S).
* **Language/Framework**: C++ / Arduino.
* **IDE/Tooling**: PlatformIO.
* **Protocols**: MQTT, HTTP, TCP/IP.
* **Libraries**: PubSubClient, ArduinoJson, IoTicosSplitter.

## 📂 Project Structure

* `src/main.cpp`: Core logic including sensor processing and network management.
* `include/Colors.h`: Custom header for ANSI color-coded serial debugging.
* `lib/IoTicosSplitter`: Private library for string parsing and data manipulation.
* `platformio.ini`: Project configuration, dependency management, and build-time environment variables.

## ⚙️ Setup and Deployment

### Configuration
Credentials are managed through `build_flags` in `platformio.ini` to avoid exposing sensitive data in source code:

1. Open `platformio.ini`.
2. Update the `build_flags` section with your network details:
   * `WIFI_SSID`, `WIFI_PASS`, `MQTT_SERVER`, `WEBHOOK_ENDPOINT`.

### Compilation
Using PlatformIO CLI or IDE:
    
    # Build project
    pio run

    # Upload to ESP32 and open monitor
    pio run --target upload --target monitor

Developed by benjoks.