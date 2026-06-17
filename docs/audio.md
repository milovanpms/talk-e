# 🎙️ Audio Processing

## A voice's journey through the system

The audio pipeline handles everything between the microphone and the antenna on one side, and between the antenna and the speaker on the other. Everything runs at `8 kHz`, which is telephony-grade quality; not a hi-fi *Pro Max Plus* sound, but *theoretically* perfectly intelligible for a walkie-talkie.

The pipeline splits into two independent paths:

**TX Path (transmit):**
```
MAX4466 ➜ ADC @ 40 kHz ➜ DC-block ➜ Decimation /5 ➜ AGC ➜ PCM (16-bit @ 8 kHz) ➜ Opus encode ➜ LoRa TX
```

**RX Path (receive):**
```
LoRa RX ➜ audio_rx_queue ➜ Opus decode ➜ Volume gain ➜ Jitter buffer ➜ I2S ➜ MAX98357A
```

Both paths run as separate FreeRTOS tasks (`audio_tx_task` and `audio_rx_task`) and communicate with the radio layer through a shared queue (`audio_rx_queue`).

> ### What is PCM?
> PCM (Pulse Code Modulation) is the standard way of representing analog audio as digital data. Each audio sample is stored as a numerical value corresponding to the instantaneous amplitude of the sound wave.
>
> In our system, the microphone signal is converted into 16-bit signed PCM samples at `8 kHz`, meaning:
> - 8,000 samples per second are taken;
> * Each sample is represented by a 16-bit integer (`-32,768` to `32,767`);
> - And these raw samples are then passed to the Opus encoder for compression.

## Audio Acquisition

### The Microphone: MAX4466

The **MAX4466** is an analog electret microphone amplifier. It outputs an analog voltage that varies with the sound pressure level. This analog signal is read by **ESP32-S3**'s internal ADC.

### ADC Configuration

Rather than sampling the microphone at the final `8 kHz` target rate, we run the ADC at `40 kHz` and then decimate. This approach is less sensitive to aliasing and fits naturally with the ESP-IDF continuous ADC driver, which is more reliable at higher rates.

| Parameter | Value |
|---|---|
| ADC unit | `ADC1` |
| Channel | `ADC1_CH0` (GPIO36) |
| Bit width | `12 bits` (values from 0 to 4095) |
| Attenuation | `12 dB` (allows reading voltages up to ~3.1 V) |
| Sample rate | `40,000 Hz` |
| Samples per DMA read | `256` |

> #### Why the hell did you take an analog microphone module?
> Because that's what we had in the parts bin.
> 
> More seriously, the MAX4466 was simply the microphone module we already had available on campus when the project started. On top of that, it could be connected directly to the **ESP32-S3**'s internal ADC with minimal effort.
>
> Looking back, we probably underestimated the impact of the microphone choice on the overall audio chain. We did not spend much time evaluating microphone options early in the project and only later realised how significant the difference between a basic analog module and a modern digital microphone could be.
>
> If we were to redesign the system today, we would seriously consider using a digital I2S microphone instead. Besides simplifying the acquisition stage, it would likely provide cleaner audio and avoid some of the limitations and noise introduced by the **ESP32**'s internal ADC.

The ADC runs in **continuous mode with DMA** (Direct Memory Access). DMA is a hardware mechanism that copies data from the ADC directly into a memory buffer without involving the CPU; this means the processor doesn't waste cycles waiting for each individual sample, and can do other things while the ADC fills the buffer in the background.

### Decimation /5 (40 kHz ➜ 8 kHz)

> ### What is decimation?
> It's a signal processing technique that reduces the sampling rate of a digital signal. Instead of keeping every sample, we group them and compute an average, which produces fewer samples per second.

We accumulate 5 consecutive ADC samples and average them into a single output sample. This divides the effective sample rate by 5: `40 kHz ÷ 5 = 8 kHz`. The result is a standard telephony sample rate, well supported by Opus.

## Signal Pre-Processing

Before the PCM samples reach the Opus encoder, two processing steps clean up the signal.

### DC-Block Filter

The **MAX4466** output sits around a DC bias point rather than around zero. If left uncorrected, this constant offset would be interpreted as a very low-frequency audio signal and would waste bitrate in the encoder.

A simple IIR high-pass filter removes this offset:

```c
dc_blocked = raw - prev_raw + 0.995f * prev_dc;
```

