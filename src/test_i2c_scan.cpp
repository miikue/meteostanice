#include <Arduino.h>
#include <Wire.h>

#define BUS0_SDA  42
#define BUS0_SCL   2
#define BUS1_SDA  17
#define BUS1_SCL  18

#define SENSOR_POWER_PIN 35
#define SENSOR_EN_PIN_1   4
#define SENSOR_EN_PIN_2  47

TwoWire WireBus1 = TwoWire(1);

void scanBus(TwoWire& bus, const char* name) {
    Serial.printf("Bus %s (SDA=%s SCL=%s):\n",
                  name,
                  (name[0] == '0') ? "42" : "17",
                  (name[0] == '0') ? "2"  : "18");

    int found = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        bus.beginTransmission(addr);
        if (bus.endTransmission() == 0) {
            Serial.printf("  0x%02X\n", addr);
            found++;
        }
    }
    if (found == 0)
        Serial.println("  (zadne zarizeni)");
    Serial.println();
}

void setup() {
    Serial.begin(115200);
    delay(600);

    pinMode(SENSOR_POWER_PIN, OUTPUT);
    pinMode(SENSOR_EN_PIN_1,  OUTPUT);
    pinMode(SENSOR_EN_PIN_2,  OUTPUT);
    digitalWrite(SENSOR_POWER_PIN, HIGH);
    digitalWrite(SENSOR_EN_PIN_1,  HIGH);
    digitalWrite(SENSOR_EN_PIN_2,  HIGH);
    delay(1000);

    Wire.begin(BUS0_SDA, BUS0_SCL);
    Wire.setClock(100000);
    Wire.setTimeOut(50);

    WireBus1.begin(BUS1_SDA, BUS1_SCL);
    WireBus1.setClock(100000);
    WireBus1.setTimeOut(50);
}

void loop() {
    Serial.println("=== I2C scan ===");
    scanBus(Wire,     "0");
    scanBus(WireBus1, "1");
    delay(3000);
}
