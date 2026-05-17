#include <Arduino.h>
#include <Wire.h>

#define INA226_ADDR   0x40
#define REG_CONFIG    0x00
#define REG_SHUNT     0x01
#define REG_BUS       0x02
#define REG_CURRENT   0x04
#define REG_CALIB     0x05

// AVG=16, VBUSCT=1.1ms, VSHCT=1.1ms, MODE=continuous
#define CONFIG_VALUE  0x4527
// CAL = 0.00512 / (0.0001 A * 0.1 Ohm) = 512
#define CALIB_VALUE   512
#define CURRENT_LSB   0.0001f  // 0.1 mA/bit

void writeReg(uint8_t reg, uint16_t val) {
    Wire.beginTransmission(INA226_ADDR);
    Wire.write(reg);
    Wire.write(val >> 8);
    Wire.write(val & 0xFF);
    Wire.endTransmission();
}

uint16_t readReg(uint8_t reg) {
    Wire.beginTransmission(INA226_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)INA226_ADDR, (uint8_t)2);
    return ((uint16_t)Wire.read() << 8) | Wire.read();
}

void setup() {
    Serial.begin(115200);
    delay(500);

    pinMode(35, OUTPUT); pinMode(4, OUTPUT); pinMode(47, OUTPUT);
    digitalWrite(35, HIGH); digitalWrite(4, HIGH); digitalWrite(47, HIGH);
    delay(1000);

    Wire.begin(17, 18);
    Wire.setTimeOut(50);

    Wire.beginTransmission(INA226_ADDR);
    if (Wire.endTransmission() != 0) {
        Serial.println("INA226 nenalezen!");
        while (true) delay(1000);
    }

    writeReg(REG_CONFIG, 0x8000);  // reset
    delay(10);
    writeReg(REG_CONFIG, CONFIG_VALUE);
    writeReg(REG_CALIB,  CALIB_VALUE);
    delay(50);  // první konverze (16 vzorků × 2 × 1.1ms ≈ 35ms)

    Serial.println("Napeti [V]  | Proud [mA] | Vykon [mW]");
    Serial.println("────────────────────────────────────────");
}

void loop() {
    float voltage = (int16_t)readReg(REG_BUS)     * 0.00125f;
    float current = (int16_t)readReg(REG_CURRENT)  * CURRENT_LSB;
    float power   = voltage * current;

    Serial.printf("  %8.4f V | %8.3f mA | %8.3f mW\n",
                  voltage, current * 1000.0f, power * 1000.0f);
    delay(500);
}
