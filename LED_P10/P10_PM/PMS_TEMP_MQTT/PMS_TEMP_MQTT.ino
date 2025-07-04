#include <WiFi.h>
#include <PubSubClient.h>
#include <PMS.h>
#include <HardwareSerial.h>
#include <DHT.h>

// ————— Pin Definitions —————
#define PMS_RX       16    // PMS TX → ESP32 RX2
#define PMS_TX       17    // PMS RX ← ESP32 TX2
#define DHTPIN       27
#define DHTTYPE      DHT11

// ————— WiFi Credentials —————
const char* ssid     = "Adyanta";
const char* password = "adya4321";

// ————— MQTT Broker Credentials —————
const char* mqtt_server   = "58.84.63.220";
const uint16_t mqtt_port  = 5883;
const char* mqtt_user     = "groot";
const char* mqtt_password = "imgroot";
const char* mqtt_topic    = "pms/data";

// ————— Objects —————
// WiFiClientSecure espClient; // Use for secure MQTT (uncomment if TLS required)
WiFiClient espClient; // Use for non-secure MQTT
PubSubClient client(espClient);
HardwareSerial pmsSerial(2);  // Use UART2
PMS pms(pmsSerial);
PMS::DATA data;
DHT dht(DHTPIN, DHTTYPE);

// ————— Timing —————
unsigned long lastPMSTime = 0;
unsigned long lastDHTTime = 0;
const unsigned long PMS_INTERVAL = 600000;  // 10 minutes
const unsigned long DHT_INTERVAL = 900000;  // 15 minutes

// Connect to WiFi
void setup_wifi() {
  Serial.print(F("Connecting to WiFi..."));
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println(F("\n✅ WiFi connected"));
  Serial.print(F("IP: "));
  Serial.println(WiFi.localIP());
  Serial.print(F("RSSI: "));
  Serial.println(WiFi.RSSI());
}

// Reconnect to MQTT if disconnected
void reconnect() {
  while (!client.connected()) {
    // Ensure WiFi is connected
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println(F("WiFi disconnected, reconnecting..."));
      setup_wifi();
    }
    
    Serial.print(F("Connecting to MQTT..."));
    // Generate unique client ID using MAC address
    String clientId = "ESP32_PMS_" + String(WiFi.macAddress());
    
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
      Serial.println(F("✅ MQTT connected"));
    } else {
      Serial.print(F("❌ failed, rc="));
      Serial.print(client.state());
      Serial.print(F(" WiFi status: "));
      Serial.print(WiFi.status());
      Serial.println(F(" Retrying in 5 seconds..."));
      delay(5000); // Wait before retrying
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(10);

  // PMS7003 init
  pmsSerial.begin(9600, SERIAL_8N1, PMS_RX, PMS_TX);
  pms.passiveMode();
  pms.wakeUp();

  // DHT init
  dht.begin();

  // Network
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setKeepAlive(60); // Set keep-alive to 60 seconds
  
  // Uncomment for secure MQTT (if broker requires TLS)
  // espClient.setInsecure(); // For testing without CA certificate
  // espClient.setCACert(root_ca); // Use if you have the broker's CA certificate
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("WiFi disconnected, reconnecting..."));
    setup_wifi();
  }
  if (!client.connected()) {
    reconnect();
  }
  client.loop();


  unsigned long now = millis();

  // Send PMS7003 data every 10 minutes
  if (now - lastPMSTime >= PMS_INTERVAL) {
    lastPMSTime = now;
    pms.requestRead();
    if (pms.readUntil(data)) {
      Serial.println(F("📤 Sending PMS data via MQTT..."));
      
      // Use char buffer to avoid String memory issues
      char payload[256];
      snprintf(payload, sizeof(payload),
               "{\"SensorID\":\"esp32-pms7003_2\",\"UID\":\"002\",\"DID\":\"D002\","
               "\"SensorType\":\"PMS7003\",\"Val1\":%d,\"Val2\":%d,\"Val3\":%d,"
               "\"Val4\":null,\"Val5\":null,\"Val6\":null,\"Val7\":null,"
               "\"Val8\":null,\"Val9\":null,\"Val10\":null}",
               data.PM_AE_UG_1_0, data.PM_AE_UG_2_5, data.PM_AE_UG_10_0);
      
      if (client.publish(mqtt_topic, payload)) {
        Serial.print(F("Payload: "));
        Serial.println(payload);
      } else {
        Serial.println(F("⚠️ Failed to publish PMS data"));
      }
    } else {
      Serial.println(F("⚠️ No PMS data"));
    }
  }

  // Send DHT11 temperature every 15 minutes
  if (now - lastDHTTime >= DHT_INTERVAL) {
    lastDHTTime = now;
    float temp = dht.readTemperature();
    if (!isnan(temp)) {
      Serial.println(F("📤 Sending DHT data via MQTT..."));
      
      // Use char buffer for DHT payload
      char payload[128];
      snprintf(payload, sizeof(payload),
               "{\"SensorID\":\"esp32-dht11_2\",\"UID\":\"002\",\"DID\":\"D002\","
               "\"SensorType\":\"DHT11\",\"Val1\":%.1f}",
               temp);
      
      if (client.publish(mqtt_topic, payload)) {
        Serial.print(F("Payload: "));
        Serial.println(payload);
      } else {
        Serial.println(F("⚠️ Failed to publish DHT data"));
      }
    } else {
      Serial.println(F("⚠️ Failed to read DHT11"));
    }
  }
}