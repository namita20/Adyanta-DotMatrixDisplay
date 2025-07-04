#include <SPI.h>
#include <DMD2.h>
#include <Wire.h>
#include <RTClib.h>
#include "fonts/Arial_Black_16.h"

#define DISPLAYS_ACROSS 2
#define DISPLAYS_DOWN 1
SoftDMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN);
char buffer[30];

// RTC
RTC_DS3231 rtc;

// Buttons
#define BTN_UP     2
#define BTN_DOWN   5
#define BTN_SELECT 10
#define BTN_BACK   12

enum TimeMode { GPS_MODE, RTC_MODE };
TimeMode currentMode = GPS_MODE;

enum MenuState {
  MENU_NONE,
  MENU_MAIN,
  MENU_RTC,
  MENU_SET_HOUR,
  MENU_SET_MIN,
  MENU_SET_SEC,
  MENU_GPS,
  MENU_TEMP_TOGGLE
};
MenuState menuState = MENU_NONE;

enum DisplayToggleState {
  SHOW_TIME,
  SHOW_TEMP
};
DisplayToggleState displayToggleState = SHOW_TIME;

// UI
int mainMenuIndex = 0;
int tempToggleMenuIndex = 0;
int setHour = 0, setMin = 0, setSec = 0;
unsigned long lastButtonTime = 0;
const unsigned long debounceDelay = 200;

unsigned long toggleTimer = 0;
const unsigned long TIME_DISPLAY_DURATION = 10000;
const unsigned long TEMP_DISPLAY_DURATION = 5000;
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_REFRESH_INTERVAL = 1000; // Refresh every 1s

bool tempDisplayEnabled = true;

String receivedTime = "";
String receivedTemp = "";
String gpsBuffer = "";
String lastText = ""; // Track last displayed text

// Menu Labels
const char* mainMenuItems[] = {
  "RTC",
  "GPS",
  "Temp"
};
const int MAIN_MENU_ITEM_COUNT = sizeof(mainMenuItems) / sizeof(mainMenuItems[0]);

const char* tempToggleMenuItems[] = {
  "ON",
  "OFF"
};
const int TEMP_TOGGLE_MENU_COUNT = sizeof(tempToggleMenuItems) / sizeof(tempToggleMenuItems[0]);

bool buttonPressed(int pin) {
  if (digitalRead(pin) == LOW && millis() - lastButtonTime > debounceDelay) {
    lastButtonTime = millis();
    return true;
  }
  return false;
}

void drawText(const char* msg) {
  if (lastText != msg || millis() - lastDisplayUpdate >= DISPLAY_REFRESH_INTERVAL) {
    dmd.clearScreen();
    dmd.drawString(1, 0, msg, GRAPHICS_NOR);
    lastText = msg;
    lastDisplayUpdate = millis();
  }
}

void setup() {
  Serial.begin(9600);
  Wire.begin();

  dmd.begin();
  dmd.setBrightness(100);
  dmd.clearScreen();
  dmd.selectFont(Arial_Black_16);
  drawText("WAIT GPS");

  if (!rtc.begin()) {
    drawText("RTC FAIL");
    while (1);
  }

  if (rtc.lostPower()) {
    rtc.adjust(DateTime(2025, 6, 17, 12, 0, 0));
  }

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);

  toggleTimer = millis();
  lastDisplayUpdate = millis();
}

