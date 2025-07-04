#include <WiFi.h>
#include <PubSubClient.h>
#include <PMS.h>
#include <HardwareSerial.h>

// PMS7003 UART pins
#define PMS_RX 16  // PMS TX → ESP32 RX2
#define PMS_TX 17  // PMS RX ← ESP32 TX2

// WiFi credentials
const char* ssid = "Adyanta";
const char* password = "adya4321";

// MQTT broker credentials
const char* mqtt_server = "58.84.63.220";
const uint16_t mqtt_port = 5883;
const char* mqtt_user = "groot";
const char* mqtt_password = "imgroot";
const char* mqtt_topic = "pms/data";

// MQTT client
WiFiClient espClient;
PubSubClient client(espClient);

// PMS7003 setup
HardwareSerial pmsSerial(2);  // Use UART2
PMS pms(pmsSerial);
PMS::DATA data;

// Connect to WiFi
void setup_wifi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected");
}

// Reconnect to MQTT if disconnected
void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    // With authentication
    if (client.connect("ESP32_PMS_Client", mqtt_user, mqtt_password)) {
      Serial.println("✅ MQTT connected");
    } else {
      Serial.print("❌ failed, rc=");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pmsSerial.begin(9600, SERIAL_8N1, PMS_RX, PMS_TX);
  pms.passiveMode();  // Manual reading mode
  pms.wakeUp();       // Wake up sensor

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Request and read PMS data
  pms.requestRead();
  if (pms.readUntil(data)) {
    Serial.println("📤 Sending PMS data via MQTT...");

String payload = String("{")
  + "\"SensorID\":\"esp32-pms7003\","
  + "\"UID\":\"001\","
  + "\"DID\":\"D001\","
  + "\"SensorType\":\"PMS7003\","
  + "\"Val1\":" + data.PM_AE_UG_1_0 + ","
  + "\"Val2\":" + data.PM_AE_UG_2_5 + ","
  + "\"Val3\":" + data.PM_AE_UG_10_0 + ","
  + "\"Val4\":null,"
  + "\"Val5\":null,"
  + "\"Val6\":null,"
  + "\"Val7\":null,"
  + "\"Val8\":null,"
  + "\"Val9\":null,"
  + "\"Val10\":null"
  + "}";


    client.publish(mqtt_topic, payload.c_str());
    Serial.println("Payload: " + payload);
  } else {
    Serial.println("⚠️ No PMS data");
  }

delay(600000);  // 600,000 milliseconds = 10 minutes
}
