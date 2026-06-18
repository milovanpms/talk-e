# 👤 Human-Machine Interface

From the beginning, we wanted Talk-E to feel like a finished product; not a dev board with DuPont wires *hanging out like cobwebs*. The kind of device you pick up, understand in ten seconds, and use without using a manual.

The interface is **deliberately minimal**: one rotary encoder, one PTT button, one channel button, one lock switch, and a small OLED display. No touchscreen, no menu tree to navigate, no deep configuration. Every interaction has immediate physical and visual feedback: the encoder clicks at each detent, the motor confirms your input, the display reacts instantly.

## Rotary Encoder (EC12)

The encoder has three electrical signals: **A**, **B**, and **SW** (push button used as PTT).

### Quadrature decoding

A rotary encoder outputs two square waves (A and B) that are 90° out of phase. The direction of rotation depends on which signal leads the other.

Rather than using interrupts, we decode the encoder by polling every 5 ms. This avoids the false transitions caused by contact bounce; a common issue with mechanical encoders. Hardware-side, 100 nF capacitors between A/GND and B/GND filter out electrical noise.

Each polling cycle, A and B are read as 2-bit state (`00`, `01`, `10`, or `11`). Combining the previous state with the current state gives a 4-bit index into a lookup table:

```c
static const int8_t quad_table[16] = {
         0,  -1,  1,  0,   /* prev = 00 */
         1,   0,  0, -1,   /* prev = 01 */
        -1,   0,  0,  1,   /* prev = 10 */
         0,   1, -1,  0    /* prev = 11 */
};
```

Each valid transition returns `+1` (clockwise) or `-1` (counter-clockwise). Invalid transitions (caused by noise or missed edges) return `0` and are ignored.

Because the EC12 has **4 quadrature transitions per mechanical detent**, transitions are accumulated in a counter. Only when the accumulator reaches ±4 is a single click credited and a vibration pulse fired. This prevents accidental double-counts on a single detent.

### Volume and channel modes

The encoder operates in two modes:

| Mode | Encoder action | Effect |
|------|---------------|--------|
| `MODE_VOLUME` | Rotate | Adjust volume (0 to 100, step 5) |
| `MODE_CHANNEL` | Rotate | Cycle through 8 channels (wrap-around) |

Pressing the **channel button** (GPIO38, separate from the encoder push) toggles between the two modes.

Both modes have an inactivity timeout after which the display returns to its idle mode:

| Mode | Timeout | Behaviour |
|------|---------|-----------|
| `MODE_VOLUME` | 1.6 s | Volume bar is hidden after 1.6 s without rotation |
| `MODE_CHANNEL` | 3 s | Returns to `MODE_VOLUME` after 3 s without rotation |

The timer resets on every encoder interaction, ensuring it only triggers after genuine inactivity.

### Push-to-Talk (PTT)

The PTT function is mapped to the encoder's push button (**SW**, GPIO2).

Debouncing is done in software by counting how many consecutive 5 ms polls agree on the button state. A press is registered only after **10 consecutive stable low readings** (50 ms of stable signal).

- **Press detected:** calls `lora_start_tx()` ➜ radio switches to TX, LED lights up.
- **Release detected:** calls `lora_stop_tx()` ➜ radio returns to IDLE, reception re-armed.

The PTT is fully disabled when the lock switch is active.

### Channel Button

A dedicated push button (GPIO38) handles mode switching. It uses the same software debouncing approach as the PTT: 50 ms stability threshold, 1 ms polling.

Only falling edges (button press, not release) are registered.

### Lock Switch

A physical slide switch (GPIO15, active low) locks the device controls.

When active, `task_encodeur` skips all encoder decoding, PTT detection, and channel button polling. The display shows a stop sign icon. This prevents accidental channel changes or transmissions when the device is pocketed.

### Channel Map

8 channels are defined, covering the 433 MHz and adjacent bands:

| Channel | Frequency |
|---------|-----------|
| CH1 | 433.050 MHz |
| CH2 | 434.100 MHz |
| CH3 | 434.700 MHz |
| CH4 | 435.300 MHz |
| CH5 | 435.900 MHz |
| CH6 | 436.500 MHz |
| CH7 | 437.100 MHz |
| CH8 | 437.700 MHz |

Channels 4 through 8 fall outside the European LPD band (`433.05–434.79 MHz`). They are available for ~~fun~~ laboratory testing.

When the user selects a new channel in `MODE_CHANNEL`, `channel_apply_frequency()` immediately calls `lora_set_frequency()` to update the SX1278, provided the radio is in `RADIO_IDLE` state.

---

## Display

The **SH1106** OLED (`128x64` pixels) is driven by the [**U8G2**](https://github.com/olikraus/u8g2) library.

`task_display` refreshes the screen every 20 ms (up to 50 FPS). Each cycle, it reads the current encoder values, checks the radio state, and redraws the full frame buffer before calling `u8g2_SendBuffer()`.

### Volume mode (default)

The main screen, active most of the time:

- **Top bar:** channel icon (Siji font), stop sign icon if locked, signal strength icon, battery icon.
- **Center:** status text in `doomalpha04` font.
  - `"Disponible"` when idle.
  - `"Transmission..."` when transmitting, with animated dots cycling through `""` → `"."` → `".."` → `"..."` every 250 ms.
  - `"Acquisition..."` when receiving, same animation.
- **Bottom:** rounded volume bar with a matching icon (muted / low / medium / high), visible for 1.6 s after the last volume interaction.

The volume bar is drawn by a custom function (`drawRoundedVolumeBar()`) that handles both the fill level and the rounded end caps using U8G2 circle and rectangle primitives.

### Channel mode

Shown when the user switches to `MODE_CHANNEL`:

- **Center:** channel icon rendered at ×2 size.
- **Below:** the channel frequency in `bpixeldouble` font.
- **Bottom:** `"Choix du canal"` subtitle.

Automatically reverts to volume mode after 3 s of inactivity.

---

## Feedback

### LEDs

Two indicator LEDs signal the radio state:

| LED | GPIO | State |
|-----|------|-------|
| TX | `GPIO0` | ON while transmitting |
| RX | `GPIO4` | ON during an active reception session |

Both are off in `RADIO_IDLE`. They are driven directly by `lora_led_tx()` and `lora_led_rx()` called from `lora_app.c` on state transitions.

### Vibration Motor

The vibration motor provides haptic confirmation of user interactions. It fires a **50 ms pulse** in two situations:

- Each encoder detent (clockwise or counter-clockwise).
- PTT button press.

The pulse is implemented with a non-blocking FreeRTOS timer: `vib_pulse()` switches the GPIO high and resets the timer; the timer callback switches it low 50 ms later. This means the encoder task is never blocked waiting for the motor to finish.

If multiple pulses are requested within 50 ms (fast rotation), the timer is simply reset each time, extending the motor-on time slightly rather than queuing up separate pulses.
