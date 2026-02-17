#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Mode 0: Basic Monitoring Window
// Displays system status information

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
