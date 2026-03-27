#include <Arduino.h>
#include <math.h>

// Definice analog pinů
const int VOLTAGE_PIN = 6;  // Pin na čtení napětí (GPIO34)
const int CURRENT_PIN = 5;  // Pin na čtení proudu (GPIO35)

// Kalibrační konstanty - upravit podle vašeho hardware
const float VOLTAGE_DIVIDER_RATIO = 5.0f;                // Modul 0-25V: deleni zhruba 5:1
const float VOLTAGE_CALIBRATION_FACTOR = 1.47f;          // Doladeni podle multimetru (3.137V vs ~2.13V)
const float CURRENT_SENSITIVITY = 0.185;      // Ampér na Volt (např. ACS712-5A: 0.185 A/V)
const float CURRENT_ZERO_DEADBAND_A = 0.10f;             // Potlaceni sumu okolo 0 A

// Proměnné pro průměrování
const int NUM_SAMPLES = 32;
float currentOffsetVoltage = 0.0f;

int readAveragedRaw(int pin, int samples) {
  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(pin);
    delay(1);
  }
  return (int)(sum / samples);
}

int readAveragedMilliVolts(int pin, int samples) {
  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogReadMilliVolts(pin);
    delay(1);
  }
  return (int)(sum / samples);
}

void calibrateCurrentOffset() {
  Serial.println("Kalibrace offsetu proudu (bez zateze)...");
  const int calibrationSamples = 400;
  int currentMv = readAveragedMilliVolts(CURRENT_PIN, calibrationSamples);
  currentOffsetVoltage = currentMv / 1000.0f;
  Serial.print("Offset proudu: ");
  Serial.print(currentOffsetVoltage, 3);
  Serial.println(" V");
}

void setup() {
  Serial.begin(115200);
  delay(100);

#if defined(ESP32)
  analogReadResolution(12);
  analogSetPinAttenuation(VOLTAGE_PIN, ADC_11db);
  analogSetPinAttenuation(CURRENT_PIN, ADC_11db);
#endif
  
  // Nastavení pinů jako input
  pinMode(VOLTAGE_PIN, INPUT);
  pinMode(CURRENT_PIN, INPUT);

  calibrateCurrentOffset();
  
  Serial.println("=== Čtečka Napětí a Proudu ===");
  Serial.println("Inicializace hotova...");
  Serial.println();
}

void loop() {
  // Čtení napětí (průměr několika vzorků)
  int voltageMv = readAveragedMilliVolts(VOLTAGE_PIN, NUM_SAMPLES);
  
  // Čtení proudu (průměr několika vzorků)
  int currentMv = readAveragedMilliVolts(CURRENT_PIN, NUM_SAMPLES);
  
  // Konverze na fyzické jednotky
  float voltage = (voltageMv / 1000.0f) * VOLTAGE_DIVIDER_RATIO * VOLTAGE_CALIBRATION_FACTOR;
  float currentVoltage = currentMv / 1000.0f;
  float current = (currentVoltage - currentOffsetVoltage) / CURRENT_SENSITIVITY;
  if (fabsf(current) < CURRENT_ZERO_DEADBAND_A) {
    current = 0.0f;
  }
  
  // Výstup do Serial monitoru
  Serial.print("Napětí: ");
  Serial.print(voltage, 2);
  Serial.print(" V | ");
  
  Serial.print("Proud: ");
  Serial.print(current, 2);
  Serial.println(" A");
  
  // Čekání před dalším měřením
  delay(1000);
}
