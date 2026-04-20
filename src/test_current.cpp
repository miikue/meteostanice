#include <Arduino.h>

const int CURRENT_PIN = 6;
const float ADC_REF = 3.3;
const int ADC_RES = 4095;
const float DIVIDER = 2.0;      // R1=R2=100k → dělí napětí 2x
float SENSITIVITY_VAR = 0.066;  // 66mV/A pro 30A verzi
float zeroOffset = 2.500;       // nastavíš přes 'k' nebo 'o'

void kalibrujOffset() {
  long sum = 0;
  for (int i = 0; i < 2000; i++) {
    sum += analogRead(CURRENT_PIN);
    delayMicroseconds(100);
  }
  zeroOffset = (sum / 2000.0 / ADC_RES) * ADC_REF * DIVIDER;
  Serial.print("Offset nastaven na: ");
  Serial.print(zeroOffset, 4);
  Serial.println(" V");
}

float readCurrent() {
  long sum = 0;
  for (int i = 0; i < 500; i++) {
    sum += analogRead(CURRENT_PIN);
    delayMicroseconds(100);
  }
  float voltage = (sum / 500.0 / ADC_RES) * ADC_REF * DIVIDER;
  float current = (voltage - zeroOffset) / SENSITIVITY_VAR;
  if (abs(current) < 0.020) current = 0.0;
  return current;
}

void printHelp() {
  Serial.println("--- Prikazy ---");
  Serial.println("k        = kalibrace offsetu (odpoj panel!)");
  Serial.println("s<cislo> = nastav sensitivity, napr. s0.066");
  Serial.println("o<cislo> = nastav offset rucne, napr. o2.500");
  Serial.println("p        = vypis aktualni nastaveni");
  Serial.println("---------------");
}

void handleSerial() {
  if (!Serial.available()) return;
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  cmd.toLowerCase();

  if (cmd == "k") {
    Serial.println("Kalibruji offset...");
    kalibrujOffset();
  } else if (cmd.startsWith("s")) {
    float val = cmd.substring(1).toFloat();
    if (val > 0) {
      SENSITIVITY_VAR = val;
      Serial.print("Sensitivity nastavena: "); Serial.println(val, 4);
    }
  } else if (cmd.startsWith("o")) {
    zeroOffset = cmd.substring(1).toFloat();
    Serial.print("Offset nastaven: "); Serial.println(zeroOffset, 4);
  } else if (cmd == "p") {
    Serial.print("Offset: "); Serial.println(zeroOffset, 4);
    Serial.print("Sensitivity: "); Serial.println(SENSITIVITY_VAR, 4);
  } else {
    printHelp();
  }
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  Serial.println("Odpoj panel, stiskni Enter pro kalibraci offsetu...");
  while (!Serial.available()) delay(100);
  Serial.read();
  kalibrujOffset();
}

void loop() {
  handleSerial();
  float I = readCurrent();
  Serial.print("Proud: ");
  Serial.print(I, 3);
  Serial.println(" A");
  delay(1000);
}
