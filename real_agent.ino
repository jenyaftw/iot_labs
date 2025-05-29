#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TinyGPS++.h>
#include "MPU6050.h"
#include <RadioLib.h>

// Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET 21
#define SCREEN_ADDRESS 0x3C

// I2C Pins
#define OLED_SDA 17
#define OLED_SCL 18
#define MPU_SDA 41
#define MPU_SCL 42

// Other pins
#define AIR_QUALITY_PIN 19
#define RX_PIN 48
#define TX_PIN 47
#define GPS_BAUD 9600
#define VEXT_PIN 36

// LoRa pins for Heltec V3
#define LORA_SCK 9
#define LORA_MISO 11
#define LORA_MOSI 10
#define LORA_CS 8
#define LORA_DIO1 14
#define LORA_RST 12
#define LORA_BUSY 13

// LoRa settings
#define FREQUENCY 868.0     // 868 MHz for Europe (unchanged)
#define BANDWIDTH 250.0     // kHz — updated to match LongFast preset
#define SPREADING_FACTOR 11 // updated from 9 to 11
#define CODING_RATE 5       // Meshtastic uses 4/5, which corresponds to 5 here
#define TRANSMIT_POWER 20   // dBm (unchanged)
#define PREAMBLE_LENGTH 8   // (unchanged)
#define SYNC_WORD 0x12      // MUST match transmitter! (unchanged)

// Create display on Wire (default I2C)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Create MPU6050 on Wire1 (secondary I2C)
MPU6050 mpu(0x68, &Wire1);

// GPS
TinyGPSPlus gps;

// LoRa radio module
SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY);

// Accelerometer variables
int16_t ax, ay, az;
int16_t gx, gy, gz;

// Calibration offsets
int16_t ax_offset = 0;
int16_t ay_offset = 0;
int16_t az_offset = 0;

// Conversion factor: MPU6050 default is ±2g range
const float ACCEL_SCALE = 9.81 / 16384.0;

// Transmission interval (milliseconds)
unsigned long lastTransmitTime = 0;
const unsigned long TRANSMIT_INTERVAL = 2000; // Send every 5 seconds

// Packet counter
uint32_t packetCounter = 0;

#define gpsSerial Serial1

// Structure for LoRa data packet
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

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("Starting Heltec V3 with LoRa, dual I2C...");

  // Power on external sensors
  pinMode(VEXT_PIN, OUTPUT);
  digitalWrite(VEXT_PIN, LOW); // Turn on Vext
  delay(500);

  // Initialize Wire for OLED (pins 17/18)
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
    display.println("Initializing...");
    display.display();
  }

  // Initialize Wire1 for MPU6050 (pins 41/42)
  Wire1.begin(MPU_SDA, MPU_SCL);
  Wire1.setClock(400000);
  delay(100);

  // Initialize MPU6050
  mpu.initialize();
  delay(100);

  bool mpu_connected = mpu.testConnection();
  Serial.println(mpu_connected ? "MPU6050 OK" : "MPU6050 FAIL");

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
  Serial.print("Transmit Power: ");
  Serial.print(TRANSMIT_POWER);
  Serial.println(" dBm");

  int state = radio.begin(FREQUENCY, BANDWIDTH, SPREADING_FACTOR, CODING_RATE, SYNC_WORD, TRANSMIT_POWER, PREAMBLE_LENGTH);

  if (state == RADIOLIB_ERR_NONE)
  {
    Serial.println("LoRa initialized!");
    display.println("LoRa OK!");

    // Print packet size for verification
    Serial.print("Packet size: ");
    Serial.print(sizeof(DataPacket));
    Serial.println(" bytes");

    // Set explicit header mode (default, but let's be sure)
    radio.explicitHeader();

    // Optional: Set CRC on
    radio.setCRC(true);
  }
  else
  {
    Serial.print("LoRa init failed, code ");
    Serial.println(state);
    display.println("LoRa FAIL!");
  }
  display.display();
  delay(1000);

  // Show status on display
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("System Status:");
  display.setCursor(0, 10);
  display.print("MPU6050: ");
  display.println(mpu_connected ? "OK" : "FAIL");
  display.setCursor(0, 20);
  display.print("LoRa: ");
  display.println(state == RADIOLIB_ERR_NONE ? "OK" : "FAIL");
  display.display();
  delay(1000);

  // Initialize GPS
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
  Serial.println("GPS initialized");

  // Calibrate accelerometer
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Calibrating...");
  display.println("Keep device still!");
  display.display();

  calibrateAccelerometer();

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Calibration done!");
  display.display();
  delay(1000);

  Serial.println("Setup complete!");
}

void loop()
{
  // Read MPU6050 from Wire1
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // Process GPS
  unsigned long startTime = millis();
  while (gpsSerial.available() > 0 && millis() - startTime < 20)
  {
    if (gps.encode(gpsSerial.read()))
    {
      continue;
    }
  }

  // Read air quality sensor
  int adc = analogRead(AIR_QUALITY_PIN);
  float v_adc = adc * (3.3 / 4095.0);
  float v_sensor = v_adc * ((100000.0 + 220000.0) / 220000.0);
  int aq = (v_sensor / 5.0) * 4095.0;

  // Battery voltage (you can add heltec battery reading here)
  float vbat = 3.7; // Placeholder - add actual battery reading

  // Calculate acceleration in m/s²
  float ax_ms2 = (ax - ax_offset) * ACCEL_SCALE;
  float ay_ms2 = (ay - ay_offset) * ACCEL_SCALE;
  float az_ms2 = (az - az_offset) * ACCEL_SCALE;

  // Update display
  updateDisplay(aq, vbat, ax_ms2, ay_ms2, az_ms2);

  // Send LoRa packet at intervals
  if (millis() - lastTransmitTime >= TRANSMIT_INTERVAL)
  {
    lastTransmitTime = millis();
    sendLoRaPacket(ax_ms2, ay_ms2, az_ms2, aq, vbat);
  }

  // Debug output
  Serial.print("Accel (m/s²) - X: ");
  Serial.print(ax_ms2, 2);
  Serial.print(" Y: ");
  Serial.print(ay_ms2, 2);
  Serial.print(" Z: ");
  Serial.println(az_ms2, 2);

  delay(100);
}

