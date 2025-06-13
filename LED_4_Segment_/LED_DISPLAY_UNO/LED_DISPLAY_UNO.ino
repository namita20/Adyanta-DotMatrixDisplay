#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 8
#define CLK_PIN 13
#define DATA_PIN 11
#define CS_PIN 3

MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
MD_Parola parola = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

char displayText[50] = "AIR QUALITY DISPLAY";

void setup() {
  Serial.begin(9600);
  parola.begin();
  parola.setIntensity(8);
  parola.displayText(displayText, PA_CENTER, 100, 0, PA_SCROLL_LEFT, PA_NO_EFFECT);
}

void loop() {
  if (parola.displayAnimate()) {
    parola.displayReset();
  }
}