#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RadioLib.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// WiFi credentials
const char *ssid = "";
const char *password = "";

// MQTT settings
const char *mqtt_server = "";
const int mqtt_port = 8883; // Changed to TLS port
const char *mqtt_user = "";
const char *mqtt_password = "";
const char *mqtt_client_id = "";

// MQTT topics
const char *mqtt_topic_data = "lora/sensor/data";
const char *mqtt_topic_status = "lora/sensor/status";
const char *mqtt_topic_rssi = "lora/sensor/rssi";

// WiFi and MQTT clients
WiFiClientSecure espClient; // Changed to secure client
PubSubClient mqtt(espClient);

// Root CA for HiveMQ Cloud (Let's Encrypt ISRG Root X1)
const char *root_ca = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

// Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET 21
#define SCREEN_ADDRESS 0x3C

// I2C Pins for OLED
#define OLED_SDA 17
#define OLED_SCL 18

// Power control
#define VEXT_PIN 36

// LoRa pins for Heltec V3
#define LORA_SCK 9
#define LORA_MISO 11
#define LORA_MOSI 10
#define LORA_CS 8
#define LORA_DIO1 14
#define LORA_RST 12
#define LORA_BUSY 13

// LoRa settings (must match transmitter EXACTLY!)
#define FREQUENCY 868.0     // 868 MHz for Europe (unchanged)
#define BANDWIDTH 250.0     // kHz — updated to match LongFast preset
#define SPREADING_FACTOR 11 // updated from 9 to 11
#define CODING_RATE 5       // Meshtastic uses 4/5, which corresponds to 5 here
#define TRANSMIT_POWER 20   // dBm (unchanged)
#define PREAMBLE_LENGTH 8   // (unchanged)
#define SYNC_WORD 0x12      // MUST match transmitter! (unchanged)

// Create display on Wire
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// LoRa radio module
SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY);

// Structure for LoRa data packet (must match transmitter)
struct DataPacket
{
  uint32_t counter;
  float latitude;
  float longitude;
  uint8_t satellites;
  float ax_ms2;
  float ay_ms2;
  float az_ms2;
  uint16_t airQuality;
  float battery;
  uint8_t gpsValid;
} __attribute__((packed));

// Current received data
DataPacket currentData = {0};
bool dataReceived = false;
unsigned long lastReceiveTime = 0;
int rssi = 0;
float snr = 0;

// Connection status
bool wifiConnected = false;
bool mqttConnected = false;
unsigned long lastReconnectAttempt = 0;
unsigned long lastMqttPublish = 0;

// Flag for interrupt
volatile bool receivedFlag = false;

// Interrupt service routine
void setFlag(void)
{
  receivedFlag = true;
}

