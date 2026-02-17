#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Mode 5: Creative Mode - Orbit Simulator Window
// Provides a visual simulation of satellite orbit around Earth

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
