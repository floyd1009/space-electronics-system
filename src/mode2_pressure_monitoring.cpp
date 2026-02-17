#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// Mode 2: Pressure Monitoring Window
// Monitors pressure changes and triggers alerts for cat safety

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
