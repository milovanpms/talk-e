<h1 align="center">Talk-E</h1>

<h3 align="center">Talk-E is a walkie-talkie powered by LoRa and an ESP32-S3, operating on the 433 MHz band. And it mostly works.</h3>

<p align="center">
  <img src="https://img.shields.io/badge/status-in%20progress-yellow"/>
  <img src="https://img.shields.io/badge/platform-ESP32--S3-blue"/>
  <img src="https://img.shields.io/badge/framework-ESP--IDF-red"/>
  <img src="https://img.shields.io/badge/language-C-lightgrey"/>
  <img src="https://img.shields.io/badge/protocol-LoRa-green"/>
  <img src="https://img.shields.io/badge/hardware-KiCad%2010-orange"/>
</p>

<h5 align="center"><a href="https://milovan.me">milovan.me</a></h5>

## In a nutshell
- **What** : Two identical devices, one button to talk, and that's it.
- **Why** : We didn't have much time, so we wanted something original, interactive, and fun to demo. The kind of project you understand immediately just by holding it in your hands.
- **Context** : Radiocommunication student project, two-person team.

## Documentation

The documentation is quite detailed, so each part has its own page instead of having it all here.

### Find your happiness:
- ⚙️ [Hardware Architecture](./docs/architecture.md): Functional synoptic diagram and component details
- 🛠️ [Tech Stack](./docs/stack.md): Firmware, PCB and case tools
- 📡 [Radio Parameters](./docs/radio.md): How we use LoRa
- 👤 [Human-Machine Interface](./docs/hmi.md): How it works for the user
- 🔋 [Power Supply Circuit](./docs/power.md): Power management and energy consumption
- 🛠️ [Firmware](./docs/firmware.md): ESP-IDF, libraries, code architecture
- 🔌 [PCB](./docs/pcb.md): KiCad, routing rules, our conception choices
- 📐 [CAO](./docs/cao.md): Our vision of a case for this device

## Repository Structure 
```
├── hardware/        # KiCad schematics, PCB
├── firmware/        # ESP-IDF code
├── docs/            # Documentation, conception notes
├── media/           # Images, schematics and illustrations
└── case/            # 3D models
```

## Who did this ?
- Thomas Pineau - [@thomaspineau85](https://github.com/thomaspineau85)
- Milovan PMS - [@milovanpms](https://github.com/milovanpms)
