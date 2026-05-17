# Meteostanice

> Plán a stav projektu: [TODO.md](TODO.md)

ESP32-S3 weather station that wakes every 60 s, reads sensors, and POSTs the data over Wi-Fi to a local Python server that stores it in a CSV file.
In future to Home assistant.

## Hardware

- **MCU**: ESP32-S3-DevKitC-1
- **Sensors (2× independent I2C buses)**:
  - BME280 – temperature, humidity, pressure, altitude
  - LTR390 – UV index, ambient light
- **Power monitoring**:
  - Battery voltage: voltage divider on GPIO 7 — see [docs/voltage-measurement.md](docs/voltage-measurement.md)
  - Solar panel: INA226 (I2C 0x40, Bus 1) — measures panel voltage and current, shunt 0.1 Ω
- **Sensor power switch**: GPIO 35, 4, 47 (powered off before deep sleep)

## Firmware ([src/main.cpp](src/main.cpp))

- Reads both sensor sets on startup
- Connects to Wi-Fi (uses RTC-cached BSSID/channel/IP for fast reconnect after deep sleep)
- POSTs a semicolon-delimited payload to `POST /ingest`
- Enters deep sleep for 60 s

### Payload format

```
t1=22.50;h1=55.10;p1=1012.30;a1=10.50;uv1=123;als1=456;t2=22.40;h2=54.80;p2=1012.25;a2=10.60;uv2=120;als2=450;rssi=-65;voltage=4.12;panel_v=18.2500;panel_i=0.3200;boot=42
```

| Pole | Popis |
|---|---|
| `t1/t2` | teplota °C (BME280 Bus 0 / Bus 1) |
| `h1/h2` | vlhkost % |
| `p1/p2` | tlak hPa |
| `a1/a2` | nadmořská výška m |
| `uv1/uv2` | UV index (LTR390) |
| `als1/als2` | okolní světlo (LTR390) |
| `rssi` | Wi-Fi signál dBm |
| `voltage` | napětí baterie V (dělič GPIO 7) |
| `panel_v` | napětí solárního panelu V (INA226) |
| `panel_i` | proud solárního panelu A (INA226) |
| `boot` | počet bootů z deep sleep |

## Server ([src/server/server.py](src/server/server.py))

![Server output](image.png)

Minimal Python HTTP server (no dependencies beyond stdlib).

```bash
python3 src/server/server.py
```

- Listens on `0.0.0.0:5000`
- `POST /ingest` – saves payload to `src/server/sensor_data.csv`
- `GET /health` – returns `ok`
- Automatically migrates CSV header if the schema changes

## Test environments

| PlatformIO env | Source file | Purpose |
|---|---|---|
| `main` | `src/main.cpp` | Full station firmware |
| `test-voltage` | `src/test_voltage.cpp` | Voltage divider calibration |
| `test-current` | `src/test_current.cpp` | ACS712 current sensor calibration |
| `test-ina226` | `src/test_ina226.cpp` | INA226 panel voltage/current readout |
| `test-i2c-scan` | `src/test_i2c_scan.cpp` | I2C scan obou sběrnic |

## Build & flash

```bash
pio run -e main -t upload
pio device monitor
```