// Function declarations
void setupWiFi();
void setupTLS();
void reconnectMQTT();
void publishData();
void updateDisplay();

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("Starting Heltec V3 LoRa Receiver...");

  // Power on external components
  pinMode(VEXT_PIN, OUTPUT);
  digitalWrite(VEXT_PIN, LOW); // Turn on Vext
  delay(500);

  // Initialize Wire for OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(400000);

  // Reset OLED if needed
  if (OLED_RESET != -1)
  {
    pinMode(OLED_RESET, OUTPUT);
    digitalWrite(OLED_RESET, LOW);
    delay(10);
    digitalWrite(OLED_RESET, HIGH);
    delay(10);
  }

  // Initialize display
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println("SSD1306 allocation failed");
  }
  else
  {
    Serial.println("Display initialized");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("LoRa Receiver");
    display.println("Initializing...");
    display.display();
  }

  // Initialize SPI for LoRa
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);

  // Initialize LoRa
  Serial.print("Initializing LoRa... ");
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Init LoRa...");
  display.display();

  // Print all parameters for verification
  Serial.println("\nLoRa Parameters:");
  Serial.print("Frequency: ");
  Serial.print(FREQUENCY);
  Serial.println(" MHz");
  Serial.print("Bandwidth: ");
  Serial.print(BANDWIDTH);
  Serial.println(" kHz");
  Serial.print("Spreading Factor: ");
  Serial.println(SPREADING_FACTOR);
  Serial.print("Coding Rate: 4/");
  Serial.println(CODING_RATE);
  Serial.print("Sync Word: 0x");
  Serial.println(SYNC_WORD, HEX);
  Serial.print("Preamble Length: ");
  Serial.println(PREAMBLE_LENGTH);

  int state = radio.begin(FREQUENCY, BANDWIDTH, SPREADING_FACTOR, CODING_RATE, SYNC_WORD, TRANSMIT_POWER, PREAMBLE_LENGTH);

  if (state == RADIOLIB_ERR_NONE)
  {
    Serial.println("LoRa initialized!");
    display.println("LoRa OK!");

    // Print packet size for verification
    Serial.print("Expected packet size: ");
    Serial.print(sizeof(DataPacket));
    Serial.println(" bytes");

    // Add a test to receive raw packets
    Serial.println("\nTrying raw packet reception test...");
    uint8_t rawData[255];
    int rawState = radio.receive(rawData, 255);
    if (rawState == RADIOLIB_ERR_NONE)
    {
      Serial.print("Raw test received ");
      Serial.print(radio.getPacketLength());
      Serial.println(" bytes!");
    }
    else
    {
      Serial.println("Raw test: no packet");
    }

    // Set the radio to fixed packet length mode
    radio.fixedPacketLengthMode(sizeof(DataPacket));

    // Set up interrupt on DIO1
    radio.setDio1Action(setFlag);

    // Start receiving
    state = radio.startReceive();
    if (state == RADIOLIB_ERR_NONE)
    {
      Serial.println("Started receiving with interrupt");
      display.println("Receiving...");
    }
    else
    {
      Serial.print("Start receive failed, code ");
      Serial.println(state);
      display.println("Receive failed!");
    }
  }
  else
  {
    Serial.print("LoRa init failed, code ");
    Serial.println(state);
    display.println("LoRa FAIL!");
  }

  display.display();
  delay(2000);

  // Clear display for normal operation
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Waiting for data...");
  display.display();

  Serial.println("Setup complete! Waiting for packets...");

  // Initialize WiFi
  setupWiFi();

  // Configure MQTT with larger buffer
  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setSocketTimeout(30); // 30 seconds timeout
  mqtt.setBufferSize(1024);  // Increase MQTT buffer size
  mqtt.setKeepAlive(60);     // 60 seconds keepalive
}

void loop()
{
  // Check if packet was received via interrupt
  if (receivedFlag)
  {
    receivedFlag = false;
    Serial.println("Interrupt triggered - packet detected!");

    DataPacket receivedPacket;
    int state = radio.readData((uint8_t *)&receivedPacket, sizeof(DataPacket));

    if (state == RADIOLIB_ERR_NONE)
    {
      // Packet received successfully
      currentData = receivedPacket;
      dataReceived = true;
      lastReceiveTime = millis();

      // Get RSSI and SNR
      rssi = radio.getRSSI();
      snr = radio.getSNR();

      // Print received data to Serial
      Serial.println();
      Serial.print("Received packet #");
      Serial.println(receivedPacket.counter);
      Serial.print("RSSI: ");
      Serial.print(rssi);
      Serial.print(" dBm, SNR: ");
      Serial.print(snr);
      Serial.println(" dB");

      Serial.print("  GPS: ");
      if (receivedPacket.gpsValid)
      {
        Serial.print(receivedPacket.latitude, 6);
        Serial.print(", ");
        Serial.print(receivedPacket.longitude, 6);
        Serial.print(" (");
        Serial.print(receivedPacket.satellites);
        Serial.print(" sats)");
      }
      else
      {
        Serial.print("No fix");
      }
      Serial.println();

      Serial.print("  Accel: X=");
      Serial.print(receivedPacket.ax_ms2, 2);
      Serial.print(" Y=");
      Serial.print(receivedPacket.ay_ms2, 2);
      Serial.print(" Z=");
      Serial.print(receivedPacket.az_ms2, 2);
      Serial.println(" m/s²");

      Serial.print("  Air Quality: ");
      Serial.print(receivedPacket.airQuality);
      Serial.print(", Battery: ");
      Serial.print(receivedPacket.battery, 2);
      Serial.println("V");

      // Publish to MQTT
      if (mqttConnected)
      {
        publishData();
      }
    }
    else if (state == RADIOLIB_ERR_CRC_MISMATCH)
    {
      Serial.println("CRC error!");
    }
    else
    {
      Serial.print("Read failed, code ");
      Serial.println(state);
    }

    // Start receiving again
    radio.startReceive();
  }

  // Also check with polling method
  if (radio.available())
  {
    Serial.println("Packet available via polling!");
    DataPacket receivedPacket;
    int state = radio.readData((uint8_t *)&receivedPacket, sizeof(DataPacket));

    if (state == RADIOLIB_ERR_NONE)
    {
      // Process packet (same as above)
      currentData = receivedPacket;
      dataReceived = true;
      lastReceiveTime = millis();
      rssi = radio.getRSSI();
      snr = radio.getSNR();
      Serial.println("Packet received via polling method");

      // Publish to MQTT
      if (mqttConnected)
      {
        publishData();
      }
    }

    radio.startReceive();
  }

  // Check for LoRa errors periodically
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 5000)
  {
    lastCheck = millis();
    Serial.println("Still listening...");
  }

  // Update display
  updateDisplay();

  // Handle WiFi and MQTT connections
  if (WiFi.status() != WL_CONNECTED)
  {
    wifiConnected = false;
    if (millis() - lastReconnectAttempt > 30000)
    { // Try every 30 seconds
      lastReconnectAttempt = millis();
      setupWiFi();
    }
  }
  else
  {
    wifiConnected = true;
    if (!mqtt.connected())
    {
      mqttConnected = false;
      if (millis() - lastReconnectAttempt > 5000)
      { // Try every 5 seconds
        lastReconnectAttempt = millis();
        reconnectMQTT();
      }
    }
    else
    {
      mqttConnected = true;
      mqtt.loop(); // Process MQTT messages
    }
  }

  // Add MQTT connection health check
  static unsigned long lastMqttCheck = 0;
  if (mqttConnected && millis() - lastMqttCheck > 30000)
  { // Every 30 seconds
    lastMqttCheck = millis();
    if (!mqtt.loop())
    { // mqtt.loop() returns false if disconnected
      Serial.println("MQTT connection lost during loop");
      mqttConnected = false;
    }
  }

  // Small delay
  delay(10);
}

