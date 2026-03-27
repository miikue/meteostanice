#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Adafruit_LTR390.h>

const char *WIFI_SSID = "Hornet";
const char *WIFI_PASSWORD = "jahoda6399";
const char *SERVER_HOST = "10.10.10.214";
const uint16_t SERVER_PORT = 5000;
const char *SERVER_PATH = "/ingest";

TwoWire Wire2 = TwoWire(1);

Adafruit_BME280 bme1;
Adafruit_LTR390 ltr1 = Adafruit_LTR390();

Adafruit_BME280 bme2;
Adafruit_LTR390 ltr2 = Adafruit_LTR390();

const int VOLTAGE_PIN = 6;
const int CURRENT_PIN = 5;



bool bme1Ok = false;
bool ltr1Ok = false;
bool bme2Ok = false;
bool ltr2Ok = false;

const float SEA_LEVEL_PRESSURE_HPA = 1013.25F;

unsigned long lastSend = 0;
const unsigned long SEND_INTERVAL_MS = 3000;

float measureVoltage() {
  const float VOLTAGE_RATIO = 5.0f;
  const float VOLTAGE_CAL = 1.47f;
  
  float sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(VOLTAGE_PIN);
    delay(1);
  }
  float voltADC = (sum / 10.0f) * 3.3f / 4095.0f;
  float voltage = voltADC * VOLTAGE_RATIO * VOLTAGE_CAL;
  return voltage;
}

// Měření proudu - vrací hodnotu v Ampérech
float measureCurrent() {
  const float CURRENT_SENSITIVITY = 0.185f;
  const float CURRENT_OFFSET = 2.5f;
  
  float sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(CURRENT_PIN);
    delay(1);
  }
  float currADC = (sum / 10.0f) * 3.3f / 4095.0f;
  float current = (currADC - CURRENT_OFFSET) / CURRENT_SENSITIVITY;
  
  if (current > -0.1f && current < 0.1f) {
    current = 0.0f;
  }
  
  return current;
}


void connectWifi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  Serial.print("Pripojuji k WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  const unsigned long timeoutMs = 20000;

  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    delay(500);
    Serial.print('.');
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi pripojeno.");
    Serial.print("IP adresa ESP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Nepodarilo se pripojit k WiFi.");
  }
}

void setupSensors() {
  pinMode(35, OUTPUT);
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);
  pinMode(47, OUTPUT);
  digitalWrite(47, HIGH);
  delay(1000);

  Wire.begin(42, 2);
  Wire2.begin(17, 18);

  bme1Ok = bme1.begin(0x77, &Wire);
  ltr1Ok = ltr1.begin(&Wire);

  bme2Ok = bme2.begin(0x77, &Wire2);
  ltr2Ok = ltr2.begin(&Wire2);

  if (!bme1Ok) Serial.println("BME1 chyba!");
  if (!ltr1Ok) Serial.println("LTR1 chyba!");
  if (!bme2Ok) Serial.println("BME2 chyba!");
  if (!ltr2Ok) Serial.println("LTR2 chyba!");

  if (ltr1Ok) {
    ltr1.setMode(LTR390_MODE_UVS);
    ltr1.setGain(LTR390_GAIN_3);
    ltr1.setResolution(LTR390_RESOLUTION_18BIT);
  }

  if (ltr2Ok) {
    ltr2.setMode(LTR390_MODE_UVS);
    ltr2.setGain(LTR390_GAIN_3);
    ltr2.setResolution(LTR390_RESOLUTION_18BIT);
  }
}

String readPayload() {
  float t1 = bme1Ok ? bme1.readTemperature() : NAN;
  float h1 = bme1Ok ? bme1.readHumidity() : NAN;
  float p1 = bme1Ok ? (bme1.readPressure() / 100.0F) : NAN;
  float a1 = bme1Ok ? bme1.readAltitude(SEA_LEVEL_PRESSURE_HPA) : NAN;
  uint32_t uv1 = 0;
  uint32_t als1 = 0;
  if (ltr1Ok) {
    ltr1.setMode(LTR390_MODE_UVS);
    uv1 = ltr1.readUVS();
    ltr1.setMode(LTR390_MODE_ALS);
    als1 = ltr1.readALS();
  }

  float t2 = bme2Ok ? bme2.readTemperature() : NAN;
  float h2 = bme2Ok ? bme2.readHumidity() : NAN;
  float p2 = bme2Ok ? (bme2.readPressure() / 100.0F) : NAN;
  float a2 = bme2Ok ? bme2.readAltitude(SEA_LEVEL_PRESSURE_HPA) : NAN;
  uint32_t uv2 = 0;
  uint32_t als2 = 0;
  if (ltr2Ok) {
    ltr2.setMode(LTR390_MODE_UVS);
    uv2 = ltr2.readUVS();
    ltr2.setMode(LTR390_MODE_ALS);
    als2 = ltr2.readALS();
  }

  long rssi = WiFi.RSSI();

  String payload;
  payload.reserve(260);
  payload += "t1=";
  payload += String(t1, 2);
  payload += ";h1=";
  payload += String(h1, 2);
  payload += ";p1=";
  payload += String(p1, 2);
  payload += ";a1=";
  payload += String(a1, 2);
  payload += ";uv1=";
  payload += String(uv1);
  payload += ";als1=";
  payload += String(als1);
  payload += ";t2=";
  payload += String(t2, 2);
  payload += ";h2=";
  payload += String(h2, 2);
  payload += ";p2=";
  payload += String(p2, 2);
  payload += ";a2=";
  payload += String(a2, 2);
  payload += ";uv2=";
  payload += String(uv2);
  payload += ";als2=";
  payload += String(als2);
  payload += ";rssi=";
  payload += String(rssi);
  return payload;
}

bool postPayload(const String &payload) {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  HTTPClient http;
  String url = String("http://") + SERVER_HOST + ":" + String(SERVER_PORT) + SERVER_PATH;

  http.begin(url);
  http.addHeader("Content-Type", "text/plain");
  int code = http.POST((uint8_t *)payload.c_str(), payload.length());
  String response = http.getString();
  http.end();

  Serial.print("POST ");
  Serial.print(url);
  Serial.print(" -> ");
  Serial.print(code);
  Serial.print(" | payload: ");
  Serial.println(payload);

  if (response.length() > 0) {
    Serial.print("Server odpoved: ");
    Serial.println(response);
  }

  return code > 0 && code < 300;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  pinMode(VOLTAGE_PIN, INPUT);
  pinMode(CURRENT_PIN, INPUT);

  setupSensors();
  connectWifi();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWifi();
    delay(1000);
    return;
  }


    float voltage = measureVoltage();
  float current = measureCurrent();
  
  Serial.print("U: ");
  Serial.print(voltage, 1);
  Serial.print("V | I: ");
  Serial.print(current, 2);
  Serial.println("A");
  delay(1000);

  if (millis() - lastSend >= SEND_INTERVAL_MS) {
    lastSend = millis();
    String payload = readPayload();
    if (!postPayload(payload)) {
      Serial.println("Odeslani selhalo.");
    }
  }
}