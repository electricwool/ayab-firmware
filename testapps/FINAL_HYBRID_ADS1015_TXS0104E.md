# Final Hybrid Design: ADS1015 + TXS0104E Level Shifter

## System Overview

**Optimized Hybrid Design for KH-930 with 5V Encoder**

- **ADS1015 (0x48):** Carriage position detection (2 hall sensors, analog)
- **TXS0104E:** 5V→3.3V level shifter for encoder signals (3 channels, digital)
- **Direct GPIO:** RP2040 reads level-shifted encoder signals

**Key Features:**
- ✅ Handles 5V encoder signals safely
- ✅ Ultra-low latency encoder (10 µs via GPIO)
- ✅ Analog carriage detection with polarity sensing
- ✅ Minimal part count (2 ICs + passives)
- ✅ Simple, reliable design

---

## Complete Circuit Schematic

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         RP2040 Pico W (3.3V)                                │
│                                                                             │
│  GP4 (I2C SDA) ────────────────────┬────────────────────────────────┐      │
│  GP5 (I2C SCL) ────────────────┐   │                                │      │
│  GP6 (ALERT) ──────────────┐   │   │                                │      │
│  GP10 (Belt Phase) ────┐   │   │   │                                │      │
│  GP11 (Encoder A) ──┐  │   │   │   │                                │      │
│  GP12 (Encoder B) ┐ │  │   │   │   │                                │      │
│                   │ │  │   │   │   │                                │      │
└───────────────────┼─┼──┼───┼───┼───┼────────────────────────────────┼──────┘
                    │ │  │   │   │   │                                │
                    │ │  │   │   │   │ 4.7kΩ                     4.7kΩ│
                    │ │  │   │   │   ├───┐                       ┌────┤
                    │ │  │   │   │   │   │                       │    │
                    │ │  │  10kΩ │  3.3V │                      3.3V  │
                    │ │  │ ┌──┴─┐ │       │                            │
                    │ │  │ │    │ │       │                            │
                    │ │  │ 3.3V │ │       │                            │
                    │ │  │      │ │       │                            │
┌───────────────────┼─┼──┼──────┼─┼───────┼────────────────────────────┼──────┐
│ CARRIAGE POSITION DETECTION (ADS1015)                                │      │
│                                                                       │      │
│  Left Hall Sensor (Bipolar: 0V / 1.68V / 3.47V)                     │      │
│      |                                                                │      │
│      +---[R1: 100k]---+---[C1: 10nF]---GND                          │      │
│                       |                                               │      │
│                       +---> AIN0                                      │      │
│                                                                       │      │
│  Right Hall Sensor (Bipolar: 0V / 1.68V / 3.47V)                    │      │
│      |                                                                │      │
│      +---[R2: 100k]---+---[C2: 10nF]---GND                          │      │
│                       |                                               │      │
│                       +---> AIN1                                      │      │
│                                                                       │      │
│    ADS1015 (Address 0x48)                                            │      │
│   +----------------+                                                  │      │
│   |VDD         SDA|--------------------------------------------------┘      │
│   |GND         SCL|----------------------------------------------------------┘
│   |A0        ALERT|----------------------------------------------------------┐
│   |A1          ADD|---GND (addr 0x48)                                       │
│   |A2             |                                                          │
│   |A3             |                                                          │
│   +----------------+                                                         │
│                                                                              │
│  Window Comparator: 1.0V - 2.5V (inactive range)                           │
│  ALERT triggers when: voltage < 1.0V (SOUTH) or > 2.5V (NORTH)             │
└──────────────────────────────────────────────────────────────────────────────┘


