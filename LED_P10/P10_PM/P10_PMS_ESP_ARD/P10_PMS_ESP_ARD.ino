#define PMS_TX 22  // ESP32 TX1 → PMS RX
#define PMS_RX 21  // ESP32 RX1 ← PMS TX
#define ARDUINO_TX 17  // ESP32 TX2 → Arduino RX (Pin 2)
#define ARDUINO_RX 16  // ESP32 RX2 ← Arduino TX (Pin 3)

HardwareSerial pmsSerial(1);       // UART1 for PMS7003
HardwareSerial arduinoSerial(2);   // UART2 for Arduino

int pm25 = 0, pm10 = 0;

void setup() {
  Serial.begin(9600);
  pmsSerial.begin(9600, SERIAL_8N1, PMS_RX, PMS_TX);       // PMS
  arduinoSerial.begin(9600, SERIAL_8N1, ARDUINO_RX, ARDUINO_TX);  // Arduino

  Serial.println("PMS7003 Initializing...");
  pmsSerial.flush();
  delay(10000); // Wait for sensor to stabilize
}

bool readPMS7003() {
  static uint8_t buffer[32];
  static uint8_t index = 0;

  while (pmsSerial.available()) {
    buffer[index] = pmsSerial.read();

    if (index == 0 && buffer[0] != 0x42) continue;
    if (index == 1 && buffer[1] != 0x4D) { index = 0; continue; }

    index++;
    if (index >= 32) {
      index = 0;

      // Checksum validation
      uint16_t sum = 0;
      for (int i = 0; i < 30; i++) sum += buffer[i];
      uint16_t checksum = (buffer[30] << 8) | buffer[31];

      if (sum == checksum) {
        int pm1_0 = (buffer[10] << 8) | buffer[11];
        pm25 = (buffer[12] << 8) | buffer[13];
        pm10 = (buffer[14] << 8) | buffer[15];

        Serial.print("Raw PM1.0: "); Serial.print(pm1_0);
        Serial.print(" | PM2.5: "); Serial.print(pm25);
        Serial.print(" | PM10: "); Serial.println(pm10);
        return true;
      } else {
        Serial.println("Checksum error! Sum: " + String(sum) + ", Expected: " + String(checksum));
        return false;
      }
    }
  }
  return false;
}

void loop() {
  String msg;
  if (readPMS7003()) {
    msg = "AIR QUALITY|PM2.5: " + String(pm25) + "|PM10: " + String(pm10);
  } else {
    msg = "AIR QUALITY|PM2.5: Error|PM10: Error";
  }
  Serial.println("Sending: " + msg);
  arduinoSerial.println(msg);  // Send to Arduino
  delay(6000); // Send every 6 seconds
}