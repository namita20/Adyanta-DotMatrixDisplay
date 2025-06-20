#include <SPI.h>
#include <DMD2.h>
#include <fonts/Arial14.h>

const int WIDTH = 4;  // Total panels = 4 (2x2), connected in snake/z-chain
const uint8_t *FONT = Arial14;

SoftDMD dmd(WIDTH, 1);  // 4 panels in one chain (64x32 total)

void setup() {
  Serial.begin(9600);
  dmd.begin();
  dmd.selectFont(FONT);
  dmd.setBrightness(255);
  dmd.clearScreen();

  // Print "Adyanta" on the first line (y=0)
  dmd.drawString(5, 0, "Adyanta");

  // Print "Semiconductor" on the second line (y=16)
  dmd.drawString(64, 0, "Semiconductor");
}

void loop() {
  // Nothing in loop
}
