#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <SoftwareSerial.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 16
#define CLK_PIN 13
#define DATA_PIN 11
#define CS_PIN 3

SoftwareSerial sensorSerial(4, 5);  // RX, TX for PMS7003

MD_Parola p = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

char displayText[100] = "Air Quality Display        Adyanta     ";
char pm1Text[32] = "PM1        : 0";
int pm1 = 0, pm25 = 0, pm10 = 0;
unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 10000;

// Uncomment for debug output during standalone testing
//#define DEBUG

void setup() {
  Serial.begin(9600);  // For PM2.5, PM10 to Uno 2 or debug
  sensorSerial.begin(9600);
  p.begin(2);
  p.setZone(0, 0, 7);  // Row 1: modules 0-7
  p.setZone(1, 8, 15); // Row 2: modules 8-15
  p.setIntensity(8);
  p.setInvert(false);
  p.displayClear();
  p.displayZoneText(0, displayText, PA_CENTER, 50, 5000, PA_SCROLL_LEFT, PA_NO_EFFECT);
  p.displayZoneText(1, pm1Text, PA_LEFT, 100, 0, PA_NO_EFFECT, PA_NO_EFFECT);
#ifdef DEBUG
  Serial.println("Uno 1: Initialized - Row 1 (AIR QUALITY DISPLAY), Row 2 (PM1: 0)");
  Serial.println("Uno 1: PMS7003 init, waiting 30s for warm-up...");
#endif
  delay(10000);
}

void loop() {
  unsigned long currentTime = millis();
  if (currentTime - lastUpdate >= UPDATE_INTERVAL) {
    updateSensorData();
    lastUpdate = currentTime;
  }
// Animate display for both zones
  if (p.displayAnimate()) {
    // Restart scrolling for zone 0 (AIR QUALITY DISPLAY)
    if (p.getZoneStatus(0)) {
      p.displayReset(0);
    }
  }
}   


void updateSensorData() {
  if (readPMS7003()) {
    char temp1[32];
    sprintf(temp1, " PM1      : %d", pm1);
    strncpy(pm1Text, temp1, sizeof(pm1Text));
#ifdef DEBUG
    Serial.print("Uno 1: "); Serial.println(pm1Text);
#endif
    p.displayZoneText(1, pm1Text, PA_LEFT, 100, 0, PA_NO_EFFECT, PA_NO_EFFECT);
    Serial.print(pm25); Serial.print(","); Serial.println(pm10);
  } else {
#ifdef DEBUG
    Serial.println("Uno 1: No valid PMS7003 data");
#endif
  }
}

bool readPMS7003() {
  static byte buffer[32];
  static int bufferIndex = 0;

  while (sensorSerial.available()) {
    buffer[bufferIndex] = sensorSerial.read();
    if (bufferIndex == 0 && buffer[0] != 0x42) continue;
    if (bufferIndex == 1 && buffer[1] != 0x4D) {
      bufferIndex = 0;
      continue;
    }
    if (bufferIndex == 31) {
      uint16_t checksum = 0;
      for (int i = 0; i < 30; i++) checksum += buffer[i];
      uint16_t receivedChecksum = (buffer[30] << 8) + buffer[31];
      if (checksum == receivedChecksum) {
        pm1 = (buffer[8] << 8) + buffer[9];
        pm25 = (buffer[10] << 8) + buffer[11];
        pm10 = (buffer[12] << 8) + buffer[13];
        bufferIndex = 0;
#ifdef DEBUG
        Serial.print("Uno 1: PM1: "); Serial.print(pm1);
        Serial.print(", PM2.5: "); Serial.print(pm25);
        Serial.print(", PM10: "); Serial.println(pm10);
#endif
        return true;
      } else {
#ifdef DEBUG
        Serial.println("Uno 1: Checksum failed");
#endif
        bufferIndex = 0;
      }
    } else {
      bufferIndex++;
    }
  }
  return false;
}