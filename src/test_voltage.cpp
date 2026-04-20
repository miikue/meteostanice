#include <Arduino.h>


// ---------------------------------------------
// Test měření napětí z baterie pomocí děliče napětí a ADC
// ---------------------------------------------


const int analogPin = 7;
const float R1 = 99000.0;
const float R2 = 99000.0;
const float ADC_REF = 3.3;
const int ADC_RES = 4095;

// Kalibrace měření
const float correction = 1.05;

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db); // rozsah 0–3.3V
}

void loop() {
  int raw = analogRead(analogPin);
  float vout = (raw / (float)ADC_RES) * ADC_REF;
  float vin = vout * ((R1 + R2) / R2) * correction;

  int percent = constrain((int)((vin - 3.0) / (4.2 - 3.0) * 100), 0, 100);

  Serial.print("Napeti: ");
  Serial.print(vin, 2);
  Serial.print(" V  |  Nabiti: ");
  Serial.print(percent);
  Serial.println(" %");

  delay(2000);
}
