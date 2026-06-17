# 📡 Radio Parameters

## Why LoRa and why 433 MHz

Several wireless technologies were considered during the design phase. Traditional modulation schemes such as FSK can provide higher data rates, while technologies like Wi-Fi or Bluetooth offer much greater throughput. However, our primary objective was not audio quality but communication range, robustness, and low hardware complexity.

LoRa is specifically designed for long-range, low-power communications and provides exceptional receiver sensitivity compared to conventional narrowband radio systems, such as FSK. Although this comes at the cost of reduced throughput, our calculations showed that the available bitrate remains sufficient for heavily compressed voice transmission. This makes LoRa a good compromise between range, reliability and implementation complexity for a digital walkie-talkie.

In short, we deliberately sacrificed audio quality and throughput in exchange for communication range and link robustness.

The `433 MHz` (UHF) band allows for free public usage (LPD band) under specific conditions. In Europe (at least in France), the regulation covers `433.05 MHz` to `434.79 MHz`, split into 69 channels with a `25 kHz` spacing.

> ### Legal Constraints
> The maximum allowed transmission power is `10 mW` with a maximum duty cycle of $\alpha = 0.10\%$ (meaning 6 minutes of cumulative transmission per hour). If you hold the PTT button for a `10 s` message, you must legally wait `90 s` before transmitting again. We plan to implement a software *cooldown mode* to prevent penalties (though we also secretly planned to bake in a *law-bypass mode* for ~~curiosity~~ laboratory testing).

## RF Parameters

To transmit compressed audio frames successfully, we need a balance between range and throughput. Based on Excel simulations of different scenarios, we settled on the following settings:

> [!NOTE]
> <details>
> <summary><strong>Carrier Frequency:</strong> <code>433.050 MHz</code> to <code>434.750 MHz</code> (Configurable via UI)</summary>
> <br>
> The radio frequency used for transmission. All communicating devices must be configured to the same frequency (or channel) to exchange data successfully.
>
> </details>
>
> <details>
> <summary><strong>Spreading Factor (SF):</strong> <code>SF7</code></summary>
> <br>
> Controls the trade-off between throughput and range. A lower SF increases data rate and reduces airtime, while a higher SF improves receiver sensitivity and communication range.
>
> </details>
>
> <details>
> <summary><strong>Bandwidth (BW):</strong> <code>250 kHz</code></summary>
> <br>
> Defines the width of the radio channel. Higher bandwidth increases throughput and reduces transmission time, but generally decreases receiver sensitivity.
>
> </details>
>
> <details>
> <summary><strong>Coding Rate (CR):</strong> <code>4/5</code></summary>
> <br>
> Determines the amount of error correction added to transmitted data. Lower redundancy improves throughput, while higher redundancy increases resistance to interference and packet loss.
>
> </details>

### Tested Configurations

| SF  | BW (kHz) | CR | Theoretical Bitrate (kbit/s) | Expected Range | Comments |
|-----|----------|----|-----------------------------|----------------|----------|
| 7   | 125      | 4/5 | 5.47  | Short | Low range, good robustness margin |
| 7   | 250      | 4/5 | 10.94 | Medium | Best trade-off for voice transmission |
| 8   | 125      | 4/5 | 3.13  | Medium | Balanced configuration |
| 8   | 250      | 4/5 | 6.25  | Medium | Slightly improved throughput |
| 9   | 125      | 4/5 | 1.76  | Long | Good range, limited audio quality |
| 9   | 250      | 4/5 | 3.52  | Long | Stable long-range voice link |
| 10  | 125      | 4/5 | 0.98  | Very long | Marginal for real-time voice |
| 10  | 250      | 4/5 | 1.95  | Long | Only usable with aggressive compression |
| 11  | 125      | 4/5 | 0.54  | Very long | Near limit for intelligible speech |
| 11  | 250      | 4/5 | 1.10  | Very long | Only emergency/low quality voice |
| 12  | 125      | 4/5 | 0.29  | Extreme | Practically unusable for voice |
| 12  | 250      | 4/5 | 0.58  | Extreme | Voice transmission not feasible |

### Throughput Analysis

To validate our radio settings before implementation, we used the theoretical LoRa useful bitrate formula:

$$
R_b = SF \times \frac{BW}{2^{SF}} \times \frac{4}{4 + CR}
$$

Where:

- `SF` is the Spreading Factor
* `BW` is the channel bandwidth in Hz
- `CR` is the Coding Rate parameter (`1` for 4/5, `2` for 4/6, etc.)

Using our selected configuration (`SF7`, `BW = 250 kHz`, `CR = 1`, corresponding to a coding rate of `4/5`), we obtain:

$$
R_b = 7 \times \frac{250000}{2^7} \times \frac{4}{5}
\approx 10.9\ \text{kbit/s}
$$

This theoretical throughput was used during Excel simulations to evaluate whether compressed audio frames could be transmitted fast enough while maintaining acceptable communication range. The actual application-level throughput is slightly lower due to LoRa overhead (preamble, headers, CRC) and our custom protocol headers.

### From Radio Throughput to Voice Transmission

The estimated useful bitrate of approximately `10.9 kbit/s` gives us a theoretical upper bound for real-time audio transmission. Since human speech remains intelligible at relatively low bitrates when compressed efficiently, this throughput is sufficient for a digital walkie-talkie application.

Modern speech codecs such as Opus can produce understandable voice streams at bitrates ranging from roughly `6 kbit/s` to `12 kbit/s`, depending on the desired quality and latency. This makes our selected LoRa configuration viable for transmitting compressed voice while maintaining acceptable communication range.

The audio acquisition, compression strategy and resulting bitrate requirements are discussed in detail in the [audio documentation](./audio.md).

## Custom Protocol
Because LoRa is fundamentally half-duplex (and so are basic walkie-talkies), our system cannot send and receive sound at the same time. Therefore, we managed to implement clean tasks, to properly handle this constraint.

### Frame Format
Every packet contains a minimal overhead header preceding the compressed audio payload:

| Field | Size | Description |
|---|---|---|
| `ADDR_SRC` | 1 byte | Source device identifier (`0x01` to `0xFE`) |
| `ADDR_DST` | 1 byte | Destination device identifier (`0xFF` for broadcast) |
| `FLAGS` | 1 byte | Frame control (Start of transmission, silence, End of frame) |
| `SEQ` | 1 byte | Sequence tracking number to catch dropped packets |
| `PAYLOAD` | Variable | Compressed voice data |

## SX1278 Configuration
The transceiver registers are fine-tuned using the following low-level behaviors:

- **Preamble Length:** Configured to `8 symbols` (default) to provide reliable receiver synchronization.
* **CRC Checking:** Enabled in hardware. Corrupted RF frames are automatically dropped by the transceiver, letting our software insert clean silence frames rather than raw static noise into the audio stream.
- **Explicit Header Mode:** Enabled. Explicit frames carry payload size, coding rate, and CRC indicators natively, ensuring robust decoding.
* **AGC & LNA:** Enabled. The Low Noise Amplifier defaults to maximum gain (`G1` state) and gets managed natively by the chip to prevent up-close saturation.
