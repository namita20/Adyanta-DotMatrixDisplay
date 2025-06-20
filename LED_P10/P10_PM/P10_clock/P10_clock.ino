#include <SPI.h>
#include <DMD2.h>
#include <fonts/SystemFont5x7.h>

const int WIDTH = 2; // 2x1 P10 display
const uint8_t *FONT = SystemFont5x7; // Small font to fit time

SoftDMD dmd(WIDTH, 1); // DMD controls the entire display

unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 1000; // Update display every second

// Time variables (start at 00:00:00)
int hours = 0;
int minutes = 0;
int seconds = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("Simple Clock Starting...");

  // Initialize P10 display
  dmd.setBrightness(20);
  dmd.selectFont(FONT);
  dmd.begin();
  dmd.drawString(0, 0, "Starting...");
  delay(2000);
  dmd.clearScreen();
}

void loop() {
  // Update time every second
  if (millis() - lastUpdateTime >= updateInterval) {
    // Increment time
    seconds++;
    if (seconds >= 60) {
      seconds = 0;
      minutes++;
      if (minutes >= 60) {
        minutes = 0;
        hours++;
        if (hours >= 24) {
          hours = 0;
        }
      }
    }

    // Format time as HH:MM:SS
    char timeStr[9]; // Buffer for "HH:MM:SS\0"
    sprintf(timeStr, "%02d:%02d:%02d", hours, minutes, seconds);

    // Update display
    dmd.clearScreen(); // Clear previous time
    dmd.drawString(0, 0, timeStr); // Display new time

    // Debug to Serial
    Serial.print("Time: ");
    Serial.println(timeStr);

    lastUpdateTime = millis();
  }
}