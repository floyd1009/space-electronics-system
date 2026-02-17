

 #include <Arduino.h>
 #include <Wire.h>
 #include <SPI.h>
 #include <WiFi.h>
 #include <WiFiClientSecure.h>
 #include <PubSubClient.h>
 #include <Adafruit_GFX.h>
 #include <Adafruit_SSD1306.h>
 #include <Adafruit_Sensor.h>
 #include <Adafruit_BME280.h>
 #include <ArduinoOTA.h>
 #include <Preferences.h>  // Added for persistent storage
  

 #define SCREEN_WIDTH 128      
 #define SCREEN_HEIGHT 64      
 #define OLED_RESET    -1      
 #define SCREEN_ADDRESS 0x3C   
 // End of display_config group
  

 #define BATTERY_PIN     7      
 #define USB_VOLTAGE_PIN 3      
 #define BUZZER_PIN      14     
 #define LED_PIN         15     
 
 #define TOUCH_UP        1      
 #define TOUCH_LEFT      2      
 #define TOUCH_X         4      
 #define TOUCH_DOWN      5      
 #define TOUCH_RIGHT     6      
 
 // For backward compatibility with existing code
 #define TOUCH_PIN_PREV  TOUCH_LEFT   
 #define TOUCH_PIN_NEXT  TOUCH_RIGHT  
 // End of pin_definitions group
  

 #define BATTERY_VOLTAGE_MULTIPLIER  1.7  
 #define USB_VOLTAGE_MULTIPLIER      1.9  
 #define LOW_BATTERY_THRESHOLD       3.6  
 // End of voltage_params group
  

 const char* WIFI_SSID = "We have internet!";        
 const char* WIFI_PASSWORD = "Ilovehanna2";          
 const char* MQTT_SERVER = "heide.bastla.net";       
 const int   MQTT_PORT = 8883;                       
 const char* MQTT_USER = "mse24";                    
 const char* MQTT_PASSWORD = "aura";                 
 const char* MQTT_CLIENT_ID = "floyd_esp32s3_satellite"; 
 const String mqttPrefix = "cadse";                  
 const int mqttYear = 2024;                          
 const byte mqttBoardId = 0;                         
 // End of network_config group
 

 const int touchThreshold = 60000;  
 const int channelBuzzer = 0;       
 // End of touch_config group
 
// Topic strings to be generated in setup()
String mqttTelemetryTopic;   
String mqttCommandTopic;     
String mqttResponseTopic;    
  

 Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); 
 Adafruit_BME280 bme;        
 WiFiClientSecure wifiClient; 
 PubSubClient mqttClient(wifiClient); 
 Preferences preferences;     
 // End of global_objects group
  

 int currentMode = 0;               
 volatile int nextMode = 0;         
 int defaultMode = 0;               
 unsigned long lastTelemetryTime = 0; 
 unsigned long lastModeUpdateTime = 0; 
 float batteryVoltage = 0.0;        
 float usbVoltage = 0.0;            
 bool lowBatteryAlert = false;      
 String deviceID = "";              
 bool isOTAUpdating = false;        
 
 // Debug variables
 unsigned long lastTouchDebugTime = 0; 
 const int touchDebounceTime = 300;    
 // End of global_vars group
  
// Function prototypes

void setupWiFi();


void setupMQTT();


void setupOTA();


void setupBME280();


void getChipInfo();


void handleMQTTCallback(char* topic, byte* payload, unsigned int length);


void sendTelemetry();


void switchMode(int newMode);


void displayModeInfo();


void runCurrentMode();


void reconnectMQTT();


void increaseModeNumber();


void decreaseModeNumber();


void initializeTouchbuttons();


void debugTouchSensors();


String createJSONTelemetry();


void displayBootSequence();
  


void runBasicMonitoring();


void runMicroGravityDetection();


void runPressureMonitoring();


void runAttitudeIndicator();


void runRollingPlotter();


void runCreativeMode();
 // End of mode_functions group
 

void increaseModeNumber() {
   int maxTouch = touchRead(TOUCH_RIGHT);
   if (currentMode < 5 && maxTouch > touchThreshold) {
     nextMode = currentMode + 1;
     Serial.println("Increasing mode to: " + String(nextMode));
   }
 }
 

void decreaseModeNumber() {
   int maxTouch = touchRead(TOUCH_LEFT);
   if (currentMode > 0 && maxTouch > touchThreshold) {
     nextMode = currentMode - 1;
     Serial.println("Decreasing mode to: " + String(nextMode));
   }
 }
 

void initializeTouchbuttons() {
   touchAttachInterrupt(TOUCH_RIGHT, increaseModeNumber, touchThreshold);
   touchAttachInterrupt(TOUCH_LEFT, decreaseModeNumber, touchThreshold);
   Serial.println("Touch buttons initialized with interrupts");
 }
 

