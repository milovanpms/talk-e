# 🔋 Power Supply Circuit

Designing a power supply circuit is a crucial step when you create a device. Especially portable ones: you have to think about the global consumption of the device, which battery suits the best for a *not-ridiculous* autonomy, how to charge it and how to protect the circuit; **to make sound, not smoke**.

## A little block diagram

![](https://cdn.discordapp.com/attachments/1492625020097396847/1516455921147183134/image.png?ex=6a32b521&is=6a3163a1&hm=deca42b63f35150d0141929ca9897d89b6d82a8351da2e7c4ca563a59d3cf085&)

## Power Consumption

This estimate was done early in the design process to size the LDO regulator correctly. The audio amp (**MAX98357A**) is fed directly from the battery output rather than through the **AP2112K**, which would never survive a `500 mA` peak on top of everything else.

| Component | Typical current |
|---|---|
| **ESP32-S3** | `~240 mA peak`; `~80 mA average` |
| **SX1278** | `~120 mA (TX)` |
| **SH1106** | `~20 mA` |
| Vibration motor | `~100 mA peak`; `~60 - 80 mA average` |
| LEDs | `~20 mA` |
| Audio amp | `~500 mA peak`; not connected on regulator |
| Buttons | `~0 mA` |
| Switch | `~0 mA` |
| **Total peak** | `~500 mA` (excluding audio amp) |
| **Total peak** | `~1000 mA` (including audio amp) |

## Voltage Regulation (AP2112K-3.3)

The **AP2112K-3.3** is a low-dropout linear regulator that converts the battery voltage down to a stable `3.3 V` for the logic side of the circuit. It has a dropout voltage of `~300 mV`, which means it needs at least `3.6 V` on its input to guarantee a stable `3.3 V` output. The audio amp (**MAX98357A**) bypasses it entirely and runs directly off the battery output, keeping the LDO within its limits. It can also provide up to `600 mA`, which is perfect for our application.

## Charging Circuit (TP4056)

The **TP4056** is a single-cell Li-Ion charge controller. It handles the full charging 
cycle and signals its state via the `CHRG` pin.
We configured the charge current to `580 mA` via `RPROG` (`R8`); other values are available in the 
datasheet.
There is also a `STDBY` pin, used to get the charge termination status. We won't use it in this project.

| Terminal | Output | Voltage | Usage |
|---|---|---|---|
| `B+` / `B-` | Raw battery voltage | `3.7 V` | Physical battery connection |
| `OUT+` / `OUT-` | Protected battery voltage | `3.7 V` | Rest of the circuit |

![](https://cdn.discordapp.com/attachments/1492625020097396847/1516448551440486500/image.png?ex=6a32ae44&is=6a315cc4&hm=42ca56bec842b8bc6b1261c2de7a270db4878cf74a8e55163be2bb9de769d467&)

## Battery Protection (**DW01A** + **FS8205A**)

The **DW01A** is a Li-Ion protection IC. It monitors the battery and cuts 
the circuit in three situations:

| Fault | Threshold | Action |
|---|---|---|
| Overcharge (OC) | `> 4.25 V` | Cuts charge MOSFET |
| Overdischarge (OD) | `< 2.5 – 2.7 V` | Cuts discharge MOSFET |
| Overcurrent / short | Current spike | Cuts immediately |

The **DW01A** doesn't cut current directly; it drives the **FS8205A**, a dual back-to-back 
MOSFET in a single package. Two MOSFETs are needed because a single one can't block 
current in both directions due to its body diode.

One more thing to keep in mind: the **AP2112K** LDO drops out at `VIN < 3.6 V`, while the 
**DW01A** only cuts at `~2.6 V`. There's a grey zone between `~3.0 V` and `~3.6 V` where the 
battery is still connected but the `3.3 V` rail is no longer guaranteed; the firmware 
should monitor battery voltage via ADC and warn the user before reaching this zone. (todo!)

![](https://cdn.discordapp.com/attachments/1492625020097396847/1516449519938703410/image.png?ex=6a32af2b&is=6a315dab&hm=2d017f279785e2bec0c3d20f08ee5d391e1a2b5171d34fbb342cbe7263685ad8&)

## Charge Indicator

The **TP4056** exposes a `CHRG` open-drain pin that reflects the charging state. We use it 
both for a visual LED indicator and for a GPIO read by the **ESP32-S3** to show it on the screen.

| State | CHRG pin | LED | **ESP32-S3** reads |
|---|---|---|---|
| Charging | LOW (active) | ON | `LOW` |
| Charge complete | Floating (pull-up) | OFF | `HIGH` |
| No battery / error | Blinking | Blinking | Alternating |

The GPIO is pulled up to `3.3 V` via `10 kΩ` when `CHRG` is floating, and pulled to `GND` via 
`47 kΩ` when active. The **ESP32-S3** displays the corresponding icon on the OLED display.
