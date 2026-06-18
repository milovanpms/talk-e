# 🔌 Electronics

## Prototyping

Before diving headfirst into a custom PCB, we did what every engineer on the planet would do: we grabbed a dev board off the shelf and started tinkering with it. The goal was to validate the core ideas: LoRa audio streaming, Opus encoding on an **ESP32** and the FreeRTOS task architecture.

### The development platform

The [**TTGO T-Beam V1.1**](https://github.com/lilygo/ttgo-t-beam) (by LilyGO) was our prototyping board of choice. It's a compact development board built around the original **ESP32**, featuring an **SX1276** LoRa transceiver, buttons, and a built-in **Li-Ion charging circuit** with a 18650 battery holder (even though we won't use the charging circuit).

![](https://cdn.discordapp.com/attachments/1492625020097396847/1517192272335343786/TBeam.png?ex=6a3562e9&is=6a341169&hm=7163693dc1fe03adae081c1677ceca3c3fb5bdcd8d9a4fc9c683373f7b1320e0&)

The T-Beam already bundles almost everything the project requires on a single board: an ESP32 and a LoRa radio. This let us focus entirely on firmware during the prototyping phase without wiring up the LoRa module on our breadboard.

### What the prototyping phase validated

By the time the PCB was designed, the following had been confirmed working on the T-Beam:

- Opus encode/decode pipeline at `8 kHz`, `10 kbps`, over I2S and ADC
* LoRa TX/RX at SF7, BW 250 kHz, CR 4/5 on 433 MHz
- FreeRTOS task architecture (five concurrent tasks, dual-core assignment)
* Audio RX queue and jitter buffer
- SH1106 OLED via I2C with U8G2
* EC12 quadrature encoder decoding and PTT debouncing
- Vibration motor + FreeRTOS timer for haptic pulses
* TX/RX status LEDs

Essentially, the firmware was feature-complete before the PCB was shipped. The transition from T-Beam to custom board was mostly a matter of re-mapping GPIOs.

---

## PCB Design

With the firmware validated on the T-Beam, the next step was to translate everything into a custom PCB. The constraints were clear: fit the complete circuit onto a board small enough to live inside a handheld enclosure, while keeping the design manufacturable by a standard two-layer process and easy to solder, even with our clumsy hands.

The PCB was designed in **KiCad 10.0** and fabricated by **JLCPCB**.

![](https://cdn.discordapp.com/attachments/1492625020097396847/1517191767898722364/image.png?ex=6a356271&is=6a3410f1&hm=7035bf9ddaff25d82f67c03f73719dc268cbced04939166f064edc130445282b&)

> [!NOTE]
> As of writing, the PCB has not yet been soldered. Any assembly-related issues will be documented here once the board is populated and tested.

### Board overview
 
| Parameter | Value |
|---|---|
| Dimensions | `51.145 mm × 95 mm` |
| Layers | 2 (`F.Cu` + `B.Cu`) |
| Substrate | FR4 |
| Board thickness | `1.6 mm` |
| Copper thickness | `35 µm` |
| Ground planes | Both layers (poured) |
| Mounting holes | 2x M3 |

The board is rectangular and sized to fit inside a handheld enclosure with margin for mounting hardware. Two M3 mounting holes are placed at the corners.

<p align="center">
  <img src="https://cdn.discordapp.com/attachments/1492625020097396847/1517198908844146748/image.png?ex=6a356918&is=6a341798&hm=33b7b466c75d6ce2a334c072f0fcac55f6c00dd6b4300d51ef161ac899beecc7&" width="400" />
  <img src="https://cdn.discordapp.com/attachments/1492625020097396847/1517198950158045384/image.png?ex=6a356921&is=6a3417a1&hm=eb5b2a9e25d2683e57541b5ec31fcf7182307b857ded7bb4e0eb7327177dd2bd&" width="400" />
</p>

### Schematic structure

The schematic is split into two separate sheets:
 
**Sheet 1 - Power (`ihm.kicad_sch`):** Contains the complete power supply chain: Li-Ion charging (**TP4056**), battery protection (**DW01A + FS8205A**), voltage regulation (**AP2112K-3.3**), the Micro USB input connector, the SMA RF connector, the UART programming header, the BOOT and RESET buttons, and the hardware lock switch (`SW_LOCK`).
 
**Sheet 2 - Main (`talk-e.kicad_sch`):** Contains the MCU (**ESP32-S3-WROOM-1**), LoRa module (**RFM96W** based on SX1278), audio amplifier module (**MAX98357A**), microphone (**MAX4466**), rotary encoder, PTT and side buttons, OLED display, LEDs, and vibration motor, along with all their supporting passives.

<p align="center">
  <img src="https://cdn.discordapp.com/attachments/1492625020097396847/1517193844855410829/Capture_decran_du_2026-06-18_17-44-36.png?ex=6a356460&is=6a3412e0&hm=2125095d5c546d730fc88230a82b65554adcfe55a724dd4f3db6b59193be6066&" width="400" />
  <img src="https://cdn.discordapp.com/attachments/1492625020097396847/1517193845350207680/Capture_decran_du_2026-06-18_17-46-28.png?ex=6a356460&is=6a3412e0&hm=4b2de27aa1129e97b502c380c0a3549ff473dabfce83dbb2cac4aeb40782319d&" width="400" />
</p>

> [!NOTE]
> Yes, I know, the filenames are strange, I will update them someday.
 
Global labels tie the two sheets together for power rails (`OUT+`, `+3.3V`) and shared signals (`RESET`, `GPIO0`, `SW_LOCK`, `GPIO_CHRG`, `ANTENNA`, `RXD`, `TXD`).

### Trace widths
 
Three trace width classes are used across the board:
 
| Class | Width (mm) | Width (mil) | Used for |
|---|---|---|---|
| Power | `0.8 mm` | `31.5 mil` | All power rails (`+3.3V`, `OUT+`, `GND`, `B+`) |
| RF (microstrip) | `1.08 mm` | `42.52 mil` | SMA to SX1278 RF path |
| Signal | `0.2 mm` | `7.87 mil` | All logic signals (SPI, I2C, I2S, GPIOs) |

### RF design
 
This is probably the most layout-sensitive part of the board.
 
The **ESP32-S3-WROOM-1** module has an integrated PCB antenna, but we don't use it for LoRa. The module's antenna is only relevant if you use the ESP32's built-in Wi-Fi or Bluetooth. For LoRa, the RF signal goes through a dedicated external path.
 
The RF chain runs as follows: 

```
SMA connector (J1, Amphenol 901-143 horizontal) ➜ 50 Ω microstrip ➜ RFM96W / SX1278 (U6).
```
 
The 50 Ω microstrip was calculated using **KiCad’s built-in transmission line calculator**, using the actual stackup parameters defined in the PCB file: `1.51 mm` FR4 core with εr = `4.5`, `35 µm` copper. This yields a trace width of `1.08 mm` to match a `50 Ω` characteristic impedance.

Two ground keepout zones are placed to protect signal integrity:

- Around the microstrip trace, to exclude the ground plane and maintain the controlled impedance
* Around the **ESP32-S3** module's integrated PCB antenna, to prevent the adjacent copper from detuning it

> [!NOTE]
> Even though it is not in active use, Espressif still recommends placing a keepout zone around the integrated antenna (see [**ESP32-S3-WROOM-1** datasheet](https://documentation.espressif.com/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf)).

> ### What is a microstrip?
> A microstrip is a type of transmission line commonly used on PCBs for RF signals. It consists of a signal trace on the surface of the board, separated from the ground plane beneath it by the PCB substrate. The characteristic impedance depends on the trace width, the substrate thickness, and its dielectric constant (`εr`). Matching this impedance to `50 Ω` (the standard for RF systems) minimises reflections and ensures maximum power transfer between the SMA connector and the radio module.

### Ground planes and via stitching

Both copper layers carry a full ground pour, tied together by a dense array of vias. This was done automatically using the [**viastitching**](https://github.com/weirdgyn/viastitching) KiCad plugin, which distributes stitching vias across the board in a regular pattern.

Via stitching serves two purposes here: it lowers the ground impedance across the board (useful for the audio and power sections), and it improves RF shielding continuity around the LoRa antenna path.

### Module connectors
 
All external modules connect to the PCB via **JST EH series connectors** (2.50 mm pitch), chosen for their polarised, locking contacts — no risk of plugging something in backwards or having it vibrate loose.
 
| Connector | Pins | Module |
|---|---|---|
| `J6` | 2 | Right button |
| `J7` | 2 | Left button |
| `J9` | 2 | Battery |
| `J10` | 2 | TX LED |
| `J12` | 2 | CHRG LED |
| `J13` | 2 | RX LED |
| `J5` | 2 | PTT |
| `J2` | 2 | Vibration motor |
| `J8` | 3 | Lock switch |
| `J11` | 3 | Microphone (MAX4466) |
| `J3` | 4 | OLED display |
| `J4` | 4 | Encoder |

The **MAX98357A audio amplifier** (Adafruit breakout module, footprint `3006`) is an exception: it is soldered directly onto the PCB footprint rather than via a detachable connector, as it doesn't need a periodic replacement or disconnection.

### Power input

The board charges via **Micro USB** (`J15`, Molex 47590-0001). The USB connector feeds `VBUS` into the **TP4056** charge controller. No USB data lines are used; the connector is power-only.

A separate **UART programming header** (`PROG`, 2×4 pin header) exposes `TXD`, `RXD`, `GPIO0`, and `RESET` for flashing via a USB-to-UART adapter. The onboard `BOOT` and `RESET` push buttons allow entering the ESP32 bootloader.

### Some pictures of the board

<p align="center">
  <img src="https://cdn.discordapp.com/attachments/1492625020097396847/1517197083168473219/image.jpg?ex=6a356764&is=6a3415e4&hm=adf3a5fbe41720b66ebac3d0635e1cce8ef5ea45e198a3ade26f037838700753&" width="400" />
  <img src="https://cdn.discordapp.com/attachments/1492625020097396847/1517197083591835648/image2.jpg?ex=6a356764&is=6a3415e4&hm=44752ddcd8a163dc332a1e7e332c8a79c3923959e4ed2a524fa49629e8cf051b&" width="400" />
</p>

---

## Next steps

- Assemble boards
- Validate RF performance and audio quality
- Measure real-world power consumption and autonomy
