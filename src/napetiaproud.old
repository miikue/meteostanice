#include <Arduino.h>

const int VOLTAGE_PIN = 6;
const int CURRENT_PIN = 5;

// Měření napětí - vrací hodnotu v Voltech
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

void setup() {
  Serial.begin(115200);
  delay(100);
  
  pinMode(VOLTAGE_PIN, INPUT);
  pinMode(CURRENT_PIN, INPUT);
  

}

void loop() {
  float voltage = measureVoltage();
  float current = measureCurrent();
  
  Serial.print("U: ");
  Serial.print(voltage, 1);
  Serial.print("V | I: ");
  Serial.print(current, 2);
  Serial.println("A");
  delay(1000);
}