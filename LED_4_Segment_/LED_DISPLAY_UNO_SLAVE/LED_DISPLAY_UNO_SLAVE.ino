#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <SoftwareSerial.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 16
#define CLK_PIN 13
#define DATA_PIN 11
#define CS_PIN 3

SoftwareSerial uno1Serial(6, 7);  // RX, TX (TX unused)

MD_Parola p = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

char pm25Text[32] = "PM2.5  : 0";
char pm10Text[32] = "PM10    : 0";
int pm25 = 0, pm10 = 0;
String serialBuffer = "";

void setup() {
  Serial.begin(9600);
  uno1Serial.begin(9600);
  p.begin(2);
  p.setZone(0, 0, 7);   // Row 3: modules 0-7
  p.setZone(1, 8, 15);  // Row 4: modules 8-15
  p.setIntensity(8);
  p.setInvert(false);
  p.displayClear();
  p.displayZoneText(0, pm25Text, PA_LEFT, 100, 0, PA_NO_EFFECT, PA_NO_EFFECT);
  p.displayZoneText(1, pm10Text, PA_LEFT, 100, 0, PA_NO_EFFECT, PA_NO_EFFECT);
  Serial.println("Uno 2: Initialized - Row 3 (PM2.5), Row 4 (PM10)");
  Serial.println("Uno 2: Waiting for serial data from Uno 1...");
}

void loop() {
  processSerialData();
  p.displayAnimate();
}

void processSerialData() {
  while (uno1Serial.available()) {
    char c = uno1Serial.read();
    if (c == '\n') {
      int comma = serialBuffer.indexOf(',');
      if (comma != -1) {
        pm25 = serialBuffer.substring(0, comma).toInt();
        pm10 = serialBuffer.substring(comma + 1).toInt();
        char temp25[32], temp10[32];
        sprintf(temp25, " PM2.5   : %d", pm25);
        sprintf(temp10, " PM10    : %d", pm10);
        strncpy(pm25Text, temp25, sizeof(pm25Text));
        strncpy(pm10Text, temp10, sizeof(pm10Text));
        Serial.print("Uno 2: "); Serial.print(pm25Text); Serial.print(", "); Serial.println(pm10Text);
        p.displayZoneText(0, pm25Text, PA_LEFT, 100, 0, PA_NO_EFFECT, PA_NO_EFFECT);
        p.displayZoneText(1, pm10Text, PA_LEFT, 100, 0, PA_NO_EFFECT, PA_NO_EFFECT);
      } else {
        Serial.println("Uno 2: Invalid data format: " + serialBuffer);
      }
      serialBuffer = "";
    } else {
      serialBuffer += c;
    }
  }
}