void updateDisplay(int aq, float vbat, float ax_ms2, float ay_ms2, float az_ms2)
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Line 1: Air quality and battery
  display.setCursor(0, 0);
  display.print("AQ:");
  display.print(aq);
  display.print(" BAT:");
  display.print(vbat, 1);
  display.print("V #");
  display.print(packetCounter);

  // GPS info
  display.setCursor(0, 10);
  if (gps.location.isValid())
  {
    display.print("GPS: ");
    display.print(gps.location.lat(), 5);
    display.print(",");
    display.print(gps.location.lng(), 5);
  }
  else
  {
    display.print("NO GPS LOCK");

    // Check if GPS is connected
    if (millis() > 5000 && gps.charsProcessed() < 10)
    {
      display.print(" (NC)");
    }
  }

  // Satellites
  display.setCursor(0, 20);
  display.print("Sats: ");
  display.print(gps.satellites.value());

  // MPU6050 data
  display.setCursor(0, 30);
  display.print("AX: ");
  display.print(ax_ms2, 1);
  display.print(" m/s2");

  display.setCursor(0, 40);
  display.print("AY: ");
  display.print(ay_ms2, 1);
  display.print(" m/s2");

  display.setCursor(0, 50);
  display.print("AZ: ");
  display.print(az_ms2, 1);
  display.print(" m/s2");

  display.display();
}

void sendLoRaPacket(float ax_ms2, float ay_ms2, float az_ms2, int aq, float vbat)
{
  DataPacket packet;

  // Fill packet data
  packet.counter = packetCounter++;
  packet.latitude = gps.location.isValid() ? gps.location.lat() : 0.0;
  packet.longitude = gps.location.isValid() ? gps.location.lng() : 0.0;
  packet.satellites = gps.satellites.value();
  packet.ax_ms2 = ax_ms2;
  packet.ay_ms2 = ay_ms2;
  packet.az_ms2 = az_ms2;
  packet.airQuality = aq;
  packet.battery = vbat;
  packet.gpsValid = gps.location.isValid() ? 1 : 0;

  // Send packet
  Serial.print("Sending packet #");
  Serial.print(packet.counter);
  Serial.print(" (");
  Serial.print(sizeof(DataPacket));
  Serial.print(" bytes) ... ");

  // For debugging: print first few bytes
  uint8_t *dataPtr = (uint8_t *)&packet;
  Serial.print("First bytes: ");
  for (int i = 0; i < 8; i++)
  {
    Serial.print(dataPtr[i], HEX);
    Serial.print(" ");
  }
  Serial.print("... ");

  int state = radio.transmit((uint8_t *)&packet, sizeof(DataPacket));

  if (state == RADIOLIB_ERR_NONE)
  {
    Serial.println("success!");

    // Get and print datarate info
    Serial.print("  Datarate: ");
    Serial.print(radio.getDataRate());
    Serial.println(" bps");

    // Print transmitted data
    Serial.print("  GPS: ");
    if (packet.gpsValid)
    {
      Serial.print(packet.latitude, 6);
      Serial.print(", ");
      Serial.print(packet.longitude, 6);
      Serial.print(" (");
      Serial.print(packet.satellites);
      Serial.print(" sats)");
    }
    else
    {
      Serial.print("No fix");
    }
    Serial.println();

    Serial.print("  Accel: X=");
    Serial.print(packet.ax_ms2, 2);
    Serial.print(" Y=");
    Serial.print(packet.ay_ms2, 2);
    Serial.print(" Z=");
    Serial.print(packet.az_ms2, 2);
    Serial.println(" m/s²");

    Serial.print("  Air Quality: ");
    Serial.print(packet.airQuality);
    Serial.print(", Battery: ");
    Serial.print(packet.battery, 2);
    Serial.println("V");
  }
  else if (state == RADIOLIB_ERR_PACKET_TOO_LONG)
  {
    Serial.println("too long!");
  }
  else if (state == RADIOLIB_ERR_TX_TIMEOUT)
  {
    Serial.println("timeout!");
  }
  else
  {
    Serial.print("failed, code ");
    Serial.println(state);
  }
}

// Calibration function
void calibrateAccelerometer()
{
  Serial.println("Calibrating accelerometer...");

  long sum_ax = 0, sum_ay = 0, sum_az = 0;
  const int samples = 1000;

  // Take multiple samples
  for (int i = 0; i < samples; i++)
  {
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    sum_ax += ax;
    sum_ay += ay;
    sum_az += az;

    if (i % 100 == 0)
    {
      Serial.print(".");
      display.setCursor(0, 20);
      display.print("Progress: ");
      display.print(i / 10);
      display.print("%");
      display.display();
    }
    delay(2);
  }

  // Calculate offsets
  ax_offset = sum_ax / samples;
  ay_offset = sum_ay / samples;
  az_offset = (sum_az / samples) - 16384; // Subtract 1g for Z axis

  Serial.println("\nCalibration complete!");
  Serial.print("Offsets - X: ");
  Serial.print(ax_offset);
  Serial.print(" Y: ");
  Serial.print(ay_offset);
  Serial.print(" Z: ");
  Serial.println(az_offset);
}
