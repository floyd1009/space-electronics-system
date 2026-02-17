CADSE v5 Space Electronics Project

Project Overview
ESP32-S3 based satellite simulation platform for TU Berlin's Space Electronics course. The system implements multiple operational modes with sensor monitoring, telemetry transmission, and remote command processing.

Hardware Specifications
- MCU: ESP32-S3 (Dual-core, WiFi/BLE capable)
- Display: SSD1306 OLED (128x64 pixels)
- Sensors: BME280 (temperature, humidity, pressure)
- Input: Capacitive touch sensors (5 buttons)
- Output: LED, Buzzer
- Power: Battery and USB monitoring

Pin Configurations
- BATTERY_PIN: 7 (ADC)
- USB_VOLTAGE_PIN: 3 (ADC)
- BUZZER_PIN: 14
- LED_PIN: 15
- Touch Sensors:
  - UP: GPIO 1
  - LEFT/PREV: GPIO 2
  - X: GPIO 4
  - DOWN: GPIO 5
  - RIGHT/NEXT: GPIO 6
- I2C: Default ESP32-S3 pins for OLED and BME280

Key Features
1. Six operational modes (0-5)
2. WiFi connectivity for telemetry and commands
3. MQTT communication for remote control
4. Onboard environmental monitoring
5. OTA (Over-The-Air) update capability
6. Touch-based user interface
7. Visual (OLED/LED) and audio (buzzer) feedback

Operational Modes
- Mode 0: Basic Monitoring - Display system status and sensor readings
- Mode 1: Micro-G Detection - Simulate microgravity detection
- Mode 2: Pressure Monitor - Monitor pressure changes with cat safety alerts
- Mode 3: Attitude Indicator - Artificial horizon visualization
- Mode 4: Rolling Plotter - Real-time graphical data visualization
- Mode 5: Orbit Simulator - Visual Earth orbit simulation

Documentation
This project includes comprehensive Doxygen documentation in the source code. The code has been annotated with special Doxygen comment blocks that can be processed to generate HTML or PDF documentation. This documentation provides detailed information about:

- Function purposes and parameters
- Module organization
- Hardware configurations
- Implementation details for each operational mode

To generate the documentation, install Doxygen and run it against the source code.

MQTT Configuration
- Server: heide.bastla.net
- Port: 8883 (TLS)
- Topics:
  - Telemetry: cadse/2024/{boardId}/tm
  - Command: cadse/2024/{boardId}/tc
  - Response: cadse/2024/{boardId}/response
- Commands:
  - "MX" - Change to mode X (0-5)
  - "SET_DEFAULT_MX" - Set default mode X
  - "OTA_RESTART" - Restart for OTA updates

Touch Control Operation
ESP32 touch values DECREASE when touched:
- Normal values: 40000-60000
- Touched values: 5000-30000
- Current threshold: 30000

Known Issues & Troubleshooting
- Touch Controls: ESP32 touch sensors read LOWER when touched (inverse logic)
- LED Control: May use MCP23017 I2C expander instead of direct GPIO
- Microgravity Detection: Uses touch differential as simulated accelerometer
- WiFi Connection: Configured for specific network credentials
- Serial Commands: Can change modes via serial monitor (type 0-5)

Development Notes
- Using PlatformIO with Arduino framework
- Libraries: PubSubClient, Adafruit_SSD1306, Adafruit_BME280, ArduinoOTA
- Fixed microgravity detection threshold to prevent false alerts
- Implemented cat safety pressure monitoring for drops >10 hPa