> ### How does the filter work?
> It's a first-order high-pass filter. It lets audio frequencies pass through while blocking the constant DC component. The coefficient `0.995` controls how aggressively low frequencies are attenuated; values close to `1.0` give a very low cutoff frequency, which is what we want (we only want to remove DC, not attenuate speech).

### Automatic Gain Control (AGC)

Speech level varies a lot; someone whispering vs. shouting at the microphone produces signals of very different amplitudes. Without gain control, quiet speech would be barely audible after transmission, while loud speech would saturate.

The AGC dynamically adjusts a gain multiplier:

| Parameter | Value | Role |
|---|---|---|
| Fixed pre-gain | `×28.0` | Scales the 12-bit ADC range to 16-bit PCM range |
| Threshold | `16,000` (out of 32 767) | Above this, gain is reduced to prevent clipping |
| Attack | `0.05` | How quickly gain decreases when signal is too loud |
| Release | `0.005` | How slowly gain recovers when signal calms down |

The asymmetry between attack and release is intentional: gain is reduced quickly to avoid clipping, but recovered slowly to avoid pumping artefacts.

## Compression with Opus

### What is Opus?

**Opus** is an open audio codec developed by the IETF, widely used for real-time voice and audio communications (WebRTC, Discord, VoIP systems). It supports bitrates from `6 kbit/s` to `510 kbit/s` and is quite efficient at low bitrates for speech. 

### Why Opus and not something else?

Opus offers better quality at lower bitrates than other speech codecs (like **G.711**, **G.726** or **G.729**), is actively maintained, has a clean C API, and has an official ESP-IDF component available. It was the obvious choice.

### Encoder configuration

The PCM frames produced by the acquisition pipeline are fed into the Opus encoder 80 samples at a time.

> ### What is a frame?
> Opus, like most audio codecs, doesn't process one sample at a time. It processes a fixed-size block of samples (a "frame") and outputs a compressed packet. This block size directly controls latency.

We settled these settings for the encoder:

| Parameter | Value | Rationale |
|---|---|---|
| Sample rate | `8 000 Hz` | Telephony grade, sufficient for speech |
| Channels | `1` (mono) | We only have one microphone |
| Application | `VOIP` / `RESTRICTED_LOWDELAY` | Optimised for real-time voice, minimal buffering |
| Bitrate | `10,000 bit/s` | Fits comfortably within the LoRa `~10.9 kbit/s` budget |
| Complexity | `1` (out of 10) | Minimum CPU cost — the ESP32-S3 has other tasks to run |
| Signal type | `OPUS_SIGNAL_VOICE` | Tells the encoder to use voice-optimised coding decisions |
| Bandwidth | `NARROWBAND` (0 – 4 kHz) | Telephony bandwidth — further reduces encoded size |
| Frame size | `80 samples` | 80 samples @ 8 kHz = **10 ms per frame** |

With these settings, each Opus packet is typically a few tens of bytes, well within the **240 byte** SX1278 payload limit defined in `lora_app.h`.

### Latency budget

The 10 ms frame size means the encoder introduces a minimum of 10 ms of algorithmic latency per direction. In practice, the total one-way latency also includes LoRa airtime, the jitter buffer on the receiver side (40 ms, see below), and FreeRTOS scheduling. The overall latency is acceptable for a push-to-talk device; it's not a phone call, and users naturally wait for the other person to finish before replying.

> ### Approximate one-way latency
> - **Opus frame accumulation:** 10 ms
> - **LoRa airtime:** Typically a few tens of milliseconds
> - **Jitter buffer:** 40 ms
>
> **Total:** ~70 ms

## Memory: PSRAM and the role of the ESP32-S3

> ### What is PSRAM?
> PSRAM (Pseudo-Static RAM) is additional RAM connected to the microcontroller via the SPI bus. On the **ESP32-S3-WROOM-1-N16R8**, this is an external 8 MB chip soldered on the module itself. It's slower than internal RAM but vastly larger; the internal RAM of the **S3** is only a few hundred kilobytes.

The Opus codec requires a significant amount of memory for its internal state, lookup tables, and processing stack. On most embedded systems, this is a problem. Here, we offload it to PSRAM via ESP-IDF's menuconfig:

