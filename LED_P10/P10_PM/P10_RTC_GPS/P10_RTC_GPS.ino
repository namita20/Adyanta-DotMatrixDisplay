#include <SPI.h>
#include <DMD2.h>
#include <Wire.h>
#include <RTClib.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include "fonts/Arial_Black_16.h"

// Display config
#define DISPLAYS_ACROSS 2
#define DISPLAYS_DOWN 1
SoftDMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN);
char buffer[20];

// RTC
RTC_DS3231 rtc;

// GPS
#define GPS_RX 3
#define GPS_TX 4
SoftwareSerial gpsSerial(GPS_RX, GPS_TX);
TinyGPSPlus gps;

// Buttons
#define BTN_UP     2
#define BTN_DOWN   5
#define BTN_SELECT 10
#define BTN_BACK   12

// Modes
enum TimeMode { GPS_MODE, RTC_MODE };
TimeMode currentMode = RTC_MODE;

enum MenuState {
  MENU_NONE,
  MENU_SELECT_SOURCE,
  MENU_SET_HOUR,
  MENU_SET_MIN,
  MENU_SET_SEC
};
MenuState menuState = MENU_NONE;

int menuIndex = 0;
int setHour = 0, setMin = 0, setSec = 0;
unsigned long lastButtonTime = 0;
const unsigned long debounceDelay = 200;

// --- Button helper ---
bool buttonPressed(int pin) {
  if (digitalRead(pin) == LOW && millis() - lastButtonTime > debounceDelay) {
    lastButtonTime = millis();
    return true;
  }
  return false;
}

// --- Display helper ---
void drawText(const char* msg) {
  dmd.clearScreen();
  dmd.drawString(1, 0, msg, GRAPHICS_NOR);
}

// --- Format GPS UTC to IST ---
void formatISTFromGPS(char* buf) {
  int h = gps.time.hour() + 5;
  int m = gps.time.minute() + 30;
  int s = gps.time.second();

  if (m >= 60) { m -= 60; h++; }
  if (h >= 24) h -= 24;

  sprintf(buf, "%02d:%02d:%02d", h, m, s);
}

void setup() {
  Serial.begin(9600);
  Wire.begin();
  gpsSerial.begin(9600);

  dmd.begin();
  dmd.setBrightness(255);
  dmd.clearScreen();
  dmd.selectFont(Arial_Black_16);

  if (!rtc.begin()) {
    Serial.println("RTC not found!");
    while (1);
  }

  if (rtc.lostPower()) {
    rtc.adjust(DateTime(2025, 6, 17, 12, 0, 0));
  }

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);
}

void loop() {
  // Read GPS
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  switch (menuState) {
    case MENU_NONE:
      if (currentMode == GPS_MODE) {
        if (gps.time.isValid()) {
          formatISTFromGPS(buffer);
        } else {
          sprintf(buffer, "WAIT GPS");
        }
      } else if (currentMode == RTC_MODE) {
        DateTime now = rtc.now();
        sprintf(buffer, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
      }

      drawText(buffer);

      if (buttonPressed(BTN_SELECT)) {
        menuState = MENU_SELECT_SOURCE;
        menuIndex = 0;
      }
      break;

    case MENU_SELECT_SOURCE:
      drawText(menuIndex == 0 ? "GPS" : "RTC");

      if (buttonPressed(BTN_UP) || buttonPressed(BTN_DOWN)) {
        menuIndex = (menuIndex + 1) % 2;
      }

      if (buttonPressed(BTN_SELECT)) {
        if (menuIndex == 0) {
          currentMode = GPS_MODE;
          menuState = MENU_NONE;
        } else {
          currentMode = RTC_MODE;
          DateTime now = rtc.now();
          setHour = now.hour();
          setMin = now.minute();
          setSec = now.second();
          menuState = MENU_SET_HOUR;
        }
      }

      if (buttonPressed(BTN_BACK)) {
        menuState = MENU_NONE;
      }
      break;

    case MENU_SET_HOUR:
      sprintf(buffer, "Hr: %02d", setHour);
      drawText(buffer);
      if (buttonPressed(BTN_UP))   setHour = (setHour + 1) % 24;
      if (buttonPressed(BTN_DOWN)) setHour = (setHour + 23) % 24;
      if (buttonPressed(BTN_SELECT)) menuState = MENU_SET_MIN;
      if (buttonPressed(BTN_BACK))   menuState = MENU_NONE;
      break;

    case MENU_SET_MIN:
      sprintf(buffer, "Min: %02d", setMin);
      drawText(buffer);
      if (buttonPressed(BTN_UP))   setMin = (setMin + 1) % 60;
      if (buttonPressed(BTN_DOWN)) setMin = (setMin + 59) % 60;
      if (buttonPressed(BTN_SELECT)) menuState = MENU_SET_SEC;
      if (buttonPressed(BTN_BACK))   menuState = MENU_NONE;
      break;

    case MENU_SET_SEC:
      sprintf(buffer, "Sec: %02d", setSec);
      drawText(buffer);
      if (buttonPressed(BTN_UP))   setSec = (setSec + 1) % 60;
      if (buttonPressed(BTN_DOWN)) setSec = (setSec + 59) % 60;
      if (buttonPressed(BTN_SELECT)) {
        rtc.adjust(DateTime(2025, 6, 17, setHour, setMin, setSec)); // RTC set
        menuState = MENU_NONE;
      }
      if (buttonPressed(BTN_BACK)) menuState = MENU_NONE;
      break;
  }

  delay(1000);
}