void updateDisplay()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (!dataReceived)
  {
    display.setCursor(0, 20);
    display.println("Waiting for data...");
    display.setCursor(0, 35);
    display.println("No packets received");
    display.display();
    return;
  }

  // Show connection status based on last receive time
  bool isConnected = (millis() - lastReceiveTime) < 10000; // Consider disconnected after 10 seconds

  // Line 1: Air quality, battery, and packet counter
  display.setCursor(0, 0);
  display.print("AQ:");
  display.print(currentData.airQuality);
  display.print(" BAT:");
  display.print(currentData.battery, 1);
  display.print("V #");
  display.print(currentData.counter);

  // Connection indicators
  if (!isConnected)
  {
    display.print(" X");
  }
  if (wifiConnected)
  {
    display.print(" W");
  }
  if (mqttConnected)
  {
    display.print(" M");
  }

  // GPS info
  display.setCursor(0, 10);
  if (currentData.gpsValid)
  {
    display.print("GPS: ");
    display.print(currentData.latitude, 5);
    display.print(",");
    display.print(currentData.longitude, 5);
  }
  else
  {
    display.print("NO GPS LOCK");
  }

  // Satellites and signal info
  display.setCursor(0, 20);
  display.print("Sats:");
  display.print(currentData.satellites);
  display.print(" RSSI:");
  display.print(rssi);
  display.print("dBm");

  // MPU6050 data
  display.setCursor(0, 30);
  display.print("AX: ");
  display.print(currentData.ax_ms2, 1);
  display.print(" m/s2");

  display.setCursor(0, 40);
  display.print("AY: ");
  display.print(currentData.ay_ms2, 1);
  display.print(" m/s2");

  display.setCursor(0, 50);
  display.print("AZ: ");
  display.print(currentData.az_ms2, 1);
  display.print(" m/s2");

  display.display();
}

void setupTLS()
{
  espClient.setCACert(root_ca);
  // Optional: Skip server verification (less secure but sometimes necessary)
  // espClient.setInsecure();
}

void setupWiFi()
{
  Serial.println("Connecting to WiFi...");
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Connecting WiFi...");
  display.println(ssid);
  display.display();

  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20)
  {
    delay(500);
    Serial.print(".");
    display.print(".");
    display.display();
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\nWiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Setup TLS after WiFi is connected
    setupTLS();

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("WiFi connected!");
    display.println(WiFi.localIP());
    display.display();
    delay(2000);
  }
  else
  {
    Serial.println("\nWiFi connection failed");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("WiFi failed!");
    display.display();
    delay(2000);
  }
}

