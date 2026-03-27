#include <Arduino.h>

// Definice analog pinů
const int VOLTAGE_PIN = 6;  // Pin na čtení napětí (GPIO34)
const int CURRENT_PIN = 5;  // Pin na čtení proudu (GPIO35)

// Kalibrační konstanty - upravit podle vašeho hardware
const float VOLTAGE_FACTOR = 3.3 / 4095.0;    // ESP32: 3.3V reference, 12-bit ADC
const float CURRENT_OFFSET = 2.5;             // Offset pro proudový senzor (meist 2.5V)
const float CURRENT_SENSITIVITY = 0.185;      // Ampér na Volt (např. ACS712-5A: 0.185 A/V)

// Proměnné pro průměrování
const int NUM_SAMPLES = 10;

void setup() {
  Serial.begin(115200);
  delay(100);
  
  // Nastavení pinů jako input
  pinMode(VOLTAGE_PIN, INPUT);
  pinMode(CURRENT_PIN, INPUT);
  
  Serial.println("=== Čtečka Napětí a Proudu ===");
  Serial.println("Inicializace hotova...");
  Serial.println();
}

void loop() {
  // Čtení napětí (průměr několika vzorků)
  int rawVoltage = 0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    rawVoltage += analogRead(VOLTAGE_PIN);
    delay(1);
  }
  rawVoltage /= NUM_SAMPLES;
  
  // Čtení proudu (průměr několika vzorků)
  int rawCurrent = 0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    rawCurrent += analogRead(CURRENT_PIN);
    delay(1);
  }
  rawCurrent /= NUM_SAMPLES;
  
  // Konverze na fyzické jednotky
  float voltage = rawVoltage * VOLTAGE_FACTOR;
  float currentVoltage = rawCurrent * VOLTAGE_FACTOR;
  float current = (currentVoltage - CURRENT_OFFSET) / CURRENT_SENSITIVITY;
  
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
