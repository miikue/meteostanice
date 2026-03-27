#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <esp_sleep.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Adafruit_LTR390.h>

const char *WIFI_SSID = "Hornet";
const char *WIFI_PASSWORD = "jahoda6399";
const char *SERVER_HOST = "10.10.10.214";
const uint16_t SERVER_PORT = 5000;
const char *SERVER_PATH = "/ingest";

const int SENSOR_POWER_PIN = 35;
const int SENSOR_EN_PIN_1 = 4;
const int SENSOR_EN_PIN_2 = 47;
const unsigned long SENSOR_POWER_STABILIZE_MS = 1000;

TwoWire Wire2 = TwoWire(1);

Adafruit_BME280 bme1;
Adafruit_LTR390 ltr1 = Adafruit_LTR390();

Adafruit_BME280 bme2;
Adafruit_LTR390 ltr2 = Adafruit_LTR390();

bool bme1Ok = false;
bool ltr1Ok = false;
bool bme2Ok = false;
bool ltr2Ok = false;

const float SEA_LEVEL_PRESSURE_HPA = 1013.25F;

const uint64_t DEEP_SLEEP_SECONDS = 60;

// Promena na pocet startu z deep sleep modu, pro otestovani funkce deep sleep
RTC_DATA_ATTR int bootCount = 0;

// wifi deep sleep promena
RTC_DATA_ATTR uint8_t bssid[6];
RTC_DATA_ATTR int channel = 0;
RTC_DATA_ATTR uint32_t ip, gateway, subnet;
RTC_DATA_ATTR bool config_valid = false;

// ==================== Funkce =====================

void powerSensors(bool on) {
  pinMode(SENSOR_POWER_PIN, OUTPUT);
  pinMode(SENSOR_EN_PIN_1, OUTPUT);
  pinMode(SENSOR_EN_PIN_2, OUTPUT);
  digitalWrite(SENSOR_POWER_PIN, on ? HIGH : LOW);
  digitalWrite(SENSOR_EN_PIN_1, on ? HIGH : LOW);
  digitalWrite(SENSOR_EN_PIN_2, on ? HIGH : LOW);
}


float measureVoltage() {
  const float VOLTAGE_RATIO = 5.0f;
  const float VOLTAGE_CAL = 1.47f;
  
  float sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(6);
    delay(1);
  }
  float voltADC = (sum / 10.0f) * 3.3f / 4095.0f;
  float voltage = voltADC * VOLTAGE_RATIO * VOLTAGE_CAL;
  return voltage;
}

float measureCurrent() {
  const float CURRENT_SENSITIVITY = 0.185f;
  const float CURRENT_OFFSET = 2.5f;
  
  float sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(5);
    delay(1);
  }
  float currADC = (sum / 10.0f) * 3.3f / 4095.0f;
  float current = (currADC - CURRENT_OFFSET) / CURRENT_SENSITIVITY;
  
  if (current > -0.1f && current < 0.1f) {
    current = 0.0f;
  }
  
  return current;
}

bool waitForWifi(unsigned long timeoutMs) {
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeoutMs) {
    delay(20);
  }
  return WiFi.status() == WL_CONNECTED;
}

bool connectWifi() {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  Serial.print("Pripojuji k WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);

  // 1) Pokus o rychly reconnect z RTC cache (deep sleep)
  if (config_valid && channel > 0) {
    bool staticOk = WiFi.config(IPAddress(ip), IPAddress(gateway), IPAddress(subnet));
    if (staticOk) {
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD, channel, bssid);
      if (waitForWifi(12000)) {
        Serial.println("WiFi pripojeno (rychly reconnect).");
        Serial.print("IP adresa ESP: ");
        Serial.println(WiFi.localIP());
        return true;
      }
    }

    // Cache je nejspis zastarala (BSSID/channel/IP), zneplatnit a jit na ciste DHCP.
    config_valid = false;
    WiFi.disconnect(true, true);
    delay(100);
  }

  // 2) Fallback: bez cache, klasicke pripojeni s DHCP
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  if (!waitForWifi(12000)) {
    Serial.println("Nepodarilo se pripojit k WiFi.");
    return false;
  }

  // Uspesne pripojeni - ulozit hodnoty pro pristi wake-up z deep sleep.
  ip = WiFi.localIP();
  gateway = WiFi.gatewayIP();
  subnet = WiFi.subnetMask();
  const uint8_t* currentBssid = WiFi.BSSID();
  if (currentBssid != nullptr) {
    memcpy(bssid, currentBssid, 6);
  }
  channel = WiFi.channel();
  config_valid = true;

  Serial.println("WiFi pripojeno.");
  Serial.print("IP adresa ESP: ");
  Serial.println(WiFi.localIP());
  return true;
}

void setupSensors() {

  // Napeti a proud
  pinMode(6, INPUT);
  pinMode(5, INPUT);

  // I2C senzory
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

  float voltage = measureVoltage();
  float current = measureCurrent();
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
  payload += ";voltage=";
  payload += String(voltage, 2);
  payload += ";current=";
  payload += String(current, 2);
  payload += ";boot=";
  payload += String(bootCount);
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

void sleepForNextCycle() {
  Serial.println("Ukladam do deep sleep na 60 s...");
  powerSensors(false);
  delay(5);
  Serial.flush();
  WiFi.disconnect(true, false);
  WiFi.mode(WIFI_OFF);
  esp_sleep_enable_timer_wakeup(DEEP_SLEEP_SECONDS * 1000000ULL);
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);

  bootCount++;

  powerSensors(true);
  delay(SENSOR_POWER_STABILIZE_MS);
  setupSensors();
}

void loop() {
  if (connectWifi()) {
    String payload = readPayload();
    if (!postPayload(payload)) {
      Serial.println("Odeslani selhalo.");
    }
  } else {
    Serial.println("WiFi nedostupna, uspavam i bez odeslani.");
  }

  sleepForNextCycle();
}