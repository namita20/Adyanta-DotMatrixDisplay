#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 32
#define CLK_PIN 18
#define DATA_PIN 23
#define CS_PIN 5

MD_Parola p = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

char displayText[100] = "Air Quality Display        Adyanta     ";
char pm1Text[32] = "PM1        : N/A";
char pm25Text[32] = "PM2.5  : N/A";
char pm10Text[32] = "PM10    : N/A";
int pm1 = -1, pm25 = -1, pm10 = -1;  // -1 indicates invalid data
unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 10000;  // 10 seconds
static byte buffer[32];
static int bufferIndex = 0;

// Use HardwareSerial2 for PMS7003
HardwareSerial sensorSerial(2);  // RX: 16, TX: 17

void setup() {
  Serial.begin(115200);  // For debug
  sensorSerial.begin(9600, SERIAL_8N1, 16, 17);
  p.begin(4);
  p.setZone(0, 0, 7);   // Row 1: modules 0-7
  p.setZone(1, 8, 15);  // Row 2: modules 8-15
  p.setZone(2, 16, 23); // Row 3: modules 16-23
  p.setZone(3, 24, 31); // Row 4: modules 24-31
  p.setIntensity(8);
  p.setInvert(false);
  p.displayClear();
  p.displayZoneText(0, displayText, PA_CENTER, 50, 5000, PA_SCROLL_LEFT, PA_NO_EFFECT);
  p.displayZoneText(1, pm1Text, PA_LEFT, 100, 0, PA_NO_EFFECT, PA_NO_EFFECT);
  p.displayZoneText(2, pm25Text, PA_LEFT, 100, 0, PA_NO_EFFECT, PA_NO_EFFECT);
  p.displayZoneText(3, pm10Text, PA_LEFT, 100, 0, PA_NO_EFFECT, PA_NO_EFFECT);
  Serial.println("ESP32: Initialized - Rows 1-4");
  Serial.println("ESP32: PMS7003 init, waiting 30s for warm-up...");
  delay(30000);
}

void loop() {
  unsigned long currentTime = millis();
  if (currentTime - lastUpdate >= UPDATE_INTERVAL) {
    processSerialData();
    lastUpdate = currentTime;
  }
  if (p.displayAnimate()) {
    // Restart scrolling for zone 0 (AIR QUALITY DISPLAY)
    if (p.getZoneStatus(0)) {
      p.displayReset(0);
    }
  }
}  

void processSerialData() {
  unsigned long startTime = millis();
  bufferIndex = 0;

  // Flush buffer with timeout
  while (sensorSerial.available() && (millis() - startTime < 5000)) {
    sensorSerial.read();
  }

  // Read packet with timeout
  while (millis() - startTime < 5000) {
    if (sensorSerial.available()) {
      buffer[bufferIndex] = sensorSerial.read();
      if (bufferIndex == 0 && buffer[0] != 0x42) continue;
      if (bufferIndex == 1 && buffer[1] != 0x4D) {
        bufferIndex = 0;
        continue;
      }
      if (bufferIndex == 31) {
        // Debug: Print raw packet
        Serial.print("ESP32: Packet: ");
        for (int i = 0; i < 32; i++) {
          Serial.print(buffer[i], HEX);
          Serial.print(" ");
        }
        Serial.println();

        // Verify checksum
        uint16_t checksum = 0;
        for (int i = 0; i < 30; i++) checksum += buffer[i];
        uint16_t receivedChecksum = (buffer[30] << 8) + buffer[31];
        if (checksum == receivedChecksum) {
          pm1 = (buffer[8] << 8) + buffer[9];
          pm25 = (buffer[10] << 8) + buffer[11];
          pm10 = (buffer[12] << 8) + buffer[13];
        } else {
          Serial.println("ESP32: Checksum failed");
          pm1 = pm25 = pm10 = -1;  // Invalidate data
        }

        // Update display text regardless of packet validity
        char temp1[32], temp25[32], temp10[32];
        if (pm1 < 0) sprintf(temp1, "PM1        : N/A");
        else sprintf(temp1, "PM1        : %d", pm1);
        if (pm25 < 0) sprintf(temp25, "PM2.5  : N/A");
        else sprintf(temp25, "PM2.5  : %d", pm25);
        if (pm10 < 0) sprintf(temp10, "PM10    : N/A");
        else sprintf(temp10, "PM10    : %d", pm10);

        strncpy(pm1Text, temp1, sizeof(pm1Text));
        strncpy(pm25Text, temp25, sizeof(pm25Text));
        strncpy(pm10Text, temp10, sizeof(pm10Text));

        // Print to Serial Monitor
        Serial.print("ESP32: "); 
        Serial.print(pm1Text); Serial.print(", ");
        Serial.print(pm25Text); Serial.print(", "); 
        Serial.println(pm10Text);

        // Update display
        p.displayZoneText(1, pm1Text, PA_LEFT, 100, 0, PA_NO_EFFECT, PA_NO_EFFECT);
        p.displayZoneText(2, pm25Text, PA_LEFT, 100, 0, PA_NO_EFFECT, PA_NO_EFFECT);
        p.displayZoneText(3, pm10Text, PA_LEFT, 100, 0, PA_NO_EFFECT, PA_NO_EFFECT);

        bufferIndex = 0;
        return;
      } else {
        bufferIndex++;
      }
    }
  }

  // Timeout: Update display with N/A if no valid packet
  Serial.println("ESP32: No valid packet received within timeout");
  pm1 = pm25 = pm10 = -1;
  sprintf(pm1Text, "PM1        : N/A");
  sprintf(pm25Text, "PM2.5  : N/A");
  sprintf(pm10Text, "PM10    : N/A");
  Serial.print("ESP32: "); 
  Serial.print(pm1Text); Serial.print(", ");
  Serial.print(pm25Text); Serial.print(", "); 
  Serial.println(pm10Text);
  p.displayZoneText(1, pm1Text, PA_LEFT, 100, 0, PA_NO_EFFECT, PA_NO_EFFECT);
  p.displayZoneText(2, pm25Text, PA_LEFT, 100, 0, PA_NO_EFFECT, PA_NO_EFFECT);
  p.displayZoneText(3, pm10Text, PA_LEFT, 100, 0, PA_NO_EFFECT, PA_NO_EFFECT);
}