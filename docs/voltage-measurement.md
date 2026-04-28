# Battery voltage measurement

Voltage is measured via a resistor divider on **GPIO 7** (ADC).

## Circuit

```
  Battery (+)
      │
    [R1 = 99 kΩ]
      │
      ├────────────── GPIO 7 (ADC, 12-bit, 11 dB attenuation)
      │
    [R2 = 99 kΩ]
      │
     GND
```

Because R1 = R2, the midpoint is exactly half the battery voltage:

```
Vgpio = Vbat × R2 / (R1 + R2) = Vbat / 2
```

The ADC measures 0–3.3 V at 12-bit resolution (0–4095), so the full formula used in firmware is:

```
Vgpio = (ADC_raw / 4095) × 3.3 V
Vbat  = Vgpio × (R1 + R2) / R2 × correction
      = Vgpio × 2 × 1.05
```

The `correction = 1.05` factor compensates for resistor tolerances and ADC non-linearity, calibrated empirically against a multimeter.

## Sampling

500 samples are averaged with 100 µs between each to reduce ADC noise:

```cpp
for (int i = 0; i < 500; i++) {
    sum += analogRead(7);
    delayMicroseconds(100);
}
```

## Expected range

| State            | Vbat   | Vgpio  |
|------------------|--------|--------|
| Fully charged    | 4.20 V | 2.10 V |
| Nominal          | 3.70 V | 1.85 V |
| Low (cut-off)    | 3.00 V | 1.50 V |

## Calibration

Use the `test-voltage` PlatformIO environment to verify readings against a multimeter before deploying:

```bash
pio run -e test-voltage -t upload
pio device monitor
```

Adjust `correction` in `measureVoltage()` ([src/main.cpp](../src/main.cpp)) if needed.
