# 🔋 Power Supply Circuit

## Charging Circuit (TP4056)

x

## Battery Protection (DW01A + FS8205)

x

## Power Regulation (AMS1117-3.3)

x

## Charge Indicator

x

## Power Consumption

This estimate was done early in the design process to size the LDO regulator correctly. The audio amp (MAX98357A) is fed directly from the battery output rather than through the AMS1117, which would never survive a 500 mA peak on top of everything else.

| Component | Typical current |
|---|---|
| ESP32-S3 | ~240 mA peak, ~80 mA average |
| SX1278 | ~120 mA (TX) |
| SH1106 | ~20 mA |
| Vibration motor | ~100 mA peak, ~60-80 mA average |
| LEDs | ~20 mA |
| Audio amp | ~500 mA peak; not connected on regulator |
| Buttons | ~0 mA |
| Switch | ~0 mA |
| **Total peak** | **~500 mA** (excluding audio amp) |
| **Total peak** | **~1000 mA** (including audio amp) |