┌──────────────────────────────────────────────────────────────────────────────┐
│ ENCODER SIGNALS (5V from KH-930)                                            │
│                                                                              │
│  Belt Phase (5V logic from KH-930)                                          │
│      |                                                                       │
│      +---[R3: 1k]---+---[C3: 100nF]---GND                                   │
│                     |                                                        │
│                     +---> TXS0104E A1 (5V side)                              │
│                                                                              │
│  Encoder A (5V logic from KH-930)                                           │
│      |                                                                       │
│      +---[R4: 1k]---+---[C4: 100nF]---GND                                   │
│                     |                                                        │
│                     +---> TXS0104E A2 (5V side)                              │
│                                                                              │
│  Encoder B (5V logic from KH-930)                                           │
│      |                                                                       │
│      +---[R5: 1k]---+---[C5: 100nF]---GND                                   │
│                     |                                                        │
│                     +---> TXS0104E A3 (5V side)                              │
│                                                                              │
│                                                                              │
│    TXS0104E (4-Channel Bidirectional Level Shifter)                         │
│   +------------------+                                                       │
│   | VCCA (5V)        |--- 5V (from KH-930 encoder supply)                   │
│   | VCCB (3.3V)      |--- 3.3V (from RP2040)                                │
│   | GND              |--- GND                                                │
│   |                  |                                                       │
│   | A1 (5V side)     |◄── Belt Phase (5V)                                   │
│   | B1 (3.3V side)   |──► GP10 (Belt Phase, 3.3V)                           │
│   |                  |                                                       │
│   | A2 (5V side)     |◄── Encoder A (5V)                                    │
│   | B2 (3.3V side)   |──► GP11 (Encoder A, 3.3V)                            │
│   |                  |                                                       │
│   | A3 (5V side)     |◄── Encoder B (5V)                                    │
│   | B3 (3.3V side)   |──► GP12 (Encoder B, 3.3V)                            │
│   |                  |                                                       │
│   | A4 (5V side)     |--- Not connected (spare channel)                     │
│   | B4 (3.3V side)   |--- Not connected (spare channel)                     │
│   |                  |                                                       │
│   | OE (Output En)   |--- 3.3V (always enabled)                             │
│   +------------------+                                                       │
│                                                                              │
│  Note: TXS0104E is bidirectional, auto-direction sensing                    │
│  No external pull-ups needed (internal 10kΩ pull-ups)                       │
└──────────────────────────────────────────────────────────────────────────────┘


