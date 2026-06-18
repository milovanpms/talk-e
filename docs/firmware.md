# 🛠️ Firmware

## Overview

The firmware is built on **ESP-IDF** (Espressif IoT Development Framework) and uses **FreeRTOS** as its real-time operating system.

> ### What is FreeRTOS?
> FreeRTOS is a lightweight real-time operating system designed for microcontrollers. It lets you run multiple independent tasks concurrently, each with its own stack, priority, and scheduling behaviour. On the **ESP32-S3**, FreeRTOS runs on both cores simultaneously, which means two tasks can execute in parallel.

The firmware is split into five independent tasks that communicate through shared variables and FreeRTOS queues.

## Task Architecture

### Task table

| Name | Function | Core | Priority | Stack | Role |
|------|----------|:----:|:--------:|------:|------|
| `enc` | `task_encodeur` | 1 | 5 | 4,096 B | Encoder polling, PTT, channel button |
| `task_lora` | `task_lora` | * | 4 | 2,048 B | LoRa RX/TX state machine |
| `audio_rx` | `audio_rx_task` | 0 | 5 | 8,192 B | Opus decode ➜ jitter buffer ➜ I2S |
| `disp` | `task_display` | 0 | 3 | 4,096 B | OLED display refresh |
| `audio_tx` | `audio_tx_task` | 0 | 3 | 16,384 B | ADC acquire ➜ Opus encode ➜ LoRa TX |

Regarding priorities and core assignments, we tried multiple configurations and we found this one to be the most stable.

### Dual-core assignment

`task_encodeur` runs exclusively on **Core 1**. All other tasks run on **Core 0**.

This isolation is intentional. The encoder task runs at 5 ms intervals and must stay responsive at all times for accurate quadrature decoding and PTT detection. Pinning it to **Core 1** means it is never preempted by the audio or display tasks running on **Core 0**, which is especially important given that `audio_tx_task` has a large 16 KB stack and does Opus compression work.

### Inter-task communication

| Producer | Consumer | Mechanism | Purpose |
|----------|----------|-----------|---------|
| `task_lora` | `audio_rx_task` | `audio_rx_queue` (32 slots) | Pass received LoRa packets to the decoder |
| `task_encodeur` | `task_lora` | `radio_state` (volatile + mutex) | Signal TX start/stop via PTT |
| `task_encodeur` | `task_display` | `volume`, `channel`, `current_mode` (volatile) | Drive display updates |
| `task_lora` | `task_display` | `radio_state` (volatile, read-only) | Show IDLE / TX / RX status |

`radio_state` is written through a `RADIO_STATE_SET()` macro that takes a mutex, since both `task_encodeur` (writes `RADIO_TX`) and `task_lora` (writes `RADIO_RX` / `RADIO_IDLE`) can update it concurrently.

> ### What is a mutex?
> A mutex (mutual exclusion) is a synchronization mechanism used to protect shared resources from being accessed by multiple tasks at the same time. In FreeRTOS, a mutex ensures that only one task can access a critical section of code or a shared peripheral (such as UART, SPI, or I2C) at any given moment. If another task tries to acquire the mutex while it is already locked, it will be blocked until the mutex is released.

## Initialisation Sequence

Everything starts in `app_main()`. The initialisation order matters: peripherals must be ready before the tasks that use them are created.

**1. I2C bus:** The display communicates via I2C. The bus is created first since the [**U8G2**](https://github.com/olikraus/u8g2) library needs it immediately during display init.

**2. SH1106 display:** **U8G2** is initialized with the custom I2C callbacks, the display is powered on and cleared.

**3. EC12 encoder GPIOs:** Pins A, B, and SW configured as inputs with pull-ups.

**4. Channel button GPIO:** Dedicated button for mode switching.

**5. Vibration motor + timer:** GPIO output configured, 50 ms FreeRTOS timer created, motor forced off.

**6. LoRa LEDs:** TX and RX indicator GPIO outputs initialized and switched off.

**7. LoRa module:** `lora_init()` followed by the full RF configuration (explicit header mode, 10 dBm, 250 kHz, 433 MHz, SF7, CR 4/5, CRC enabled). The module is fully configured before any LoRa task starts.

**8. Core task creation:** `task_encodeur` (Core 1), `task_display` (Core 0), `task_lora` (any core). The LoRa application layer (`lora_app_init`) is called from inside `task_lora` on first run.

**9. I2S peripheral init:** Audio output configured for the **MAX98357A** amplifier.

**10. ADC peripheral init:** Continuous ADC driver started for the **MAX4466**.

**11. Audio task creation:** `audio_tx_task` and `audio_rx_task` created last, once the peripherals they depend on are ready.

## Watchdog

`task_encodeur` registers itself with the ESP-IDF task watchdog (`esp_task_wdt_add`) and calls `esp_task_wdt_reset()` at every polling cycle. If the task ever hangs, the watchdog will trigger a system reset. Given that this task also handles the PTT, a lockup here would leave the radio stuck in TX, which is exactly the kind of failure the watchdog is there to catch.

## Human-Machine Interface

See [HMI documentation](./hmi.md).

## Known Limitations

**`display_settings.c` is a stub.** The settings menu exists as a function that only draws a border frame (currently).

**Channel change is rejected during RX.** If the user tries to change channel while receiving a message, the change is dropped. There is currently no visual feedback to indicate this to the user.