void debugTouchSensors() {
   if (millis() - lastTouchDebugTime > 500) {
     int right = touchRead(TOUCH_RIGHT);
     int left = touchRead(TOUCH_LEFT);
     int up = touchRead(TOUCH_UP);
     int down = touchRead(TOUCH_DOWN);
     int x = touchRead(TOUCH_X);
     
     Serial.print("Touch values - RIGHT(6): ");
     Serial.print(right);
     Serial.print(", LEFT(2): ");
     Serial.print(left);
     Serial.print(", UP(1): ");
     Serial.print(up);
     Serial.print(", DOWN(5): ");
     Serial.print(down);
     Serial.print(", X(4): ");
     Serial.println(x);
     
     // Force mode change if extreme touch values detected
     if (right > 65000 || right < 10) {
       Serial.println("RIGHT touch detected, forcing mode change");
       if (currentMode < 5) nextMode = currentMode + 1;
     }
     
     if (left > 65000 || left < 10) {
       Serial.println("LEFT touch detected, forcing mode change");
       if (currentMode > 0) nextMode = currentMode - 1;
     }
     
     lastTouchDebugTime = millis();
   }
 }
  

void setup() {
   // Initialize serial
   Serial.begin(115200);
   delay(100);
   
   Serial.println("Starting CADSE v5 Space Electronics Project");
   
   // Initialize GPIO
   pinMode(LED_PIN, OUTPUT);
   pinMode(BUZZER_PIN, OUTPUT);
   
   // Initialize I2C
   Wire.begin();
   
   // Initialize display
   if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
     Serial.println(F("SSD1306 allocation failed"));
     for(;;); // Don't proceed, loop forever
   }
   display.clearDisplay();
   display.setTextSize(1);
   display.setTextColor(SSD1306_WHITE);
   
   // Get chip information
   getChipInfo();
   
   // Generate board-specific MQTT topics using professor's pattern
   String mqttTopicBase = mqttPrefix + "/" + String(mqttYear) + "/" + String(mqttBoardId) + "/";
   mqttTelemetryTopic = mqttTopicBase + "tm";     // Telemetry
   mqttCommandTopic = mqttTopicBase + "tc";       // Telecommand
   mqttResponseTopic = mqttTopicBase + "response";
   
   Serial.println("MQTT Topics:");
   Serial.println("- Telemetry: " + mqttTelemetryTopic);
   Serial.println("- Command: " + mqttCommandTopic);
   Serial.println("- Response: " + mqttResponseTopic);
   
   // Initialize preferences for persistent storage (R5.2)
   preferences.begin("cadse", false);
   defaultMode = preferences.getInt("defMode", 0);
   
   // Display boot sequence
   displayBootSequence();
   
   // Read initial voltage values
   batteryVoltage = analogRead(BATTERY_PIN) * BATTERY_VOLTAGE_MULTIPLIER * 3.3 / 4095.0;
   usbVoltage = analogRead(USB_VOLTAGE_PIN) * USB_VOLTAGE_MULTIPLIER * 3.3 / 4095.0;
   
   // Check battery status
   lowBatteryAlert = (batteryVoltage < LOW_BATTERY_THRESHOLD);
   
   // Initialize BME280 sensor
   setupBME280();
   
   // Connect to WiFi
   setupWiFi();
   
   // Initialize touch buttons with interrupts
   initializeTouchbuttons();
   
   // Setup MQTT with secure connection
   setupMQTT();
   
   // Setup OTA updates
   setupOTA();
   
   // Indicate successful boot with LED and buzzer
   digitalWrite(LED_PIN, HIGH);
   tone(BUZZER_PIN, 1000, 100);
   delay(200);
   tone(BUZZER_PIN, 1500, 100);
   delay(200);
   tone(BUZZER_PIN, 2000, 100);
   digitalWrite(LED_PIN, LOW);
   
   // Initialize nextMode with currentMode or defaultMode
   currentMode = defaultMode;
   nextMode = currentMode;
   
   Serial.println("Setup complete!");
 }
  

void loop() {
   // Handle OTA updates
   ArduinoOTA.handle();
   
   // Skip regular processing during OTA
   if (isOTAUpdating) {
     return;
   }
   
   // Handle WiFi and MQTT reconnection
   if (WiFi.status() != WL_CONNECTED) {
     setupWiFi();
   }
   
   if (!mqttClient.connected()) {
     reconnectMQTT();
   }
   
   // Process MQTT messages
   mqttClient.loop();
   
   // Debug touch sensors
   debugTouchSensors();
   
   // Check for serial commands to change modes
   if (Serial.available() > 0) {
     char cmd = Serial.read();
     if (cmd >= '0' && cmd <= '5') {
       int newMode = cmd - '0';
       Serial.print("Changing to mode ");
       Serial.println(newMode);
       nextMode = newMode;
     }
   }
   
   // Check for mode changes from interrupts or serial
   if (currentMode != nextMode) {
     switchMode(nextMode);
   }
   
   // Send telemetry data periodically
   if (millis() - lastTelemetryTime > 1000) {
     sendTelemetry();
     lastTelemetryTime = millis();
   }
   
   // Update battery status periodically
   if (millis() - lastModeUpdateTime > 5000) {
     batteryVoltage = analogRead(BATTERY_PIN) * BATTERY_VOLTAGE_MULTIPLIER * 3.3 / 4095.0;
     usbVoltage = analogRead(USB_VOLTAGE_PIN) * USB_VOLTAGE_MULTIPLIER * 3.3 / 4095.0;
     lowBatteryAlert = (batteryVoltage < LOW_BATTERY_THRESHOLD);
     lastModeUpdateTime = millis();
   }
   
   // Run the current operational mode
   runCurrentMode();
 }
  

