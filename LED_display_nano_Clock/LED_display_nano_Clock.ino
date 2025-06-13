#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 16
#define CLK_PIN 13
#define DATA_PIN 11
#define CS_PIN 10

MD_Parola p = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

char displayText[100] = "Air Quality Display        Adyanta     ";
char clockText[32] = "00:00:00";
unsigned long lastUpdate = 0;
unsigned long lastClockUpdate = 0;
const unsigned long DISPLAY_REFRESH_INTERVAL = 30000;  // 30 seconds
const unsigned long CLOCK_UPDATE_INTERVAL = 1000;      // 1 second
unsigned long secondsSinceStart = 0;

void setup() {
  Serial.begin(9600);  // For debug
  p.begin(2);
  p.setZone(0, 0, 7);   // Strip 1: modules 0-7 (scrolling text)
  p.setZone(1, 8, 15);  // Strip 2: modules 8-15 (clock)
  p.setIntensity(8);
  p.setInvert(false);
  p.displayClear();
  p.displayZoneText(0, displayText, PA_CENTER, 50, 5000, PA_SCROLL_LEFT, PA_NO_EFFECT);
  p.displayZoneText(1, clockText, PA_CENTER, 100, 0, PA_NO_EFFECT, PA_NO_EFFECT);
  Serial.println("Nano: Initialized - Strip 1 (AIR QUALITY DISPLAY), Strip 2 (CLOCK)");
  updateClock();  // Initialize clock display
}

void loop() {
  unsigned long currentTime = millis();

  // Update clock every 1 second
  if (currentTime - lastClockUpdate >= CLOCK_UPDATE_INTERVAL) {
    secondsSinceStart++;
    updateClock();
    lastClockUpdate = currentTime;
  }

  // Refresh display every 30 seconds
  if (currentTime - lastUpdate >= DISPLAY_REFRESH_INTERVAL) {
    p.displayZoneText(0, displayText, PA_CENTER, 50, 5000, PA_SCROLL_LEFT, PA_NO_EFFECT);
    p.displayZoneText(1, clockText, PA_CENTER, 100, 0, PA_NO_EFFECT, PA_NO_EFFECT);
    Serial.print("Nano: Clock refreshed - "); Serial.println(clockText);
    lastUpdate = currentTime;
  }

  p.displayAnimate();  // Keep Strip 1 scrolling
}

void updateClock() {
  // Calculate hours, minutes, seconds from secondsSinceStart
  unsigned long totalSeconds = secondsSinceStart;
  int hours = (totalSeconds / 3600) % 24;
  int minutes = (totalSeconds / 60) % 60;
  int seconds = totalSeconds % 60;

  // Format clock text
  sprintf(clockText, "%02d:%02d:%02d", hours, minutes, seconds);
  p.displayZoneText(1, clockText, PA_CENTER, 100, 0, PA_NO_EFFECT, PA_NO_EFFECT);
  Serial.print("Nano: Clock updated - "); Serial.println(clockText);
}