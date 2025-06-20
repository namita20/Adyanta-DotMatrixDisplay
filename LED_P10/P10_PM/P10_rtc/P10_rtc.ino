#include <SPI.h>
#include <DMD2.h>
#include <Wire.h>
#include <RTClib.h>
#include "fonts/Arial_Black_16.h" // Ensure this file is in the 'fonts' subfolder

#define DISPLAYS_ACROSS 2
#define DISPLAYS_DOWN 1
SoftDMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN); // Two 32x16 P10 panels (64x16)

RTC_DS3231 rtc;
char timeStr[9]; // Buffer for time string (HH:MM:SS)

void setup() {
  // Initialize Serial for debugging
  Serial.begin(9600);
  
  // Initialize P10 display
  dmd.begin();
  dmd.setBrightness(100); // Set brightness (0-255, adjust as needed)
  dmd.clearScreen();
  dmd.selectFont(Arial_Black_16);
  
  // Initialize RTC
  Wire.begin();
  if (!rtc.begin()) {
    Serial.println("RTC not found!");
    while (1); // Stop if RTC not found
  }
  
  // Set RTC to current time (7:49 PM IST, June 17, 2025) on first upload
  // Uncomment, upload, then comment out and re-upload to avoid resetting
  // rtc.adjust(DateTime(2025, 6, 17, 19, 49, 0));
  
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting time!");
    rtc.adjust(DateTime(2025, 6, 17, 19, 49, 0));
  }
}

void loop() {
  // Get current time from RTC
  DateTime now = rtc.now();
  
  // Format time as HH:MM:SS
  sprintf(timeStr, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  
  // Print to Serial for debugging
  Serial.println(timeStr);
  
  // Display time on P10
  dmd.clearScreen();
  dmd.drawString(2, 1, timeStr, GRAPHICS_NOR);
  delay(1000); // Update every second
}