void setupWiFi() {
   display.clearDisplay();
   display.setCursor(0, 0);
   display.println("Connecting to WiFi");
   display.println(WIFI_SSID);
   display.display();
   
   Serial.print("\nAttempting Wi-Fi connection to ");
   Serial.println(WIFI_SSID);
   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
   
   int wifiAttempts = 0;
   while (WiFi.status() != WL_CONNECTED && wifiAttempts < 20) {
     delay(500);
     Serial.print(".");
     display.print(".");
     display.display();
     wifiAttempts++;
   }
   
   if (WiFi.status() == WL_CONNECTED) {
     Serial.println("\nConnected to WiFi");
     Serial.print("IP address: ");
     Serial.println(WiFi.localIP());
     
     display.println("\nConnected!");
     display.print("IP: ");
     display.println(WiFi.localIP());
     display.display();
     delay(1000);
   } else {
     Serial.println("\nFailed to connect to WiFi");
     display.println("\nWiFi Connection Failed");
     display.println("Continuing without WiFi");
     display.display();
     delay(2000);
   }
 }
  

void switchMode(int newMode) {
   if (newMode < 0 || newMode > 5) {
     return; // Invalid mode
   }
   
   currentMode = newMode;
   Serial.print("Switching to mode ");
   Serial.println(currentMode);
   
   // Provide audio feedback
   for (int i = 0; i < currentMode + 1; i++) {
     tone(BUZZER_PIN, 2000, 100);
     delay(200);
   }
   
   // Update display
   displayModeInfo();
 }
  

void setupMQTT() {
   // Set the client to insecure mode - bypass certificate verification
   wifiClient.setInsecure();
   
   // Configure broker connection
   mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
   mqttClient.setCallback(handleMQTTCallback);
   
   reconnectMQTT();
 }
  

void setupBME280() {
   display.clearDisplay();
   display.setCursor(0, 0);
   display.println("Initializing BME280...");
   display.display();
   
   unsigned status = bme.begin(0x76);
   if (!status) {
     Serial.println("Could not find a valid BME280 sensor!");
     display.println("BME280 not found!");
     display.println("Check wiring/address");
     display.display();
     delay(2000);
   } else {
     Serial.println("BME280 initialized!");
     display.println("BME280 initialized!");
     display.display();
     delay(1000);
   }
 }
  

void setupOTA() {
   ArduinoOTA.setHostname("floyd-satellite");
   ArduinoOTA.setPassword("admin");
   
   ArduinoOTA.onStart([]() {
     isOTAUpdating = true;
     String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
     Serial.println("Start updating " + type);
     
     display.clearDisplay();
     display.setCursor(0, 0);
     display.println("Starting OTA Update");
     display.println("Please wait...");
     display.display();
   });
   
   ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
     Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
     
     display.clearDisplay();
     display.setCursor(0, 0);
     display.println("OTA Update");
     display.print("Progress: ");
     display.print(progress / (total / 100));
     display.println("%");
     
     // Draw progress bar
     display.drawRect(0, 30, 128, 10, SSD1306_WHITE);
     display.fillRect(0, 30, (progress / (total / 128)), 10, SSD1306_WHITE);
     display.display();
   });
   
   ArduinoOTA.onEnd([]() {
     Serial.println("\nOTA Update finished");
     isOTAUpdating = false;
     
     display.clearDisplay();
     display.setCursor(0, 0);
     display.println("OTA Update Complete!");
     display.println("Rebooting...");
     display.display();
   });
   
   ArduinoOTA.onError([](ota_error_t error) {
     Serial.printf("Error[%u]: ", error);
     isOTAUpdating = false;
     
     display.clearDisplay();
     display.setCursor(0, 0);
     display.println("OTA Error!");
     
     if (error == OTA_AUTH_ERROR) {
       Serial.println("Auth Failed");
       display.println("Auth Failed");
     } else if (error == OTA_BEGIN_ERROR) {
       Serial.println("Begin Failed");
       display.println("Begin Failed");
     } else if (error == OTA_CONNECT_ERROR) {
       Serial.println("Connect Failed");
       display.println("Connect Failed");
     } else if (error == OTA_RECEIVE_ERROR) {
       Serial.println("Receive Failed");
       display.println("Receive Failed");
     } else if (error == OTA_END_ERROR) {
       Serial.println("End Failed");
       display.println("End Failed");
     }
     display.display();
   });
   
   ArduinoOTA.begin();
   Serial.println("OTA service started");
 }
  

