#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// Mode 4: Rolling Plotter Window
// Displays real-time graph of sensor values

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