void loop() {
  // Serial Input from ESP32
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '<') gpsBuffer = "";
    else if (c == '>') {
      gpsBuffer.trim();
      if (gpsBuffer.startsWith("TEMP:")) {
        receivedTemp = gpsBuffer.substring(5);
      } else if (gpsBuffer.indexOf(':') != -1 && gpsBuffer.length() >= 8) {
        receivedTime = gpsBuffer;
      }
      gpsBuffer = "";
    } else {
      gpsBuffer += c;
    }
  }

  switch (menuState) {
    case MENU_NONE:
      if (millis() - toggleTimer >= (displayToggleState == SHOW_TIME ? TIME_DISPLAY_DURATION : TEMP_DISPLAY_DURATION)) {
        toggleTimer = millis();
        if (tempDisplayEnabled) {
          displayToggleState = (displayToggleState == SHOW_TIME) ? SHOW_TEMP : SHOW_TIME;
        } else {
          displayToggleState = SHOW_TIME;
        }
      }

      if (displayToggleState == SHOW_TIME) {
        if (currentMode == GPS_MODE && receivedTime.length() >= 8) {
          receivedTime.toCharArray(buffer, sizeof(buffer));
          drawText(buffer);
        } else {
          DateTime now = rtc.now();
          sprintf(buffer, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
          drawText(buffer);
        }
      } else if (displayToggleState == SHOW_TEMP) {
        if (receivedTemp.length() > 0) {
          sprintf(buffer, "%sC", receivedTemp.c_str());
          drawText(buffer);
        } else {
          drawText("TMP:WAIT");
        }
      }

      if (buttonPressed(BTN_SELECT)) {
        menuState = MENU_MAIN;
        mainMenuIndex = 0;
        drawText(mainMenuItems[mainMenuIndex]);
      }
      break;

    case MENU_MAIN:
      drawText(mainMenuItems[mainMenuIndex]);
      if (buttonPressed(BTN_UP)) {
        mainMenuIndex = (mainMenuIndex + MAIN_MENU_ITEM_COUNT - 1) % MAIN_MENU_ITEM_COUNT;
      }
      if (buttonPressed(BTN_DOWN)) {
        mainMenuIndex = (mainMenuIndex + 1) % MAIN_MENU_ITEM_COUNT;
      }
      if (buttonPressed(BTN_SELECT)) {
        switch (mainMenuIndex) {
          case 0: menuState = MENU_RTC; break;
          case 1: menuState = MENU_GPS; break;
          case 2: menuState = MENU_TEMP_TOGGLE; tempToggleMenuIndex = tempDisplayEnabled ? 0 : 1; break;
        }
      }
      if (buttonPressed(BTN_BACK)) {
        menuState = MENU_NONE;
        toggleTimer = millis();
      }
      break;

    case MENU_RTC:
      drawText("SetTime");
      if (buttonPressed(BTN_SELECT)) {
        DateTime now = rtc.now();
        setHour = now.hour();
        setMin = now.minute();
        setSec = now.second();
        menuState = MENU_SET_HOUR;
      }
      if (buttonPressed(BTN_BACK)) {
        menuState = MENU_MAIN;
      }
      break;

    case MENU_GPS:
      drawText(currentMode == GPS_MODE ? "GPS ON" : "GPS OFF");
      if (buttonPressed(BTN_UP) || buttonPressed(BTN_DOWN)) {
        currentMode = (currentMode == GPS_MODE) ? RTC_MODE : GPS_MODE;
      }
      if (buttonPressed(BTN_SELECT) || buttonPressed(BTN_BACK)) {
        menuState = MENU_MAIN;
      }
      break;

    case MENU_TEMP_TOGGLE:
      drawText(tempToggleMenuItems[tempToggleMenuIndex]);
      if (buttonPressed(BTN_UP)) {
        tempToggleMenuIndex = (tempToggleMenuIndex + TEMP_TOGGLE_MENU_COUNT - 1) % TEMP_TOGGLE_MENU_COUNT;
      }
      if (buttonPressed(BTN_DOWN)) {
        tempToggleMenuIndex = (tempToggleMenuIndex + 1) % TEMP_TOGGLE_MENU_COUNT;
      }
      if (buttonPressed(BTN_SELECT)) {
        tempDisplayEnabled = (tempToggleMenuIndex == 0);
        menuState = MENU_MAIN;
      }
      if (buttonPressed(BTN_BACK)) {
        menuState = MENU_MAIN;
      }
      break;

    case MENU_SET_HOUR:
      sprintf(buffer, "Hr: %02d", setHour);
      drawText(buffer);
      if (buttonPressed(BTN_UP))   setHour = (setHour + 1) % 24;
      if (buttonPressed(BTN_DOWN)) setHour = (setHour + 23) % 24;
      if (buttonPressed(BTN_SELECT)) menuState = MENU_SET_MIN;
      if (buttonPressed(BTN_BACK))   menuState = MENU_RTC;
      break;

    case MENU_SET_MIN:
      sprintf(buffer, "Min: %02d", setMin);
      drawText(buffer);
      if (buttonPressed(BTN_UP))   setMin = (setMin + 1) % 60;
      if (buttonPressed(BTN_DOWN)) setMin = (setMin + 59) % 60;
      if (buttonPressed(BTN_SELECT)) menuState = MENU_SET_SEC;
      if (buttonPressed(BTN_BACK))   menuState = MENU_RTC;
      break;

    case MENU_SET_SEC:
      sprintf(buffer, "Sec: %02d", setSec);
      drawText(buffer);
      if (buttonPressed(BTN_UP))   setSec = (setSec + 1) % 60;
      if (buttonPressed(BTN_DOWN)) setSec = (setSec + 59) % 60;
      if (buttonPressed(BTN_SELECT)) {
        currentMode = RTC_MODE; // Switch to RTC mode after setting time
        rtc.adjust(DateTime(2025, 6, 17, setHour, setMin, setSec));
        menuState = MENU_NONE;
        toggleTimer = millis();
        // Force display update with new RTC time
        DateTime now = rtc.now();
        sprintf(buffer, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
        drawText(buffer);
      }
      if (buttonPressed(BTN_BACK)) menuState = MENU_RTC;
      break;
  }
}