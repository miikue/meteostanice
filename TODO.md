# Plán projektu

## BME280 (teplota, vlhkost, tlak)
- [x] Zapojit na dvě nezávislé I2C sběrnice (GPIO 42/2 a GPIO 17/18)
- [x] Ověřit přesnost na obou sběrnicích

## LTR390 (UV, ambient light)
- [x] Zapojit na obě sběrnice
- [ ] Kalibrovat / porovnat s referenčním měřidlem

## Napěťový dělič (GPIO 7)
- [x] Zapojit (R1 = R2 = 99 kΩ)
- [x] Kalibrovat korekční faktor proti multimetru
— viz [docs/voltage-measurement.md](docs/voltage-measurement.md)

## INA226 (proud a napětí solárního panelu)
- [x] Zapojit do nabíjecí větve (mezi panel a nabíječku), I2C
- [x] Implementovat driver v `main.cpp`, nahradit stávající `measureCurrent()`
- [ ] Kalibrovat (shunt odpor, rozsah)

## Mikrofon (hluk)
- [ ] Implementace "hluku" venku
- [ ] Implementovat měření hladiny hluku (dB SPL průměr za interval)
- [ ] Přidat `noise` pole do payloadu a CSV schématu serveru

## Krabička
- [ ] Venkovní část: BME280 + LTR390 + solární panel + LiPo — větrání bez přímého slunce (Stevenson screen princip)
- [ ] Vnitřní část: BME280 + LTR390
- [ ] Řešení průchodky / propojení mezi částmi

## Home Assistant
- [ ] Integrace posílání
- [ ] inteligentní klíčování
