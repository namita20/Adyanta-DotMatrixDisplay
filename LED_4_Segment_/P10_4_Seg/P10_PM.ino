#include <SoftwareSerial.h> // Revert to SoftwareSerial for pins 3 and 4
#include <SPI.h>
#include <DMD2.h>
#include <fonts/SystemFont5x7.h>
#include <fonts/Arial14.h>

const int WIDTH = 2; // 2x1 P10 display
const uint8_t *FONT = Arial14; // Use Arial14 for larger text

SoftDMD dmd(WIDTH, 1);  // DMD controls the entire display
DMD_TextBox box(dmd, 0, 0, WIDTH * 32, 16); // Text box for automatic scrolling

SoftwareSerial pmsSerial(3, 4); // RX = pin 3, TX = pin 4 for PMS7003

unsigned long lastReadTime = 0;
const unsigned long readInterval = 10000; // Read PM sensor every 10 seconds
unsigned long lastDebugTime = 0; // For loop debugging
unsigned long lastScrollTime = 0; // For controlling scroll speed
const unsigned long scrollInterval = 500; // 100ms for slower scrolling
String currentMsg = "PM2.5: N/A ug/m3";
bool displayInitialized = false; // Flag to control P10 display activation
bool messageUpdated = false; // Flag to track if the message needs to be rewritten

void setup() {
  Serial.begin(9600);
  pmsSerial.begin(9600);

  // Set PMS7003 to active mode
  Serial.println("Setting PMS7003 to active mode...");
  byte activeModeCmd[] = {0x42, 0x4D, 0xE1, 0x00, 0x01, 0x01, 0x71}; // Command to set active mode
  pmsSerial.write(activeModeCmd, sizeof(activeModeCmd));
  delay(100);

  Serial.println("Waiting 30s for PMS7003 to stabilize...");
  delay(30000);

  // Initial buffer flush to clear any garbage data
  Serial.println("Initial buffer flush for 5 seconds...");
  unsigned long flushStart = millis();
  while (millis() - flushStart < 5000) {
    while (pmsSerial.available()) {
      pmsSerial.read();
    }
    delay(1);
  }
}

void loop() {
  // Debug to confirm loop is running
  if (millis() - lastDebugTime >= 1000) {
    Serial.println("Loop running...");
    lastDebugTime = millis();
  }

  // Read PMS7003 data every 10 seconds
  if (millis() - lastReadTime >= readInterval) {
    Serial.println("Attempting to read PMS7003...");
    int pm25 = readPMS7003();

    if (pm25 >= 0) {
      currentMsg = "PM2.5: " + String(pm25) + "ug/m3";
      Serial.print("Live PM2.5 data from sensor: ");
      Serial.println(currentMsg);
      messageUpdated = true; // Flag to rewrite the message

      // Initialize P10 display only after the first valid reading
      if (!displayInitialized) {
        Serial.println("Initializing P10 Display after valid PM2.5 reading...");
        dmd.setBrightness(20);
        dmd.selectFont(FONT);
        dmd.begin();
        dmd.drawString(0, 0, "Starting...");
        delay(5000);
        dmd.clearScreen();
        displayInitialized = true;
      }
    } else {
      Serial.println("PM2.5: Error");
    }

    lastReadTime = millis();
    Serial.println("PM2.5 reading completed.");
  }

  // Scroll the message continuously if display is initialized
  if (displayInitialized && millis() - lastScrollTime >= scrollInterval) {
    static int charIndex = 0; // Track which character to write next
    static String displayedMsg = ""; // Track the message being displayed

    // If the message has been updated or we've scrolled all characters
    if (messageUpdated || charIndex >= displayedMsg.length()) {
      box.clear(); // Clear the text box
      charIndex = 0; // Reset character index
      displayedMsg = currentMsg + " "; // Add a space for separation
      messageUpdated = false; // Reset the update flag
    }

    // Write the next character to the text box
    if (charIndex < displayedMsg.length()) {
      box.write(displayedMsg[charIndex]);
      charIndex++;
    }

    lastScrollTime = millis();
  }
}

int readPMS7003() {
  byte buf[32];
  unsigned long start = millis();
  int idx = 0;
  int pm25 = -1;
  int maxRetries = 3; // Retry up to 3 times for a valid packet
  int retryCount = 0;

  while (retryCount < maxRetries) {
    // Buffer flush aligned with sensor's data interval (~2.5s)
    Serial.println("Buffer flush for 3 seconds to align with sensor data...");
    unsigned long flushStart = millis();
    while (millis() - flushStart < 3000) {
      while (pmsSerial.available()) {
        pmsSerial.read();
      }
      delay(1);
    }

    Serial.println("Waiting for PMS7003 data...");
    unsigned long lastByteTime = millis();
    idx = 0; // Reset index for each retry

    while (millis() - start < 10000) {
      if (pmsSerial.available()) {
        byte b = pmsSerial.read();
        Serial.print("Byte: 0x");
        Serial.println(b, HEX);
        lastByteTime = millis();

        if (idx == 0 && b != 0x42) continue;
        if (idx == 1 && b != 0x4D) {
          idx = 0;
          continue;
        }

        buf[idx++] = b;

        if (idx == 4 && (buf[2] != 0x00 || buf[3] != 0x1C)) {
          Serial.println("Invalid frame length");
          idx = 0;
          break; // Break inner loop to retry
        }

        if (idx == 32) {
          uint16_t sum = 0;
          for (int i = 0; i < 30; i++) sum += buf[i];
          uint16_t checksum = (buf[30] << 8) | buf[31];

          if (sum == checksum) {
            pm25 = (buf[10] << 8) | buf[11];
            Serial.print("Fresh packet received at millis: ");
            Serial.println(millis());
            return pm25; // Success, return the PM2.5 value
          } else {
            Serial.println("Checksum mismatch");
            break; // Break inner loop to retry
          }
        }
      }

      // Timeout if no new bytes received for 2 seconds
      if (millis() - lastByteTime > 2000) {
        Serial.println("Timeout: No new bytes received for 2 seconds.");
        break;
      }
    }

    if (idx != 32) {
      Serial.println("Incomplete packet");
    }
    retryCount++;
    Serial.print("Retry attempt: ");
    Serial.println(retryCount);
  }

  Serial.println("Failed to read valid packet after retries.");
  return -1; // Return -1 if all retries fail
}