void reconnectMQTT()
{
  Serial.print("Attempting MQTT connection...");
  Serial.print(" Server: ");
  Serial.print(mqtt_server);
  Serial.print(":");
  Serial.println(mqtt_port);

  if (mqtt.connect(mqtt_client_id, mqtt_user, mqtt_password))
  {
    Serial.println("MQTT connected!");

    // Publish online status
    mqtt.publish(mqtt_topic_status, "online", true);

    // Publish device info
    StaticJsonDocument<200> doc;
    doc["device"] = mqtt_client_id;
    doc["ip"] = WiFi.localIP().toString();
    doc["rssi_wifi"] = WiFi.RSSI();

    char buffer[256];
    serializeJson(doc, buffer);
    mqtt.publish(mqtt_topic_status, buffer);
  }
  else
  {
    Serial.print("MQTT connection failed, rc=");
    Serial.print(mqtt.state());
    Serial.println();

    // Detailed error messages
    switch (mqtt.state())
    {
    case -4:
      Serial.println("MQTT_CONNECTION_TIMEOUT");
      break;
    case -3:
      Serial.println("MQTT_CONNECTION_LOST");
      break;
    case -2:
      Serial.println("MQTT_CONNECT_FAILED");
      break;
    case -1:
      Serial.println("MQTT_DISCONNECTED");
      break;
    case 1:
      Serial.println("MQTT_CONNECT_BAD_PROTOCOL");
      break;
    case 2:
      Serial.println("MQTT_CONNECT_BAD_CLIENT_ID");
      break;
    case 3:
      Serial.println("MQTT_CONNECT_UNAVAILABLE");
      break;
    case 4:
      Serial.println("MQTT_CONNECT_BAD_CREDENTIALS");
      break;
    case 5:
      Serial.println("MQTT_CONNECT_UNAUTHORIZED");
      break;
    default:
      Serial.println("Unknown error");
    }
  }
}

void publishData()
{
  // Create JSON document with larger size
  StaticJsonDocument<768> doc; // Increased size

  // Add packet info
  doc["packet_id"] = currentData.counter;
  doc["timestamp"] = millis();

  // Add GPS data
  JsonObject gps = doc.createNestedObject("gps");
  gps["valid"] = (bool)currentData.gpsValid;
  gps["latitude"] = currentData.latitude;
  gps["longitude"] = currentData.longitude;
  gps["satellites"] = currentData.satellites;

  // Add accelerometer data
  JsonObject accel = doc.createNestedObject("accelerometer");
  accel["x"] = currentData.ax_ms2;
  accel["y"] = currentData.ay_ms2;
  accel["z"] = currentData.az_ms2;
  accel["unit"] = "m/s²";

  // Add sensor data
  doc["air_quality"] = currentData.airQuality;
  doc["battery"] = currentData.battery;

  // Add radio info
  JsonObject radio = doc.createNestedObject("radio");
  radio["rssi"] = rssi;
  radio["snr"] = snr;

  // Serialize and publish with larger buffer
  char buffer[768]; // Increased buffer size
  size_t len = serializeJson(doc, buffer);

  // Check if serialization was successful
  if (len == 0)
  {
    Serial.println("Failed to serialize JSON");
    return;
  }

  Serial.print("JSON size: ");
  Serial.print(len);
  Serial.println(" bytes");

  // Check MQTT connection before publishing
  if (!mqtt.connected())
  {
    Serial.println("MQTT not connected, skipping publish");
    return;
  }

  // Publish with error checking
  if (mqtt.publish(mqtt_topic_data, buffer, false))
  { // false = not retained
    Serial.println("Data published to MQTT");
    lastMqttPublish = millis();
  }
  else
  {
    Serial.println("Failed to publish data");
    Serial.print("MQTT state: ");
    Serial.println(mqtt.state());

    // Check if message was too large
    if (strlen(buffer) > mqtt.getBufferSize())
    {
      Serial.print("Message too large! Size: ");
      Serial.print(strlen(buffer));
      Serial.print(" Max: ");
      Serial.println(mqtt.getBufferSize());
    }

    // Force reconnect if publish fails
    mqttConnected = false;
  }

  // Publish RSSI separately for easy monitoring
  char rssiBuffer[32];
  snprintf(rssiBuffer, sizeof(rssiBuffer), "%d", rssi);
  if (!mqtt.publish(mqtt_topic_rssi, rssiBuffer))
  {
    Serial.println("Failed to publish RSSI");
    // Force reconnect if even small message fails
    mqttConnected = false;
  }
}
