#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

// --- Wi-Fi ---
const char* ssid = "Adyanta";
const char* password = "adya4321";

// --- MQTT ---
const char* mqttServer = "58.84.63.220";
const int mqttPort = 5883;
const char* mqttUser = "groot";
const char* mqttPassword = "imgroot";
const char* mqttTopic = "pms/data";

// --- DHT Sensor ---
#define DHTPIN 21
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// --- UART Setup ---
#define GPS_RX 16  // ESP32 RX2 ← GPS TX
#define GPS_TX 17  // ESP32 TX2 → GPS RX (not used)
#define ARDUINO_TX 15  // ESP32 TX1 → Arduino RX
#define ARDUINO_RX 4   // Not used

HardwareSerial gpsSerial(2);      // UART2 for GPS
HardwareSerial arduinoSerial(1);  // UART1 for Arduino

TinyGPSPlus gps;

WiFiClient espClient;
PubSubClient client(espClient);

// --- Wi-Fi Setup ---
void setup_wifi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected");
}

// --- MQTT Reconnect ---
void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("esp32-temp", mqttUser, mqttPassword)) {
      Serial.println("✅ MQTT connected");
    } else {
      Serial.print("❌ MQTT failed: ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

// --- Setup ---
void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  arduinoSerial.begin(9600, SERIAL_8N1, ARDUINO_TX, ARDUINO_RX);
  dht.begin();

  setup_wifi();
  client.setServer(mqttServer, mqttPort);

  Serial.println("🌡️ ESP32: DHT + GPS → Arduino + MQTT");
}

// --- TEMP Handling ---
static unsigned long lastTempToArduino = 0;
static unsigned long lastTempToMQTT = 0;
const unsigned long tempToArduinoInterval = 10 * 1000UL;       // 10 sec
const unsigned long tempToMQTTInterval   = 10 * 60 * 1000UL;   // 10 min

unsigned long lastGPSTimeSend = 0;
const unsigned long gpsInterval = 1000;  // 1 second

// --- Loop ---
void loop() {
  if (!client.connected()) reconnect_mqtt();
  client.loop();

  // --- GPS Handling ---
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  if (millis() - lastGPSTimeSend > gpsInterval && gps.time.isUpdated()) {
    lastGPSTimeSend = millis();

    if (gps.date.isValid() && gps.time.isValid()) {
      int h = gps.time.hour();
      int m = gps.time.minute();
      int s = gps.time.second();

      // IST conversion
      h += 5; m += 30;
      if (m >= 60) { m -= 60; h++; }
      if (h >= 24) h -= 24;

      char timeStr[16];
      sprintf(timeStr, "%02d:%02d:%02d", h, m, s);

      Serial.print("🛰️ GPS IST Time: ");
      Serial.println(timeStr);

      // Send time to Arduino
      arduinoSerial.print('<');
      arduinoSerial.print(timeStr);
      arduinoSerial.println('>');
    } else {
      Serial.println("❌ GPS not fixed yet.");
    }
  }

  // Read temp once and reuse
float temp = dht.readTemperature();

if (!isnan(temp)) {
  char tempStr[8];
  dtostrf(temp, 4, 1, tempStr);

  // Send to Arduino every 10 sec
  if (millis() - lastTempToArduino > tempToArduinoInterval) {
    lastTempToArduino = millis();
    arduinoSerial.print("<TEMP:");
    arduinoSerial.print(tempStr);
    arduinoSerial.println(">");
    Serial.println("✅ Temp Sent to Arduino: " + String(tempStr));
  }

  // Send to MQTT every 10 min
  if (millis() - lastTempToMQTT > tempToMQTTInterval) {
    lastTempToMQTT = millis();
    String payload = String("{")
      + "\"SensorID\":\"esp32-temp\","
      + "\"UID\":\"003\","
      + "\"DID\":\"D003\","
      + "\"SensorType\":\"DHT11\","
      + "\"Val1\":" + tempStr + ","
      + "\"Val2\":null,"
      + "\"Val3\":null,"
      + "\"Val4\":null,"
      + "\"Val5\":null,"
      + "\"Val6\":null,"
      + "\"Val7\":null,"
      + "\"Val8\":null,"
      + "\"Val9\":null,"
      + "\"Val10\":null"
      + "}";

    client.publish(mqttTopic, payload.c_str());
    Serial.println("📤 MQTT Published: " + payload);
  }

} else {
  Serial.println("❌ DHT read failed");
}
}