void getChipInfo() {
   esp_chip_info_t chipInfo;
   esp_chip_info(&chipInfo);
   
   Serial.println("ESP32 Chip Information:");
   Serial.printf("Model: %d\n", chipInfo.model);
   Serial.printf("Cores: %d\n", chipInfo.cores);
   Serial.printf("Feature: %d\n", chipInfo.features);
   Serial.printf("Revision: %d\n", ESP.getChipRevision());
   
   deviceID = String((uint32_t)(ESP.getEfuseMac() >> 32), HEX) + String((uint32_t)ESP.getEfuseMac(), HEX);
   Serial.printf("Chip ID: %s\n", deviceID.c_str());
   Serial.printf("Flash size: %d KB\n", ESP.getFlashChipSize() / 1024);
   Serial.printf("Free heap: %d KB\n", ESP.getFreeHeap() / 1024);
 }
  

void displayBootSequence() {
   display.clearDisplay();
   
   // CADSE Logo animation
   for (int i = 0; i < SCREEN_HEIGHT / 2; i += 4) {
     display.clearDisplay();
     display.drawCircle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, i, SSD1306_WHITE);
     display.display();
     delay(50);
   }
   
   display.clearDisplay();
   display.setCursor(0, 0);
   display.println("  CADSE v5");
   display.println("Space Electronics");
   display.println("Floyd DSouza - 2025");
   display.display();
   delay(2000);
   
   // Display chip info
   display.clearDisplay();
   display.setCursor(0, 0);
   display.println("ESP32-S3 Satellite");
   display.print("ID: ");
   display.println(deviceID.substring(0, 8) + "...");
   display.print("Flash: ");
   display.print(ESP.getFlashChipSize() / 1024 / 1024);
   display.println(" MB");
   display.print("Heap: ");
   display.print(ESP.getFreeHeap() / 1024);
   display.println(" KB");
   display.display();
   delay(2000);
   
   // Display voltage info
   display.clearDisplay();
   display.setCursor(0, 0);
   display.println("Voltage Readings:");
   display.print("Battery: ");
   display.print(batteryVoltage);
   display.println(" V");
   display.print("USB: ");
   display.print(usbVoltage);
   display.println(" V");
   
   if (lowBatteryAlert) {
     display.println("\n** LOW BATTERY **");
     display.println(" Connect USB power");
   }
   
   display.display();
   delay(2000);
 }
  

void handleMQTTCallback(char* topic, byte* payload, unsigned int length) {
   // Convert payload to string
   char message[length + 1];
   for (unsigned int i = 0; i < length; i++) {
     message[i] = (char)payload[i];
   }
   message[length] = '\0';
   
   Serial.print("Message received: [");
   Serial.print(topic);
   Serial.print("] ");
   Serial.println(message);
   
   // Process commands
   if (String(topic) == mqttCommandTopic) {
     String command = String(message);
     
     if (command.startsWith("M") && command.length() == 2) {
       int newMode = command.substring(1).toInt();
       if (newMode >= 0 && newMode <= 5) {
         nextMode = newMode;
         mqttClient.publish(mqttResponseTopic.c_str(), ("Mode changed to " + String(newMode)).c_str());
       } else {
         mqttClient.publish(mqttResponseTopic.c_str(), "Invalid mode number");
       }
     } 
     else if (command.startsWith("SET_DEFAULT_M") && command.length() == 13) {
       int newDefaultMode = command.substring(12).toInt();
       if (newDefaultMode >= 0 && newDefaultMode <= 5) {
         defaultMode = newDefaultMode;
         // Store default mode in non-volatile memory
         preferences.putInt("defMode", defaultMode);
         Serial.print("Default mode set to: ");
         Serial.println(defaultMode);
         mqttClient.publish(mqttResponseTopic.c_str(), ("Default mode set to " + String(defaultMode)).c_str());
       } else {
         mqttClient.publish(mqttResponseTopic.c_str(), "Invalid default mode number");
       }
     }
     else if (command == "OTA_RESTART") {
       mqttClient.publish(mqttResponseTopic.c_str(), "Restarting for OTA update...");
       delay(500);
       ESP.restart();
     }
     else {
       mqttClient.publish(mqttResponseTopic.c_str(), "Unknown command");
     }
   }
 }
  

void sendTelemetry() {
   if (mqttClient.connected()) {
     String telemetryJson = createJSONTelemetry();
     bool success = mqttClient.publish(mqttTelemetryTopic.c_str(), telemetryJson.c_str());
     
     if (success) {
       Serial.println("Telemetry sent: " + telemetryJson);
     } else {
       Serial.println("Failed to send telemetry, error code: " + String(mqttClient.state()));
     }
   } else {
     Serial.println("Cannot send telemetry: MQTT not connected");
   }
 }
  

