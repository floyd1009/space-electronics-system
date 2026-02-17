#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Mode 3: Attitude Indicator Window
// Provides a visual artificial horizon display for landing maneuvers

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
