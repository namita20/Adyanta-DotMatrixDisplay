#include <DMD2.h>
#include <fonts/Arial14.h>

#define DISPLAYS_WIDE 2
#define DISPLAYS_HIGH 2
SoftDMD dmd(DISPLAYS_WIDE, DISPLAYS_HIGH);  // Create a DMD object

void setup() {
  Serial.begin(9600);  // Read from ESP32 TX2
  dmd.selectFont(Arial14);
  dmd.begin();
  dmd.drawString(0, 0, "Waiting...");
}

void loop() {
  static String received = "";

  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      dmd.clearScreen();

      // Parse PM2.5 and PM10
      int pm25Index = received.indexOf("PM2.5:");
      int pm10Index = received.indexOf("PM10:");

      if (pm25Index != -1 && pm10Index != -1) {
        String pm25 = received.substring(pm25Index + 6, received.indexOf('|', pm25Index + 6));
        String pm10 = received.substring(pm10Index + 5);

        dmd.drawString(0, 0, "PM2.5:" + pm25);
        dmd.drawString(0, 16, "PM10:" + pm10);
      } else {
        dmd.drawString(0, 0, "Invalid data");
      }

      Serial.println("Received: " + received);
      received = "";
    } else {
      received += c;
    }
  }
}