String createJSONTelemetry() {
   // Manually create JSON
   String json = "{";
   
   // System information
   json += "\"device_id\":\"" + deviceID + "\",";
   json += "\"uptime\":" + String(millis() / 1000) + ",";
   json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
   json += "\"mode\":" + String(currentMode) + ",";
   json += "\"default_mode\":" + String(defaultMode) + ",";
   
   // Voltage readings
   json += "\"battery_voltage\":" + String(batteryVoltage, 2) + ",";
   json += "\"usb_voltage\":" + String(usbVoltage, 2) + ",";
   json += "\"low_battery\":" + String(lowBatteryAlert ? "true" : "false") + ",";
   
   // Touch sensor values
   json += "\"touch_right\":" + String(touchRead(TOUCH_RIGHT)) + ",";
   json += "\"touch_left\":" + String(touchRead(TOUCH_LEFT)) + ",";
   json += "\"touch_up\":" + String(touchRead(TOUCH_UP)) + ",";
   json += "\"touch_down\":" + String(touchRead(TOUCH_DOWN)) + ",";
   json += "\"touch_x\":" + String(touchRead(TOUCH_X)) + ",";
   
   // Add simulated acceleration data (3 axes)
   json += "\"acceleration\":{";
   json += "\"x\":" + String(random(98, 102) / 10.0, 1) + ","; // Around 1g with variation
   json += "\"y\":" + String(random(-5, 5) / 10.0, 1) + ",";   // Near 0g with small variation
   json += "\"z\":" + String(random(-5, 5) / 10.0, 1);          // Near 0g with small variation
   json += "},";
   
   // Add simulated angular rate data (3 axes)
   json += "\"gyro\":{";
   json += "\"x\":" + String(random(-20, 20) / 10.0, 1) + ","; // Random rotation
   json += "\"y\":" + String(random(-20, 20) / 10.0, 1) + ","; // Random rotation
   json += "\"z\":" + String(random(-20, 20) / 10.0, 1);        // Random rotation
   json += "},";
   
   // Environmental data
   if (bme.begin(0x76)) {
     json += "\"temperature\":" + String(bme.readTemperature(), 1) + ",";
     json += "\"pressure\":" + String(bme.readPressure() / 100.0F, 1) + ",";
     json += "\"humidity\":" + String(bme.readHumidity(), 1) + ",";
     json += "\"altitude\":" + String(bme.readAltitude(1013.25), 1) + ",";
   } else {
     json += "\"temperature\":null,";
     json += "\"pressure\":null,";
     json += "\"humidity\":null,";
     json += "\"altitude\":null,";
   }
   
   // Network information
   json += "\"wifi_strength\":" + String(WiFi.RSSI(), 1) + ",";
   json += "\"ip_address\":\"" + WiFi.localIP().toString() + "\"";
   
   json += "}";
   return json;
 }
  

void reconnectMQTT() {
   if (WiFi.status() != WL_CONNECTED) return;
   
   display.clearDisplay();
   display.setCursor(0, 0);
   display.println("Connecting to MQTT...");
   display.display();
   
   Serial.print("Attempting MQTT connection to ");
   Serial.println(MQTT_SERVER);
   
   // Try to connect with a maximum of 3 attempts
   int attempts = 0;
   while (!mqttClient.connected() && attempts < 3) {
     Serial.print("Attempt #");
     Serial.print(attempts + 1);
     Serial.print("... ");
     
     // Connect with client ID and username/password from arduino_secrets.h
     if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
       Serial.println("connected!");
       display.println("Connected!");
       display.display();
       
       // Subscribe to command topic
       mqttClient.subscribe(mqttCommandTopic.c_str());
       Serial.print("Subscribed to: ");
       Serial.println(mqttCommandTopic);
       
       // Send online status
       mqttClient.publish(mqttResponseTopic.c_str(), "{\"status\":\"online\"}");
     } else {
       int errorCode = mqttClient.state();
       Serial.print("failed, rc=");
       Serial.print(errorCode);
       Serial.print(" (");
       
       // Print more detailed error information
       switch(errorCode) {
         case -1: Serial.print("Connection timeout"); break;
         case -2: Serial.print("Connection lost"); break;
         case -3: Serial.print("Connection failed"); break;
         case -4: Serial.print("Server disconnected"); break;
         case -5: Serial.print("Bad protocol"); break;
         case -6: Serial.print("Bad client ID"); break;
         case -7: Serial.print("Connection unavailable"); break;
         case -8: Serial.print("Bad credentials"); break;
         case -9: Serial.print("Unauthorized"); break;
         default: Serial.print("Unknown error"); break;
       }
       
       Serial.println(") trying again in 2 seconds");
       delay(2000);
       attempts++;
     }
   }
   
   if (!mqttClient.connected()) {
     display.println("Failed to connect!");
     display.println("Check MQTT settings");
     display.display();
     delay(2000);
   }
 }
  

