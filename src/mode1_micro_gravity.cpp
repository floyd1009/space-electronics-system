#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Mode 1: Micro-Gravity Detection Window
// Detects microgravity conditions using touch sensor differences

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
