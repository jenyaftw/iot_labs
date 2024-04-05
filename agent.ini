#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>
#include <TinyGPS++.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

#define RXD2 16
#define TXD2 17

#define AQI 34

Adafruit_MPU6050 mpu;

const char* ssid = ""; // WiFi SSID
const char* password = ""; // WiFi Password

const char* mqtt_host = "mqtt-dashboard.com";
const int mqtt_port = 1883;
const char* mqtt_topic = "agent_data_topic";
const char* mqtt_username = "";
const char* mqtt_password = "";

WiFiClient espClient;
PubSubClient client(espClient);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

TinyGPSPlus gps;

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600, SERIAL_8N1, TXD2, RXD2);

  delay(1000);

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }

  Serial.println("\nConnected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_host, mqtt_port);
  while (!client.connected()) {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Public emqx mqtt broker connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }

  timeClient.begin();
  timeClient.setTimeOffset(10800);
}

void loop() {
  client.loop();

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  int aqi = analogRead(AQI);

  bool recebido = false;
  while (Serial1.available()) {
    char cIn = Serial1.read();
    recebido = gps.encode(cIn);
  }

  JsonDocument doc;
  doc["user_id"] = 1;
  doc["gps"]["longitude"] = gps.location.lng();
  doc["gps"]["latitude"] = gps.location.lat();
  doc["accelerometer"]["x"] = a.acceleration.z;  // Because MPU-6050 is mounted vertically, axis needs to be
  doc["accelerometer"]["y"] = a.acceleration.y;
  doc["accelerometer"]["z"] = a.acceleration.x;
  doc["sensors"]["temperature"] = temp.temperature;
  doc["sensors"]["aqi"] = aqi;
  doc["timestamp"] = "2018-04-30T16:00:13Z";

  char buffer[256];
  size_t n = serializeJson(doc, buffer);
  client.publish(mqtt_topic, buffer, n);

  delay(500);
}
