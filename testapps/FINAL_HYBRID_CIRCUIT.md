# Final Hybrid Circuit Design - KH-930 Knitting Machine

## System Overview

**Hybrid Design: ADS1015 + GPIO Interrupts**

- **ADS1015 #1 (0x48):** Carriage position detection (2 hall sensors)
- **GPIO Interrupts:** Belt phase + Encoder A/B (3 signals)

**Performance:**
- Carriage detection: 1.6 ms latency (analog, polarity sensing)
- Encoder signals: 10 µs latency (digital, ultra-fast)
- Total cost: $5.35
- GPIO pins: 6 total

---

## Complete Circuit Schematic

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         RP2040 Pico W                                       │
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
│ ENCODER SIGNALS (GPIO INTERRUPTS)                                           │
│                                                                              │
│  Belt Phase Signal (from optical/hall sensor on belt pulley)                │
│      |                                                                       │
│      +---[R3: 10k]---+---[C3: 100nF]---GND                                  │
│                      |                                                       │
│                      +---> Comparator IN+                                    │
│                                                                              │
│    LM393 Comparator #1                                                      │
│   +----------------+                                                         │
│   |IN+         OUT|---+---> GP10 (Belt Phase)                               │
│   |IN-         VCC|   |                                                     │
│   |GND            |   +---[R4: 10k]---3.3V (pull-up)                        │
│   +----------------+                                                         │
│     |                                                                        │
│     +--- 1.68V reference (R5: 10k + R6: 10k voltage divider from 3.3V)     │
│                                                                              │
│                                                                              │
│  Encoder A Signal (from optical encoder on belt)                            │
│      |                                                                       │
│      +---[R7: 10k]---+---[C4: 100nF]---GND                                  │
│                      |                                                       │
│                      +---> Comparator IN+                                    │
│                                                                              │
│    LM393 Comparator #2                                                      │
│   +----------------+                                                         │
│   |IN+         OUT|---+---> GP11 (Encoder A)                                │
│   |IN-         VCC|   |                                                     │
│   |GND            |   +---[R8: 10k]---3.3V (pull-up)                        │
│   +----------------+                                                         │
│     |                                                                        │
│     +--- 1.68V reference (shared with Belt Phase)                           │
│                                                                              │
│                                                                              │
│  Encoder B Signal (from optical encoder on belt)                            │
│      |                                                                       │
│      +---[R9: 10k]---+---[C5: 100nF]---GND                                  │
│                      |                                                       │
│                      +---> Comparator IN+                                    │
│                                                                              │
│    LM393 Comparator #3 (uses 2nd half of LM393 #2)                         │
│   +----------------+                                                         │
│   |IN+         OUT|---+---> GP12 (Encoder B)                                │
│   |IN-         VCC|   |                                                     │
│   |GND            |   +---[R10: 10k]---3.3V (pull-up)                       │
│   +----------------+                                                         │
│     |                                                                        │
│     +--- 1.68V reference (shared)                                           │
│                                                                              │
│  Note: LM393 is dual comparator, so need 2 ICs for 3 comparators           │
└──────────────────────────────────────────────────────────────────────────────┘


