#include <SoftwareSerial.h>
#include <SPI.h>
#include <DMD2.h>
#include <fonts/Arial14.h>

// Display configuration
const int WIDTH = 3; // 2 P10 panels wide (64x16 pixels)
const uint8_t *FONT = Arial14;

SoftDMD dmd(WIDTH, 1);  // SoftDMD for 2x1 panels

// PMS7003 sensor pins
#define PMS_RX_PIN 2
#define PMS_TX_PIN 3

SoftwareSerial serial(PMS_RX_PIN, PMS_TX_PIN);  // RX = 2, TX = 3

// State variables
String currentMsg = "PM2.5: N/A";
bool displayInitialized = false;
bool messageUpdated = false;
unsigned long lastDisplayUpdate = 0;
const unsigned long displayRefreshInterval = 10000;  // 10 second


void setup() {
  Serial.begin(115200);
  serial.begin(9600);

  Serial.println("PMS7003 Initializing...");
  delay(10000); // Wait for sensor to boot up

  // Set PMS7003 to active mode
  Serial.println("Setting PMS7003 to active mode...");
  byte activeModeCmd[] = {0x42, 0x4D, 0xE1, 0x00, 0x01, 0x01, 0x71};
  serial.write(activeModeCmd, sizeof(activeModeCmd));
  delay(100);

  Serial.println("Waiting 10s for PMS7003 to stabilize...");
  delay(10000);
}

void loop() {
  static uint8_t buffer[32];
  static uint8_t index = 0;

  if (displayInitialized) dmd.end();

  while (serial.available()) {
    buffer[index] = serial.read();

    if (index == 0 && buffer[0] != 0x42) continue;
    if (index == 1 && buffer[1] != 0x4D) { index = 0; continue; }

    index++;

    if (index >= 32) {
      index = 0;

      uint16_t sum = 0;
      for (int i = 0; i < 30; i++) sum += buffer[i];
      uint16_t checksum = (buffer[30] << 8) + buffer[31];

      if (sum == checksum) {
        uint16_t pm2_5 = (buffer[6] << 8) + buffer[7];
        currentMsg = "PM2.5: " + String(pm2_5) + "ug/m3";
        messageUpdated = true;

        Serial.println(currentMsg);
      } else {
        Serial.println("Checksum error!");
      }
    }
  }

  if (displayInitialized) dmd.beginNoTimer();

  // Initialize display if needed
  if (!displayInitialized) {
    dmd.begin();
    dmd.selectFont(FONT);
    dmd.setBrightness(225);
    dmd.clearScreen();
    displayInitialized = true;
  }

  // Always update display every 10 seconds
  if (millis() - lastDisplayUpdate >= displayRefreshInterval) {
    dmd.clearScreen();
    int textLen = currentMsg.length();
    int pixelWidth = textLen * 7;
    int x = max((WIDTH * 32 - pixelWidth) / 2, 0);
    dmd.drawString(x, 0, currentMsg.c_str());
    lastDisplayUpdate = millis();
  }

  delay(100); // prevent tight loop
}
