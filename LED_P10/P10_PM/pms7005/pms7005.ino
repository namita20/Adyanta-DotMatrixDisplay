#include <SoftwareSerial.h>

#define PMS_RX_PIN 2
#define PMS_TX_PIN 3

SoftwareSerial serial(PMS_RX_PIN, PMS_TX_PIN);

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  serial.begin(9600);
  Serial.println("PMS7003 Initializing...");
  delay(10000);
}

void loop() {
  static uint8_t buffer[32];
  static uint8_t index = 0;

  while (serial.available()) {
    buffer[index] = serial.read();
    if (index == 0 && buffer[0] != 0x42) continue;
    if (index == 1 && buffer[1] != 0x4D) { index = 0; continue; }
    index++;
    if (index >= 32) {
      index = 0;
      // Calculate checksum
      uint16_t sum = 0;
      for (int i = 0; i < 30; i++) sum += buffer[i];
      uint16_t checksum = (buffer[30] << 8) + buffer[31];
      if (sum == checksum) {
        uint16_t pm1_0 = (buffer[4] << 8) + buffer[5];
        uint16_t pm2_5 = (buffer[6] << 8) + buffer[7];
        uint16_t pm10 = (buffer[8] << 8) + buffer[9];
        Serial.print("PM1.0: "); Serial.print(pm1_0);
        Serial.print("\tPM2.5: "); Serial.print(pm2_5);
        Serial.print("\tPM10: "); Serial.println(pm10);
        Serial.println("-----------------------------");
        
      } else {
        Serial.print("Checksum error! Calculated: "); Serial.print(sum, HEX);
        Serial.print(" Received: "); Serial.println(checksum, HEX);
      }
      // Print raw buffer
      Serial.print("Raw: ");
      for (int i = 0; i < 32; i++) {
        Serial.print(buffer[i], HEX); Serial.print(" ");
      }
      Serial.println();
    }
  }
  delay(1000); // Update every second
}