REFERENCE VOLTAGE GENERATOR (Shared by all comparators)
┌──────────────────────────────────────────────────────┐
│                                                      │
│  3.3V ----[R5: 10k]----+----[R6: 10k]---- GND       │
│                        |                             │
│                        +---> 1.68V Reference         │
│                        |                             │
│                        +---[C6: 10µF]--- GND         │
│                                                      │
│  This provides stable 1.68V threshold for all       │
│  comparators (midpoint of hall sensor range)        │
└──────────────────────────────────────────────────────┘
```

---

## Detailed Connection Table

### ADS1015 Connections

| ADS1015 Pin | Connection | Description |
|-------------|------------|-------------|
| VDD | 3.3V | Power supply |
| GND | GND | Ground |
| SDA | RP2040 GP4 | I2C data |
| SCL | RP2040 GP5 | I2C clock |
| ALERT | RP2040 GP6 | Interrupt output (open-drain) |
| ADDR | GND | I2C address = 0x48 |
| AIN0 | Left Hall Sensor | Carriage left position |
| AIN1 | Right Hall Sensor | Carriage right position |
| AIN2 | Not connected | - |
| AIN3 | Not connected | - |

### Comparator Connections

**LM393 #1 (Belt Phase + Encoder A):**

| Pin | Connection | Description |
|-----|------------|-------------|
| 1 (OUT1) | RP2040 GP10 + 10kΩ pull-up | Belt Phase output |
| 2 (IN1-) | 1.68V reference | Threshold voltage |
| 3 (IN1+) | Belt Phase sensor | Input signal |
| 4 (GND) | GND | Ground |
| 5 (IN2+) | Encoder A sensor | Input signal |
| 6 (IN2-) | 1.68V reference | Threshold voltage |
| 7 (OUT2) | RP2040 GP11 + 10kΩ pull-up | Encoder A output |
| 8 (VCC) | 3.3V | Power supply |

**LM393 #2 (Encoder B + spare):**

| Pin | Connection | Description |
|-----|------------|-------------|
| 1 (OUT1) | RP2040 GP12 + 10kΩ pull-up | Encoder B output |
| 2 (IN1-) | 1.68V reference | Threshold voltage |
| 3 (IN1+) | Encoder B sensor | Input signal |
| 4 (GND) | GND | Ground |
| 5 (IN2+) | Not connected | Spare comparator |
| 6 (IN2-) | Not connected | Spare comparator |
| 7 (OUT2) | Not connected | Spare comparator |
| 8 (VCC) | 3.3V | Power supply |

### RP2040 GPIO Assignments

| GPIO Pin | Function | Type | Description |
|----------|----------|------|-------------|
| GP4 | I2C SDA | I2C | ADS1015 data line |
| GP5 | I2C SCL | I2C | ADS1015 clock line |
| GP6 | ALERT | Input (interrupt) | ADS1015 comparator alert |
| GP10 | Belt Phase | Input (interrupt) | Belt position detection |
| GP11 | Encoder A | Input (interrupt) | Quadrature channel A |
| GP12 | Encoder B | Input (interrupt) | Quadrature channel B |

---

## Bill of Materials (BOM)

### ICs and Modules

| Qty | Part Number | Description | Unit Price | Total | Source |
|-----|-------------|-------------|------------|-------|--------|
| 1 | ADS1015 | 12-bit ADC Module | $3.50 | $3.50 | Adafruit, SparkFun |
| 2 | LM393 | Dual Comparator IC | $0.30 | $0.60 | Digikey, Mouser |

### Resistors (1/4W, 5%)

| Qty | Value | Description | Unit Price | Total |
|-----|-------|-------------|------------|-------|
| 2 | 100kΩ | Hall sensor input protection | $0.05 | $0.10 |
| 3 | 10kΩ | Encoder input resistors | $0.05 | $0.15 |
| 3 | 10kΩ | Comparator pull-up resistors | $0.05 | $0.15 |
| 2 | 10kΩ | Reference voltage divider | $0.05 | $0.10 |
| 2 | 4.7kΩ | I2C pull-up resistors | $0.05 | $0.10 |
| 1 | 10kΩ | ALERT pull-up resistor | $0.05 | $0.05 |

### Capacitors

| Qty | Value | Description | Unit Price | Total |
|-----|-------|-------------|------------|-------|
| 2 | 10nF | Hall sensor input filtering | $0.05 | $0.10 |
| 3 | 100nF | Encoder input filtering | $0.05 | $0.15 |
| 1 | 10µF | Reference voltage filtering | $0.10 | $0.10 |

### **TOTAL COST: $5.35**

---

## PCB Layout Recommendations

### Component Placement

```
┌─────────────────────────────────────────────────┐
│                                                 │
│  [RP2040 Pico W]                                │
│                                                 │
│  GP4 ──┐  GP10 ──┐                             │
│  GP5 ──┤  GP11 ──┤                             │
│  GP6 ──┤  GP12 ──┤                             │
│        │         │                              │
│        │         │                              │
│  ┌─────┴────┐  ┌─┴──────────┐                  │
│  │ ADS1015  │  │ LM393 #1   │                  │
│  │          │  │ LM393 #2   │                  │
│  │  0x48    │  │            │                  │
│  └──────────┘  └────────────┘                  │
│                                                 │
│  [Voltage Reference]                            │
│  [Decoupling Caps]                              │
│                                                 │
│  Connectors:                                    │
│  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐          │
│  │Hall L│ │Hall R│ │Belt  │ │Enc AB│          │
│  └──────┘ └──────┘ └──────┘ └──────┘          │
└─────────────────────────────────────────────────┘
```

### Design Guidelines

1. **Keep I2C traces short** - minimize noise on SDA/SCL
2. **Place decoupling caps close to ICs** - 100nF near each VCC pin
3. **Separate analog and digital grounds** - star ground at power supply
4. **Shield encoder signals** - use twisted pair or shielded cable
5. **Add test points** - for debugging and calibration

---

## Connector Pinouts

### Hall Sensor Connectors (2× 3-pin)

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

### Encoder Connector (5-pin)

```
Pin 1: VCC (3.3V or 5V depending on encoder)
Pin 2: Belt Phase → Comparator → GP10
Pin 3: Encoder A → Comparator → GP11
Pin 4: Encoder B → Comparator → GP12
Pin 5: GND
```

---

## Performance Summary

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

### Encoder Signals (GPIO)

| Parameter | Value |
|-----------|-------|
| Resolution | 16 PPR (KH-930) |
| Effective Resolution | 64 counts/rev (4× multiplier) |
| Latency | 10 µs |
| Max Pulse Frequency | 140 Hz (at 1 m/s) |
| Pulse Period | 7.1 ms |
| Margin | 99.86% (7090 µs) |
| Direction Detection | Yes (quadrature) |
| Speed Measurement | Yes (pulse frequency) |

### System Performance

| Metric | Value |
|--------|-------|
| Total GPIO Pins | 6 |
| Total Cost | $5.35 |
| Power Consumption | ~50 mA |
| I2C Bus Speed | 400 kHz |
| Interrupt Sources | 4 (ALERT + 3× GPIO) |
| Update Rate | Event-driven (instant) |

---

## Advantages of Hybrid Design

### ✅ Best of Both Worlds

**ADS1015 for Carriage Detection:**
- ✅ Analog sensing (detects magnet polarity)
- ✅ Distinguishes 3 carriage types
- ✅ Window comparator (automatic ALERT)
- ✅ Noise filtering
- ✅ Event-driven operation
- ✅ 92% margin (plenty for 20ms window)

**GPIO for Encoder Signals:**
- ✅ Ultra-low latency (10µs vs 1600µs)
- ✅ 160× faster than ADS1015
- ✅ 99.86% margin (massive headroom)
- ✅ Hardware quadrature decoding
- ✅ No I2C overhead
- ✅ Deterministic timing

### ✅ Cost Effective

- Single ADS1015: $3.50
- Comparators: $0.60
- Passives: $1.25
- **Total: $5.35** (vs $7.65 for dual ADS1015)
- **Savings: $2.30 (30% cheaper)**

### ✅ Performance

- Carriage: 1.6 ms (sufficient)
- Encoder: 10 µs (excellent)
- **Best performance where it matters most**

---

## Firmware Integration

The firmware from [`OPTIMIZED_LOW_LATENCY_ENCODER.md`](testapps/OPTIMIZED_LOW_LATENCY_ENCODER.md) implements this exact circuit design with:

- ADS1015 I2C communication
- Window comparator configuration
- GPIO interrupt handlers
- Quadrature decoding
- Carriage type identification
- Position tracking
- Direction detection

---

## Testing and Calibration

### Initial Setup

1. **Power up circuit** - verify 3.3V on all VCC pins
2. **Check I2C** - scan for ADS1015 at 0x48
3. **Verify reference voltage** - measure 1.68V at comparator IN- pins
4. **Test hall sensors** - verify voltage swing (0V / 1.68V / 3.47V)
5. **Test encoder signals** - verify quadrature pattern

### Calibration

1. **ADS1015 thresholds** - adjust if needed (default: 1.0V / 2.5V)
2. **Comparator thresholds** - adjust R5/R6 if needed (default: 1.68V)
3. **Encoder direction** - swap A/B if direction inverted
4. **Position zero** - set reference when carriage at known position

---

## Conclusion

This hybrid design provides:

✅ **Optimal performance** - Fast where needed, precise where required  
✅ **Cost effective** - 30% cheaper than dual ADS1015  
✅ **Reliable** - 92-99% margins on all signals  
✅ **Scalable** - Easy to add more sensors  
✅ **Proven** - Uses standard, widely-available components  

**Recommended for KH-930 knitting machine carriage detection and position tracking.**