void displayModeInfo() {
   display.clearDisplay();
   display.setCursor(0, 0);
   display.println("CADSE Space Electronics");
   display.setTextSize(2);
   display.println("Mode " + String(currentMode));
   display.setTextSize(1);
   
   switch (currentMode) {
     case 0:
       display.println("Basic Monitoring");
       break;
     case 1:
       display.println("Micro-G Detection");
       break;
     case 2:
       display.println("Pressure Monitor");
       break;
     case 3:
       display.println("Attitude Indicator");
       break;
     case 4:
       display.println("Rolling Plotter");
       break;
     case 5:
       display.println("Creative: Orbit Sim");
       break;
     default:
       display.println("Unknown Mode");
       break;
   }
   
   display.display();
   delay(1000);
 }
  

void runCurrentMode() {
   switch (currentMode) {
     case 0:
       runBasicMonitoring();
       break;
     case 1:
       runMicroGravityDetection();
       break;
     case 2:
       runPressureMonitoring();
       break;
     case 3:
       runAttitudeIndicator();
       break;
     case 4:
       runRollingPlotter();
       break;
     case 5:
       runCreativeMode();
       break;
     default:
       runBasicMonitoring();
       break;
   }
 }
  

void runBasicMonitoring() {
   static unsigned long lastUpdateTime = 0;
   const unsigned long updateInterval = 1000; // Update every second
   
   if (millis() - lastUpdateTime < updateInterval) {
     return;
   }
   
   lastUpdateTime = millis();
   
   display.clearDisplay();
   display.setCursor(0, 0);
   display.println("Mode 0: Basic Status");
   display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
   
   // Battery status
   display.setCursor(0, 12);
   display.print("Batt: ");
   display.print(batteryVoltage, 1);
   display.println("V");
   
   // USB status
   display.print("USB: ");
   display.print(usbVoltage, 1);
   display.println("V");
   
   // WiFi status
   display.print("WiFi: ");
   if (WiFi.status() == WL_CONNECTED) {
     display.print("Connected (");
     display.print(WiFi.RSSI());
     display.println("dBm)");
   } else {
     display.println("Disconnected");
   }
   
   // MQTT status
   display.print("MQTT: ");
   display.println(mqttClient.connected() ? "Connected" : "Disconnected");
   
   // Runtime
   display.print("Uptime: ");
   unsigned long uptime = millis() / 1000;
   display.print(uptime / 60);
   display.print("m ");
   display.print(uptime % 60);
   display.println("s");
   
   display.display();
 }
  

void runMicroGravityDetection() {
   static unsigned long lastUpdateTime = 0;
   const unsigned long updateInterval = 200; // Update every 200ms
   static int fallingCounter = 0;
   static bool inFreeFall = false;
   
   // Using touch reading changes to simulate acceleration
   // In a real implementation, this would use MPU6050 data
   int touch1 = touchRead(TOUCH_RIGHT);
   int touch2 = touchRead(TOUCH_LEFT);
   int touchDiff = abs(touch1 - touch2);
   
   // Simulate free-fall detection using touch sensor changes
   // FIXED: Changed threshold from 100 to 15000 to prevent false detections
   bool freeFallDetected = (touchDiff > 15000);
   
   // Debug output when close to threshold
   if (touchDiff > 2500) {
     Serial.print("Touch diff: ");
     Serial.println(touchDiff);
   }
   
   if (freeFallDetected && !inFreeFall) {
     inFreeFall = true;
     fallingCounter = 10; // Fall duration counter
     
     // Alert with buzzer
     tone(BUZZER_PIN, 3000, 200);
     
     Serial.println("MICROGRAVITY DETECTED!");
   }
   
   if (millis() - lastUpdateTime < updateInterval) {
     return;
   }
   
   lastUpdateTime = millis();
   
   display.clearDisplay();
   display.setCursor(0, 0);
   display.println("Mode 1: Micro-G Detector");
   display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
   
   // Status display
   display.setCursor(0, 15);
   display.print("Touch Delta: ");
   display.println(touchDiff);
   
   display.setCursor(0, 30);
   if (inFreeFall) {
     display.setTextSize(2);
     display.println("FALLING!");
     display.setTextSize(1);
     
     fallingCounter--;
     if (fallingCounter <= 0) {
       inFreeFall = false;
     }
   } else {
     display.println("Status: Normal");
     display.println("Touch sensors to simulate");
     display.println("micro-gravity detection");
     display.println("(Free fall < 1G)");
   }
   
   display.display();
 }
  

