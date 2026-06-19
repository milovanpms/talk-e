# ⚙️ Hardware Architecture

## Overview
The architecture is built around an **ESP32-S3** (**ESP32-S3-WROOM-1-N16R8**) that handles everything: radio via SPI, 
display via I2C, audio via I2S and GPIOs for buttons and feedback.

### Why specifically the S3?
We picked an **S3** module for its built-in PSRAM, which we needed for audio buffering. See the ![corresponding section on audio processing](./firmware.md#audio-processing).

### Why did you choose the module with the integrated antenna?
We took the **ESP32-S3-WROOM-1** (integrated PCB antenna) instead of the **ESP32-S3-WROOM-1U**, even though we don't need the built-in `2.4 GHz` antenna. The main reason was that we already had several **ESP32-S3-WROOM-1** modules available on campus; but it would also work with the module without an antenna!

## Functional synoptic diagram
![](https://cdn.discordapp.com/attachments/1492625020097396847/1516416328632635483/image.png?ex=6a329042&is=6a313ec2&hm=379b1b4ad2e43e1ddad577d74de03030aa0c9ba9d64bd04e83b98d7f257038fc&)

## Component Details

| Block | Component | Role |
|---|---|---|
| Microcontroller | **ESP32-S3** | Main processing, SPI/I2C/I2S |
| Radio | **SX1278** | LoRa transceiver, `433 MHz` |
| Audio input | **MAX4466** | Analog microphone |
| Audio output | **MAX98357A** | Class D amplifier |
| Display | **SH1106 OLED 128x64** | Status display |
| Battery | **Li-Ion 3.7V 2000mAh** | Power source |
| Charging | **TP4056** | Charge controller |
| Protection | **DW01A + FS8205A** | Overvoltage, overdischarge, short-circuit |
| Regulation | **AMS1117-3.3** | `3.3 V` |
| Vibration | Vibration motor | Haptic feedback |

## Pin Mapping

As you can see, the GPIOs are nearly fully used!

| Component       | Pin          | ESP32-S3 GPIO | Interface |
|-----------------|--------------|---------------|-----------|
| **SX1278**      | `MOSI`       | `GPIO11`      | SPI       |
| **SX1278**      | `MISO`       | `GPIO13`      | SPI       |
| **SX1278**      | `CS`         | `GPIO10`      | SPI       |
| **SX1278**      | `DIO0`       | `GPIO9`       | SPI       |
| **SX1278**      | `RST`        | `GPIO8`       | SPI       |
| **SX1278**      | `SCK`        | `GPIO12`      | SPI       |
| **SH1106**      | `SDA`        | `GPIO4`       | I2C       |
| **SH1106**      | `SCL`        | `GPIO5`       | I2C       |
| **MAX98357A**   | `BCLK`       | `GPIO39`      | I2S       |
| **MAX98357A**   | `LRCLK`      | `GPIO40`      | I2S       |
| **MAX98357A**   | `DOUT`       | `GPIO41`      | I2S       |
| **MAX98357A**   | `SD`         | `GPIO42`      | I2S       |
| **MAX4466**     | -            | `GPIO14`      | ADC       |
| **Encoder**     | `A`          | `GPIO1`       | GPIO      |
| **Encoder**     | `B`          | `GPIO2`       | GPIO      |
| **Encoder**     | `SW`         | `GPIO3`       | GPIO      |
| **PTT button**  | -            | `GPIO6`       | GPIO      |
| **Button 1**    | -            | `GPIO7`       | GPIO      |
| **Button 2**    | -            | `GPIO14`      | GPIO      |
| **Vibration motor** | -        | `GPIO17`      | GPIO      |
| **Lock switch** | -            | `GPIO18`      | GPIO      |
| **TX LED**      | -            | `GPIO38`      | GPIO      |
| **RX LED**      | -            | `GPIO21`      | GPIO      |
| **CHRG LED**    | -            | `GPIO13`      | GPIO      |
| **RESET button**| `EN`         | -             | GPIO      |
| **BOOT button** | -            | `GPIO0`       | GPIO      |