- **SPI RAM support** is enabled under `Component config → ESP PSRAM`.
* The access method is set so that PSRAM is available both via standard `malloc()` and via `heap_caps_malloc(..., MALLOC_CAP_SPIRAM)`.
- Under `Component config ➜ Opus Audio Codec`, the **memory preference** is set to *"Prefer PSRAM, fallback to internal RAM"*, which causes the Opus encoder and decoder state to be allocated in PSRAM automatically.

The ADC DMA buffers, by contrast, **must** stay in internal RAM. DMA controllers on the **ESP32-S3** cannot address PSRAM directly, which is why `audio_tx_task` allocates its ADC buffer with `MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL`.

This is the primary reason we chose the **S3** variant specifically: it's a common ESP32 module with integrated PSRAM, and without it, the Opus encoder simply would not fit in memory alongside the rest of the firmware.

## Reception Path

### Queue between tasks

The `task_lora` task receives raw LoRa packets and pushes them into `audio_rx_queue`, a FreeRTOS queue of 32 slots, each holding a complete `audio_pkt_t` (data buffer + actual length). The `audio_rx_task` reads from this queue and handles decoding.

If the queue fills up (e.g. the decoder is temporarily slow), incoming packets are silently dropped; this is intentional. Dropping a packet causes a brief audio gap, which is far preferable to blocking `task_lora` and causing it to miss subsequent LoRa frames entirely.

### Opus decoding

Each packet received from the queue is decoded by a global `OpusDecoder` instance (created once at startup) back into 80 PCM samples at `8 kHz`.

### Volume control

After decoding, a linear gain is applied to every sample before playback:

```c
float vol_gain = (volume / 100.0f) * 2.0f;  // 0 to 200%
```

The `volume` variable (0-100) is controlled by the rotary encoder on the device. The gain range of `0x` to `2x` allows the user to amplify the decoded signal up to twice its original level, useful in noisy environments.

Samples are clamped to the 16-bit signed range (`−32,768` to `32,767`) after gain is applied to prevent saturating artefacts.

### Jitter buffer

> ### What is jitter?
> In a radio link, consecutive packets don't always arrive with perfectly regular spacing. Some arrive a few milliseconds early, others late, due to radio channel variations and scheduling. This irregular timing is called jitter.

A jitter buffer absorbs this variability by storing a few decoded frames before they are played, and reading them out at a steady rate. Ours holds 4 frames:

| Parameter | Value |
|---|---|
| Buffer depth | `4 frames` |
| Frame duration | `10 ms` |
| Total buffering | `40 ms` |

This 40 ms window is the maximum jitter the system can absorb without audible gaps. It also adds 40 ms of latency on the receive side; a reasonable trade-off for a walkie-talkie.

If the buffer is empty (no packets received), the task simply waits without writing anything to I2S, which causes the **MAX98357A** to output silence. This is also what happens when corrupted packets are discarded by the **SX1278** CRC checker (see [radio documentation](./radio.md)).

### I2S output to MAX98357A

> ### What is I2S?
> I2S (Inter-IC Sound) is a digital communication protocol specifically designed for transporting audio data between integrated circuits. Unlike analog audio signals, I2S transmits audio as a stream of digital samples synchronised by dedicated clock signals.
>
> In our system, the ESP32-S3 sends the decoded PCM samples to the **MAX98357A** amplifier over I2S. The amplifier receives the digital audio stream, converts it internally to an analog signal, and drives the speaker.
>
> Using I2S avoids the noise and quality issues associated with transmitting low-level analog audio signals across the PCB.

> [!NOTE]
> Although I2S supports stereo audio, our implementation only uses a single mono channel.

The decoded and volume-adjusted PCM samples are written to the **MAX98357A** amplifier via I2S:

| Parameter | Value |
|---|---|
| I2S peripheral | `I2S_NUM_1` |
| Sample rate | `8 000 Hz` |
| Bit depth | `16 bits` |
| Mode | Mono, Philips standard |
| DMA descriptors | `8` |
| DMA frame size | `256 samples` |

## Known Limitations

**Audio quality** is telephony-grade. The combination of 8 kHz bandwidth, Opus narrowband mode, and 10 kbps bitrate produces intelligible but not crystal-clear speech. This was a trade-off to stay within the LoRa throughput budget.

**Encoder complexity is at minimum.** Setting `OPUS_SET_COMPLEXITY(1)` keeps CPU usage low but produces slightly less efficient compression than higher complexity levels. At `10 kbps` over a narrowband signal this might not be noticeable in practice.