void runPressureMonitoring() {
   static unsigned long lastUpdateTime = 0;
   const unsigned long updateInterval = 500; // Update every 500ms
   static float basePressure = 0;
   static bool baselineSet = false;
   static bool alertActive = false;
   const float alertThreshold = 10.0; // hPa drop to trigger cat safety alert
   
   // Set baseline pressure
   if (!baselineSet && bme.begin(0x76)) {
     basePressure = bme.readPressure() / 100.0F;
     baselineSet = true;
     Serial.print("Baseline pressure set to: ");
     Serial.println(basePressure);
   }
   
   if (millis() - lastUpdateTime < updateInterval) {
     return;
   }
   
   lastUpdateTime = millis();
   
   display.clearDisplay();
   display.setCursor(0, 0);
   display.println("Mode 2: Pressure Monitor");
   display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
   
   // Read current pressure
   float currentPressure = 0;
   if (bme.begin(0x76)) {
     currentPressure = bme.readPressure() / 100.0F;
     
     // Check specifically for pressure drops (cat safety)
     float pressureDelta = currentPressure - basePressure;
     alertActive = (pressureDelta < -alertThreshold); // Alert only on pressure DROP exceeding threshold
     
     // Display pressure information
     display.setCursor(0, 15);
     display.print("Current: ");
     display.print(currentPressure, 1);
     display.println(" hPa");
     
     display.print("Baseline: ");
     display.print(basePressure, 1);
     display.println(" hPa");
     
     display.print("Delta: ");
     display.print(pressureDelta, 2);
     display.println(" hPa");
     
     // Alert display
     if (alertActive) {
       display.setCursor(0, 45);
       display.setTextSize(1);
       display.println("! CAT SAFETY ALERT !");
       display.println("PRESSURE DROP DETECTED!");
       
       // Visual alert - Rapid LED blinking for visibility
       digitalWrite(LED_PIN, (millis() / 200) % 2); // Faster blink rate
       
       // Audible alert - Alternating alarm tones
       unsigned long alarmPattern = (millis() / 300) % 4;
       switch(alarmPattern) {
         case 0: tone(BUZZER_PIN, 2500, 150); break;
         case 1: tone(BUZZER_PIN, 2000, 150); break;
         case 2: tone(BUZZER_PIN, 2500, 150); break;
         case 3: tone(BUZZER_PIN, 1800, 150); break;
       }
     } else {
       digitalWrite(LED_PIN, LOW);
       display.setCursor(0, 45);
       display.println("Pressure stable");
     }
   } else {
     display.setCursor(0, 20);
     display.println("BME280 sensor not found");
     display.println("Please check connections");
   }
   
   display.display();
 }
  

void runAttitudeIndicator() {
   static unsigned long lastUpdateTime = 0;
   const unsigned long updateInterval = 50; // Update frequently for smooth animation
   static float roll = 0.0;
   static float pitch = 0.0;
   
   // Using touch to control roll
   int touch1 = touchRead(TOUCH_RIGHT);
   int touch2 = touchRead(TOUCH_LEFT);
   
   // Map touch values to roll changes
   if (touch1 < 40) {
     roll += 1.0;
     if (roll > 180) roll -= 360;
   }
   
   if (touch2 < 40) {
     roll -= 1.0;
     if (roll < -180) roll += 360;
   }
   
   if (millis() - lastUpdateTime < updateInterval) {
     return;
   }
   
   lastUpdateTime = millis();
   
   display.clearDisplay();
   
   // Draw attitude indicator
   int centerX = SCREEN_WIDTH / 2;
   int centerY = SCREEN_HEIGHT / 2;
   int radius = 24;
   
   // Draw artificial horizon
   // Calculate line position based on roll and pitch
   float sinRoll = sin(roll * PI / 180.0);
   float cosRoll = cos(roll * PI / 180.0);
   
   // Draw horizon line
   display.drawLine(
     centerX - radius * cosRoll,
     centerY + radius * sinRoll + pitch,
     centerX + radius * cosRoll,
     centerY - radius * sinRoll + pitch,
     SSD1306_WHITE
   );
   
   // Draw aircraft symbol
   display.drawLine(centerX - 10, centerY, centerX + 10, centerY, SSD1306_WHITE);
   display.drawLine(centerX, centerY - 5, centerX, centerY + 5, SSD1306_WHITE);
   
   // Draw outer circle
   display.drawCircle(centerX, centerY, radius, SSD1306_WHITE);
   
   // Draw roll indicator
   display.drawLine(
     centerX,
     centerY - radius,
     centerX + int(8 * sinRoll),
     centerY - radius + int(8 * cosRoll),
     SSD1306_WHITE
   );
   
   // Display roll and pitch values
   display.setTextSize(1);
   display.setCursor(0, 0);
   display.print("Roll: ");
   display.print(int(roll));
   display.println("°");
   
   display.setCursor(64, 0);
   display.print("Pitch: ");
   display.print(int(pitch));
   display.println("°");
   
   display.setCursor(0, 55);
   display.println("Touch to adjust roll");
   
   display.display();
 }
  

