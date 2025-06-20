#include <DMD2.h>
#include <fonts/Arial14.h>

#define DISPLAYS_WIDE 2   // 2 displays wide = 64px
#define DISPLAYS_HIGH 2   // 2 displays tall = 32px

SoftDMD dmd(DISPLAYS_WIDE, DISPLAYS_HIGH);

int pm25 = 0, pm10 = 0;

void setup() {
  Serial.begin(9600);  // PMS7003 on hardware Serial (pins 0/1)
  dmd.begin();
  dmd.selectFont(Arial14);
  dmd.clearScreen();
  dmd.drawString(0, 0, "Initializing...");
  delay(2000);
  dmd.clearScreen();
}

bool readPMS() {
  static byte buffer[32];
  static int index = 0;

  while (Serial.available()) {
    byte b = Serial.read();

    if (index == 0 && b != 0x42) continue;
    if (index == 1 && b != 0x4D) { index = 0; continue; }

    buffer[index++] = b;

    if (index == 32) {
      index = 0;
      uint16_t sum = 0;
      for (int i = 0; i < 30; i++) sum += buffer[i];
      uint16_t checksum = (buffer[30] << 8) | buffer[31];

      if (sum == checksum) {
        pm25 = (buffer[12] << 8) | buffer[13];
        pm10 = (buffer[14] << 8) | buffer[15];
        return true;
      }
    }
  }
  return false;
}

void loop() {
  if (readPMS()) {
    // Debug output on Serial Monitor
    Serial.print("PM2.5: "); Serial.print(pm25);
    Serial.print(" | PM10: "); Serial.println(pm10);

    // Display on P10
    dmd.clearScreen();

    String line1 = "PM2.5 :" + String(pm25);
    String line2 = "PM10 :" + String(pm10);

    dmd.drawString(0, 0, line1.c_str());   // Row 1 (0 px Y)
    dmd.drawString(0, 16, line2.c_str());  // Row 2 (16 px Y)
    
    delay(5000);  // Update every 5 seconds
  }
}