POWER SUPPLY
┌──────────────────────────────────────────────────────┐
│                                                      │
│  5V Supply (from KH-930 encoder)                    │
│    ├──> TXS0104E VCCA                               │
│    └──> [100nF decoupling cap] ──> GND              │
│                                                      │
│  3.3V Supply (from RP2040)                          │
│    ├──> ADS1015 VDD                                 │
│    ├──> TXS0104E VCCB                               │
│    ├──> TXS0104E OE                                 │
│    ├──> [100nF decoupling cap] ──> GND (ADS1015)   │
│    └──> [100nF decoupling cap] ──> GND (TXS0104E)  │
└──────────────────────────────────────────────────────┘
```

---

## Bill of Materials (BOM)

### ICs and Modules

| Qty | Part Number | Description | Unit Price | Total | Notes |
|-----|-------------|-------------|------------|-------|-------|
| 1 | ADS1015 | 12-bit ADC Module | $3.50 | $3.50 | Pre-assembled module |
| 1 | TXS0104E | 4-ch Level Shifter | $1.20 | $1.20 | TSSOP-14 package |

### Resistors (1/4W, 5%)

| Qty | Value | Description | Unit Price | Total |
|-----|-------|-------------|------------|-------|
| 2 | 100kΩ | Hall sensor input protection | $0.05 | $0.10 |
| 3 | 1kΩ | Encoder input protection | $0.05 | $0.15 |
| 2 | 4.7kΩ | I2C pull-ups | $0.05 | $0.10 |
| 1 | 10kΩ | ALERT pull-up | $0.05 | $0.05 |

### Capacitors

| Qty | Value | Description | Unit Price | Total |
|-----|-------|-------------|------------|-------|
| 2 | 10nF | Hall sensor filtering | $0.05 | $0.10 |
| 3 | 100nF | Encoder filtering | $0.05 | $0.15 |
| 2 | 100nF | Decoupling (ADS1015, TXS0104E) | $0.05 | $0.10 |

### **TOTAL COST: $5.55**
### **TOTAL PART COUNT: 17 components**

---

## Detailed Connection Table

### ADS1015 Connections

| ADS1015 Pin | Connection | Description |
|-------------|------------|-------------|
| VDD | 3.3V + 100nF cap | Power supply |
| GND | GND | Ground |
| SDA | RP2040 GP4 + 4.7kΩ pull-up | I2C data |
| SCL | RP2040 GP5 + 4.7kΩ pull-up | I2C clock |
| ALERT | RP2040 GP6 + 10kΩ pull-up | Interrupt output |
| ADDR | GND | I2C address = 0x48 |
| AIN0 | Left Hall Sensor + 100kΩ + 10nF | Carriage left |
| AIN1 | Right Hall Sensor + 100kΩ + 10nF | Carriage right |
| AIN2 | NC | Not connected |
| AIN3 | NC | Not connected |

### TXS0104E Connections

| TXS0104E Pin | Connection | Description |
|--------------|------------|-------------|
| VCCA | 5V + 100nF cap | 5V side power |
| VCCB | 3.3V + 100nF cap | 3.3V side power |
| GND | GND | Ground |
| OE | 3.3V | Output enable (always on) |
| A1 | Belt Phase (5V) + 1kΩ + 100nF | 5V input |
| B1 | RP2040 GP10 | 3.3V output |
| A2 | Encoder A (5V) + 1kΩ + 100nF | 5V input |
| B2 | RP2040 GP11 | 3.3V output |
| A3 | Encoder B (5V) + 1kΩ + 100nF | 5V input |
| B3 | RP2040 GP12 | 3.3V output |
| A4 | NC | Spare channel |
| B4 | NC | Spare channel |

### RP2040 GPIO Assignments

| GPIO Pin | Function | Type | Description |
|----------|----------|------|-------------|
| GP4 | I2C SDA | I2C | ADS1015 data line |
| GP5 | I2C SCL | I2C | ADS1015 clock line |
| GP6 | ALERT | Input (interrupt) | ADS1015 comparator alert |
| GP10 | Belt Phase | Input (interrupt) | Level-shifted from 5V |
| GP11 | Encoder A | Input (interrupt) | Level-shifted from 5V |
| GP12 | Encoder B | Input (interrupt) | Level-shifted from 5V |

**Total GPIO pins: 6**

---

## PCB Layout

### Compact 2-Layer Design

```
┌─────────────────────────────────────────────────┐
│                                                 │
│  [RP2040 Pico W]                                │
│                                                 │
│  GP4 ──┬──────────────────────┐                │
│  GP5 ──┤                      │                │
│  GP6 ──┤  ┌────────────┐      │                │
│  GP10 ─┼──┤ TXS0104E   │      │                │
│  GP11 ─┼──┤            │      │                │
│  GP12 ─┼──┤  5V↔3.3V   │      │                │
│        │  └────────────┘      │                │
│        │         ↑            │                │
│        │         │ 5V         │                │
│        │         │            │                │
│  ┌─────┴────┐    │            │                │
│  │ ADS1015  │    │            │                │
│  │          │    │            │                │
│  │  0x48    │    │            │                │
│  │          │    │            │                │
│  │ AIN0 AIN1│    │            │                │
│  └──┬───┬───┘    │            │                │
│     │   │        │            │                │
│  ┌──┴─┐ ┌┴──┐    │            │                │
│  │Hall│ │Hall│   │            │                │
│  │Left│ │Right   │            │                │
│  └────┘ └────┘   │            │                │
│                   │            │                │
│  Connectors:      │            │                │
│  [Hall L] [Hall R] [5V Encoder (4-pin)]        │
└─────────────────────────────────────────────────┘
```

### Layout Guidelines

1. **Place TXS0104E close to RP2040** - minimize 3.3V trace length
2. **Keep 5V traces short** - minimize noise coupling
3. **Separate 5V and 3.3V grounds** - connect at single point (star ground)
4. **Decoupling caps** - place close to IC VCC pins
5. **Input filtering** - RC filters close to TXS0104E inputs

---

## Connector Pinouts

### Hall Sensor Connectors (2× 3-pin JST-XH)

**Left Hall Sensor:**
```
Pin 1: VCC (3.3V)
Pin 2: Signal → ADS1015 AIN0
Pin 3: GND
```

**Right Hall Sensor:**
```
Pin 1: VCC (3.3V)
Pin 2: Signal → ADS1015 AIN1
Pin 3: GND
```

### Encoder Connector (5-pin JST-XH)

```
Pin 1: VCC (5V from KH-930)
Pin 2: Belt Phase (5V) → TXS0104E A1
Pin 3: Encoder A (5V) → TXS0104E A2
Pin 4: Encoder B (5V) → TXS0104E A3
Pin 5: GND
```

---

## Performance Specifications

### Carriage Detection (ADS1015)

| Parameter | Value |
|-----------|-------|
| Resolution | 12-bit (4096 levels) |
| Sample Rate | 3300 SPS |
| Latency | 1.6 ms |
| Detection Window | 20 ms (at 1 m/s) |
| Margin | 92% (18.4 ms) |
| Detections per Pass | 12× |
| States Detected | 3 (South/Inactive/North) |
| Carriage Types | 3 (Lace/K/G) |

### Encoder Signals (GPIO via TXS0104E)

| Parameter | Value |
|-----------|-------|
| Level Shifter | TXS0104E (bidirectional) |
| Propagation Delay | 10 ns (TXS0104E) |
| Total Latency | ~10 µs (GPIO interrupt + processing) |
| KH-930 Encoder | 16 PPR |
| Pulse Period | 7.1 ms (at 1 m/s) |
| Margin | 99.86% (7090 µs) |
| Input Voltage | 5V (from KH-930) |
| Output Voltage | 3.3V (to RP2040) |
| Bandwidth | 100 Mbps (TXS0104E) |

### System Performance

| Metric | Value |
|--------|-------|
| Total GPIO Pins | 6 |
| Total Cost | $5.55 |
| Part Count | 17 |
| Power Consumption | ~45 mA |
| I2C Bus Speed | 400 kHz |
| Interrupt Sources | 4 (ALERT + 3× GPIO) |
| 5V Compatibility | Yes (via TXS0104E) |

---

## Advantages of This Design

### ✅ Key Benefits

1. **5V Safe** - TXS0104E protects RP2040 from 5V encoder signals
2. **Ultra-low latency** - 10 µs for encoder (160× faster than ADS1015)
3. **Analog carriage detection** - Polarity sensing for carriage ID
4. **Minimal parts** - Only 17 components total
5. **Bidirectional** - TXS0104E can handle bidirectional signals if needed
6. **Auto-direction** - TXS0104E automatically senses signal direction
7. **No pull-ups needed** - TXS0104E has internal 10kΩ pull-ups
8. **Fast** - 100 Mbps bandwidth, 10 ns propagation delay
9. **Reliable** - Industry-standard level shifter IC

### ✅ vs Other Designs

| Design | Cost | Latency (Encoder) | 5V Safe | Part Count |
|--------|------|-------------------|---------|------------|
| **This Design** | $5.55 | **10 µs** | **Yes** | 17 |
| Dual ADS1015 | $7.65 | 1600 µs | Yes | 15 |
| Resistor Dividers | $3.80 | 10 µs | Yes | 15 |
| Direct GPIO | $3.50 | 10 µs | **No** | 9 |

---

## TXS0104E Features

### Why TXS0104E is Ideal

**Key Features:**
- ✅ **Bidirectional** - auto-direction sensing
- ✅ **No direction pin** - automatically detects signal direction
- ✅ **Internal pull-ups** - 10kΩ on both sides
- ✅ **Fast** - 10 ns propagation delay, 100 Mbps
- ✅ **Low power** - < 1 µA quiescent current
- ✅ **Wide voltage** - VCCA: 1.65-5.5V, VCCB: 1.2-3.6V
- ✅ **Small package** - TSSOP-14 (easy to solder)
- ✅ **ESD protection** - ±2kV HBM

**How It Works:**
- Uses one-shot edge detection
- Automatically determines signal direction
- Drives output based on input edge
- No external components needed (except decoupling)

---

## Firmware Integration

The firmware from [`OPTIMIZED_LOW_LATENCY_ENCODER.md`](testapps/OPTIMIZED_LOW_LATENCY_ENCODER.md) works directly with this design:

- ADS1015 I2C communication (same as before)
- GPIO interrupt handlers for encoder (same as before)
- TXS0104E is transparent - firmware doesn't need to know it's there!

**No firmware changes needed** - TXS0104E is hardware-only solution.

---

## Testing and Calibration

### Initial Setup

1. **Power up** - verify 5V and 3.3V supplies
2. **Check I2C** - scan for ADS1015 at 0x48
3. **Test level shifter** - apply 5V to A1, measure 3.3V at B1
4. **Verify hall sensors** - check voltage swing (0V / 1.68V / 3.47V)
5. **Test encoder** - verify 5V signals convert to 3.3V

### Calibration

1. **ADS1015 thresholds** - adjust if needed (default: 1.0V / 2.5V)
2. **Encoder direction** - swap A/B if direction inverted
3. **Position zero** - set reference when carriage at known position

---

## Conclusion

This hybrid design provides:

✅ **5V compatibility** - Safe for KH-930's 5V encoder  
✅ **Ultra-low latency** - 10 µs for encoder signals  
✅ **Analog precision** - Carriage polarity detection  
✅ **Minimal parts** - Only 17 components  
✅ **Cost effective** - $5.55 total  
✅ **Reliable** - Industry-standard ICs  
✅ **Simple** - No firmware complexity  

**Recommended for KH-930 knitting machine with 5V encoder signals.**

**Total manufacturing cost: ~$10 per board** (including labor and testing)