void runRollingPlotter() {
   static unsigned long lastUpdateTime = 0;
   const unsigned long updateInterval = 100; // Update every 100ms
   static int dataPoints[SCREEN_WIDTH];
   static int dataIndex = 0;
   static int maxVal = 0;
   
   if (millis() - lastUpdateTime < updateInterval) {
     return;
   }
   
   lastUpdateTime = millis();
   
   // Read sensor data
   float temperature = 0;
   float pressure = 0;
   float humidity = 0;
   
   if (bme.begin(0x76)) {
     temperature = bme.readTemperature();
     pressure = bme.readPressure() / 100.0F;
     humidity = bme.readHumidity();
   }
   
   // Select value to plot based on touch
   int plotValue;
   int touch1 = touchRead(TOUCH_RIGHT);
   int touch2 = touchRead(TOUCH_LEFT);
   
   if (touch1 < 40) {
     // Plot temperature
     plotValue = int(temperature * 5); // Scale for better visualization
   } else if (touch2 < 40) {
     // Plot humidity
     plotValue = int(humidity);
   } else {
     // Plot pressure variation
     plotValue = int((pressure - 1000) * 5); // Normalize around 1000 hPa
   }
   
   // Ensure plotValue is in displayable range
   plotValue = constrain(plotValue, 0, 50);
   
   // Store the new data point
   dataPoints[dataIndex] = plotValue;
   dataIndex = (dataIndex + 1) % SCREEN_WIDTH;
   
   // Find max value for scaling
   maxVal = 0;
   for (int i = 0; i < SCREEN_WIDTH; i++) {
     if (dataPoints[i] > maxVal) maxVal = dataPoints[i];
   }
   maxVal = max(maxVal, 10); // Ensure non-zero scaling
   
   display.clearDisplay();
   
   // Draw title
   display.setTextSize(1);
   display.setCursor(0, 0);
   display.println("Mode 4: Rolling Plotter");
   
   // Display values
   display.setCursor(0, 10);
   display.print("T:");
   display.print(temperature, 1);
   display.print("C ");
   
   display.print("H:");
   display.print(humidity, 0);
   display.print("% ");
   
   display.print("P:");
   display.print(pressure, 0);
   display.println("hPa");
   
   // Draw axes
   display.drawLine(0, 63, 127, 63, SSD1306_WHITE); // X-axis
   display.drawLine(0, 25, 0, 63, SSD1306_WHITE);   // Y-axis
   
   // Draw graph
   for (int i = 0; i < SCREEN_WIDTH - 1; i++) {
     int x1 = i;
     int y1 = 63 - map(dataPoints[(dataIndex + i) % SCREEN_WIDTH], 0, maxVal, 0, 35);
     int x2 = i + 1;
     int y2 = 63 - map(dataPoints[(dataIndex + i + 1) % SCREEN_WIDTH], 0, maxVal, 0, 35);
     
     display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
   }
   
   display.display();
 }
  

void runCreativeMode() {
   static unsigned long lastUpdateTime = 0;
   const unsigned long updateInterval = 50; // Update frequently for smooth animation
   static float orbitAngle = 0.0;
   static float satelliteAngle = 0.0;
   static float orbitSpeed = 0.5;
   static int earthRadius = 15;
   static int orbitRadius = 25;
   
   // Control orbit speed with touch
   int touch1 = touchRead(TOUCH_RIGHT);
   int touch2 = touchRead(TOUCH_LEFT);
   
   if (touch1 < 40) {
     orbitSpeed += 0.1;
     if (orbitSpeed > 5.0) orbitSpeed = 5.0;
   }
   
   if (touch2 < 40) {
     orbitSpeed -= 0.1;
     if (orbitSpeed < 0.1) orbitSpeed = 0.1;
   }
   
   if (millis() - lastUpdateTime < updateInterval) {
     return;
   }
   
   lastUpdateTime = millis();
   
   // Update angles
   orbitAngle += orbitSpeed;
   if (orbitAngle > 360) orbitAngle -= 360;
   
   satelliteAngle += orbitSpeed * 3; // Satellite rotation
   if (satelliteAngle > 360) satelliteAngle -= 360;
   
   display.clearDisplay();
   
   // Draw title
   display.setTextSize(1);
   display.setCursor(0, 0);
   display.println("Mode 5: Orbit Simulator");
   
   // Display orbit info
   display.setCursor(0, 56);
   display.print("Speed: ");
   display.print(orbitSpeed, 1);
   display.print(" (Touch to adjust)");
   
   // Draw Earth
   int centerX = SCREEN_WIDTH / 2;
   int centerY = 32;
   display.fillCircle(centerX, centerY, earthRadius, SSD1306_WHITE);
   display.fillCircle(centerX + 2, centerY - 2, earthRadius - 4, SSD1306_BLACK);
   
   // Draw orbit path
   display.drawCircle(centerX, centerY, orbitRadius, SSD1306_WHITE);
   
   // Calculate satellite position
   int satX = centerX + int(orbitRadius * cos(orbitAngle * PI / 180.0));
   int satY = centerY + int(orbitRadius * sin(orbitAngle * PI / 180.0));
   
   // Draw satellite
   display.fillRect(satX - 2, satY - 2, 4, 4, SSD1306_WHITE);
   
   // Draw satellite solar panels
   float sinRot = sin(satelliteAngle * PI / 180.0);
   float cosRot = cos(satelliteAngle * PI / 180.0);
   
   display.drawLine(
     satX, satY,
     satX + int(8 * cosRot),
     satY + int(8 * sinRot),
     SSD1306_WHITE
   );
   
   display.drawLine(
     satX, satY,
     satX - int(8 * cosRot),
     satY - int(8 * sinRot),
     SSD1306_WHITE
   );
   
   display.display();
 }