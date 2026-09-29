/*
  IoT Mini Project: Smart Temperature & Humidity Monitor
  Board  : ESP32 (Wokwi simulator)
  Sensor : DHT22
  Output : Green LED = normal, Red LED = high temperature alert
  Cloud  : Sends data over WiFi using MQTT (public HiveMQ broker)
*/

#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// ---------- Pins ----------
#define DHT_PIN     15
#define DHT_TYPE    DHT22
#define GREEN_LED   4
#define RED_LED     2

// ---------- WiFi (Wokwi's built-in virtual WiFi) ----------
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";

// ---------- MQTT ----------
const char* MQTT_SERVER = "broker.hivemq.com";
const int   MQTT_PORT   = 1883;
// Change "demo123" to something unique (e.g. your enrollment no.) so
// other people's data does not mix with yours on the public broker.
const char* TOPIC_DATA  = "iot_mini_project/demo123/data";
const char* TOPIC_ALERT = "iot_mini_project/demo123/alert";

// ---------- Settings ----------
const float TEMP_LIMIT = 35.0;   // alert above this temperature (deg C)

DHT dht(DHT_PIN, DHT_TYPE);
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS, 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println(" connected!");
}

void connectMQTT() {
  while (!mqtt.connected()) {
    Serial.print("Connecting to MQTT broker...");
    String clientId = "esp32-" + String(random(0xffff), HEX);
    if (mqtt.connect(clientId.c_str())) {
      Serial.println(" connected!");
    } else {
      Serial.print(" failed, rc=");
      Serial.println(mqtt.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  dht.begin();
  connectWiFi();
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();

  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();

  if (isnan(temp) || isnan(hum)) {
    Serial.println("Failed to read from DHT22 sensor!");
    delay(2000);
    return;
  }

  bool alert = temp > TEMP_LIMIT;
  digitalWrite(RED_LED, alert ? HIGH : LOW);
  digitalWrite(GREEN_LED, alert ? LOW : HIGH);

  char payload[64];
  snprintf(payload, sizeof(payload), "{\"temp\":%.1f,\"hum\":%.1f}", temp, hum);
  mqtt.publish(TOPIC_DATA, payload);
  if (alert) mqtt.publish(TOPIC_ALERT, "HIGH TEMPERATURE");

  Serial.print("Published: ");
  Serial.print(payload);
  Serial.println(alert ? "  [ALERT]" : "  [OK]");

  delay(3000);
}
