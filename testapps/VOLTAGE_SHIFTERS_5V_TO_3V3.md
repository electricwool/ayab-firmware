# Voltage Level Shifters: 5V to 3.3V for Quadrature Encoders

## The Problem

**KH-930 encoder outputs 5V logic, RP2040 GPIO is 3.3V**

**Options:**
1. Use voltage divider (passive)
2. Use level shifter IC (active)
3. Use RP2040's 5V-tolerant inputs (if available)
4. Use series resistor (simple protection)

---

## RP2040 GPIO Specifications

**From RP2040 Datasheet:**
- **VDD:** 3.3V (1.8V to 3.3V)
- **GPIO Input High (VIH):** 2.0V minimum
- **GPIO Input Low (VIL):** 0.8V maximum
- **Absolute Maximum:** 3.63V ⚠️
- **5V Tolerant:** ❌ NO! (will damage GPIO)

**Conclusion: Must use level shifting for 5V encoder signals**

---

## Option 1: Voltage Divider (Passive)

### Circuit

```
5V Encoder ──┬──[R1: 1.8kΩ]──┬──> RP2040 GPIO (3.3V)
             │                │
             │                └──[100nF]──> GND
             │
             └──[R2: 3.3kΩ]──> GND
```

### Specifications

- **Cost:** $0.10 (2 resistors + cap per channel)
- **Speed:** ~100 kHz (limited by RC time constant)
- **Channels:** Need 3 (A, B, INDEX)
- **Direction:** Unidirectional (5V → 3.3V only)
- **Total Cost:** $0.30 (3 channels)

### Voltage Calculation

```
Vout = Vin × R2 / (R1 + R2)
Vout = 5V × 3.3kΩ / (1.8kΩ + 3.3kΩ)
Vout = 5V × 0.647 = 3.24V ✅
```

### Pros
✅ **Cheapest** ($0.10 per channel)  
✅ **Simplest** (just resistors)  
✅ **No power needed**  
✅ **Reliable**

### Cons
❌ **Slow** (~100 kHz max)  
❌ **Not bidirectional**  
❌ **Wastes power** (constant current draw)  
❌ **Takes PCB space** (3 resistor pairs)

### Verdict
⚠️ **Acceptable for KH-930** (encoder is < 1 kHz)  
✅ **Best for cost-sensitive designs**

---

## Option 2: TXS0104E (Texas Instruments)

### Specifications

- **Cost:** $0.85
- **Channels:** 4 (perfect for A, B, INDEX + spare)
- **Speed:** 110 Mbps (ultra-fast)
- **Direction:** Bidirectional (auto-sensing)
- **Supply:** 1.65V to 5.5V (both sides)
- **Package:** TSSOP-14

### Circuit

```
        TXS0104E
    +----------------+
5V  |VCCA        VCCB| 3.3V
    |                |
A   |A1           B1 |──> RP2040 GP10
B   |A2           B2 |──> RP2040 GP11
IDX |A3           B3 |──> RP2040 GP12
    |A4           B4 |──> (spare)
    |                |
GND |GND         OE  | 3.3V (enable)
    +----------------+
```

### Pros
✅ **Fast** (110 Mbps - overkill for encoder)  
✅ **Bidirectional** (auto-sensing)  
✅ **4 channels** (perfect for 3 encoder signals)  
✅ **Clean design** (single IC)  
✅ **Low power** (< 1 µA idle)

### Cons
⚠️ **More expensive** ($0.85 vs $0.30)  
⚠️ **Requires PCB space** (TSSOP-14)  
⚠️ **Overkill** for low-speed encoder

### Verdict
✅ **Best overall choice** for production design  
✅ **Professional solution**

---

## Option 3: TXS0108E (Texas Instruments)

### Specifications

- **Cost:** $1.20
- **Channels:** 8 (way more than needed)
- **Speed:** 60 Mbps
- **Direction:** Bidirectional (auto-sensing)
- **Supply:** 1.2V to 3.6V (A), 1.65V to 5.5V (B)
- **Package:** TSSOP-20

### Pros
✅ **8 channels** (can use for other signals)  
✅ **Bidirectional**  
✅ **Fast enough**

### Cons
❌ **Overkill** (only need 3 channels)  
❌ **More expensive** ($1.20)  
❌ **Larger package** (TSSOP-20)

### Verdict
⚠️ **Only if you need 8 channels**  
❌ **Not recommended for encoder only**

---

## Option 4: 74LVC245 (Octal Bus Transceiver)

### Specifications

- **Cost:** $0.45
- **Channels:** 8 (unidirectional)
- **Speed:** 100 MHz
- **Direction:** Unidirectional (set by DIR pin)
- **Supply:** 1.65V to 5.5V
- **Package:** TSSOP-20

### Circuit

```
        74LVC245
    +----------------+
5V  |VCC         DIR | GND (B→A direction)
    |                |
A   |A1           B1 | 5V Encoder A
B   |A2           B2 | 5V Encoder B
IDX |A3           B3 | 5V Encoder INDEX
    |A4-A8      B4-8| (unused)
    |                |
GND |GND         OE  | GND (enable)
3.3V|                |
    +----------------+
```

### Pros
✅ **Fast** (100 MHz)  
✅ **Cheap** ($0.45)  
✅ **8 channels** (extras for other signals)  
✅ **Robust** (industry standard)

### Cons
❌ **Unidirectional** (not an issue for encoder)  
⚠️ **Larger package** (TSSOP-20)  
⚠️ **Overkill** (8 channels for 3 signals)

### Verdict
✅ **Good alternative** to TXS0104E  
✅ **Best if you need extra channels**

---

## Option 5: BSS138 MOSFET (Discrete)

### Specifications

- **Cost:** $0.15 per channel ($0.45 for 3)
- **Channels:** 1 per MOSFET
- **Speed:** ~1 MHz (with pull-ups)
- **Direction:** Bidirectional
- **Supply:** No power needed (passive)

### Circuit (per channel)

```
5V Side ──┬──[10kΩ]──> 5V
          │
          ├──> BSS138 Drain
          │
3.3V Side ┼──[10kΩ]──> 3.3V
          │
          └──> BSS138 Source
          
BSS138 Gate ──> GND
```

### Pros
✅ **Cheap** ($0.15 per channel)  
✅ **Bidirectional**  
✅ **Simple**  
✅ **No power supply needed**

### Cons
❌ **Slow** (~1 MHz with pull-ups)  
❌ **Requires 3 MOSFETs + 6 resistors**  
❌ **Takes PCB space**  
❌ **More complex than IC solution**

### Verdict
⚠️ **Only for DIY/prototyping**  
❌ **Not recommended for production**

---

## Option 6: Series Resistor (Simple Protection)

### Circuit

```
5V Encoder ──[1kΩ]──> RP2040 GPIO
                  │
                  └──[100nF]──> GND
```

### Theory

**Does this work?**

RP2040 GPIO has:
- **Input protection diodes** to VDD (3.3V)
- **Maximum current:** 50 mA (absolute max)

When 5V is applied:
- Voltage across resistor: 5V - 3.3V = 1.7V
- Current through diode: 1.7V / 1kΩ = 1.7 mA ✅

**This is safe!** (well below 50 mA limit)

### Specifications

- **Cost:** $0.05 per channel ($0.15 for 3)
- **Speed:** ~1 MHz (limited by RC)
- **Direction:** Unidirectional (5V → 3.3V)
- **Protection:** Via internal diodes

### Pros
✅ **Cheapest** ($0.05 per channel)  
✅ **Simplest** (one resistor + cap)  
✅ **Fast enough** for encoder  
✅ **Minimal PCB space**

### Cons
⚠️ **Relies on internal diodes** (not ideal)  
⚠️ **Wastes power** (1.7 mA per channel)  
⚠️ **Not recommended by RP2040 datasheet**  
❌ **May reduce GPIO lifespan**

### Verdict
⚠️ **Works but not recommended**  
❌ **Use only for prototyping**

---

## Comparison Table

| Method | Cost | Speed | Channels | Bidirectional | Recommended |
|--------|------|-------|----------|---------------|-------------|
| **Voltage Divider** | $0.30 | 100 kHz | 3 | No | ⚠️ Budget |
| **TXS0104E** | $0.85 | 110 Mbps | 4 | Yes | ✅ **Best** |
| **TXS0108E** | $1.20 | 60 Mbps | 8 | Yes | ⚠️ Overkill |
| **74LVC245** | $0.45 | 100 MHz | 8 | No | ✅ Good |
| **BSS138** | $0.45 | 1 MHz | 3 | Yes | ⚠️ DIY only |
| **Series Resistor** | $0.15 | 1 MHz | 3 | No | ❌ Not safe |

---

## Detailed Recommendation

### For Production: **TXS0104E** ⭐

**Why:**
1. ✅ **Professional solution** - designed for level shifting
2. ✅ **Fast** (110 Mbps - handles any encoder speed)
3. ✅ **4 channels** - perfect for A, B, INDEX + spare
4. ✅ **Bidirectional** - future-proof
5. ✅ **Low power** - < 1 µA idle
6. ✅ **Small footprint** - TSSOP-14
7. ✅ **Reliable** - industry standard

**Cost:** $0.85 (reasonable for quality)

### For Budget Builds: **Voltage Divider**

**Why:**
1. ✅ **Cheapest** - $0.30 total
2. ✅ **Simple** - just resistors
3. ✅ **Fast enough** - 100 kHz > 1 kHz encoder
4. ✅ **Reliable** - no active components

**Limitation:** Slower than IC solutions (not an issue for KH-930)

### For Multi-Signal: **74LVC245**

**Why:**
1. ✅ **8 channels** - can shift other signals too
2. ✅ **Fast** - 100 MHz
3. ✅ **Cheap** - $0.45
4. ✅ **Robust** - industry standard

**Use if:** You have other 5V signals to shift

---

## Final Circuit with TXS0104E

```
┌─────────────────────────────────────────────────────────────────┐
│ ENCODER LEVEL SHIFTING (TXS0104E)                               │
│                                                                  │
│  KH-930 Encoder (5V)                                            │
│                                                                  │
│  Encoder A (5V) ──[100Ω]──┬──[100nF]──GND                      │
│                            │                                     │
│                            └──> TXS0104E A1                     │
│                                                                  │
│  Encoder B (5V) ──[100Ω]──┬──[100nF]──GND                      │
│                            │                                     │
│                            └──> TXS0104E A2                     │
│                                                                  │
│  Belt Phase (5V) ─[100Ω]──┬──[100nF]──GND                      │
│                            │                                     │
│                            └──> TXS0104E A3                     │
│                                                                  │
│    TXS0104E                                                      │
│   +----------------+                                             │
│ 5V|VCCA        VCCB| 3.3V                                       │
│   |                |                                             │
│   |A1           B1 |──> RP2040 GP10 (Encoder A)                │
│   |A2           B2 |──> RP2040 GP11 (Encoder B)                │
│   |A3           B3 |──> RP2040 GP12 (Belt Phase)               │
│   |A4           B4 |──> (spare)                                 │
│   |                |                                             │
│GND|GND         OE  | 3.3V (always enabled)                     │
│   +----------------+                                             │
│                                                                  │
│  Decoupling:                                                     │
│  5V   ──[100nF]──> GND (near VCCA)                             │
│  3.3V ──[100nF]──> GND (near VCCB)                             │
│                                                                  │
│  Speed: 110 Mbps (overkill for 1 kHz encoder)                  │
│  Latency: < 10 ns (negligible)                                  │
│  Cost: $0.85 + $0.30 (passives) = $1.15                        │
└─────────────────────────────────────────────────────────────────┘
```

---

## Updated BOM (with TXS0104E)

| Qty | Part | Description | Unit Price | Total |
|-----|------|-------------|------------|-------|
| 1 | ADS1015 | 12-bit I2C ADC | $3.50 | $3.50 |
| 1 | TXS0104E | 4-ch level shifter | $0.85 | $0.85 |
| 1 | MCP23S17 | 16-pin SPI GPIO | $1.20 | $1.20 |
| 5 | 100kΩ | Input resistors | $0.05 | $0.25 |
| 3 | 100Ω | Encoder resistors | $0.05 | $0.15 |
| 2 | 10nF | Hall sensor caps | $0.05 | $0.10 |
| 3 | 100nF | Encoder filter caps | $0.05 | $0.15 |
| 3 | 100nF | Decoupling caps | $0.05 | $0.15 |
| 2 | 4.7kΩ | I2C pull-ups | $0.05 | $0.10 |
| 1 | 10kΩ | ALERT pull-up | $0.05 | $0.05 |
| **TOTAL** | | | | **$6.50** |

**Total cost: $6.50 (was $5.60 without level shifter)**

---

## Alternative: Budget Build (Voltage Divider)

| Qty | Part | Description | Unit Price | Total |
|-----|------|-------------|------------|-------|
| 1 | ADS1015 | 12-bit I2C ADC | $3.50 | $3.50 |
| 1 | MCP23S17 | 16-pin SPI GPIO | $1.20 | $1.20 |
| 5 | 100kΩ | Input resistors | $0.05 | $0.25 |
| 3 | 1.8kΩ | Divider R1 | $0.05 | $0.15 |
| 3 | 3.3kΩ | Divider R2 | $0.05 | $0.15 |
| 2 | 10nF | Hall sensor caps | $0.05 | $0.10 |
| 3 | 100nF | Encoder filter caps | $0.05 | $0.15 |
| 3 | 100nF | Decoupling caps | $0.05 | $0.15 |
| 2 | 4.7kΩ | I2C pull-ups | $0.05 | $0.10 |
| 1 | 10kΩ | ALERT pull-up | $0.05 | $0.05 |
| **TOTAL** | | | | **$5.80** |

**Budget build: $5.80 (saves $0.70)**

---

## Conclusion

### Production Design: **TXS0104E**

✅ **Professional** - designed for level shifting  
✅ **Fast** - 110 Mbps (overkill but future-proof)  
✅ **Reliable** - industry standard  
✅ **Clean** - single IC solution  
✅ **Cost** - $6.50 total (reasonable)

### Budget Design: **Voltage Divider**

✅ **Cheapest** - $5.80 total  
✅ **Simple** - just resistors  
✅ **Adequate** - 100 kHz > 1 kHz encoder  
✅ **Reliable** - passive components

**Recommendation: Use TXS0104E for production, voltage divider for prototypes.**

**Final system:**
- **ADS1015 (I2C):** Hall sensors with comparator
- **TXS0104E:** 5V to 3.3V level shifting
- **GPIO Interrupts:** Encoder counting (free!)
- **MCP23S17 (SPI):** Solenoid control

**Total cost: $6.50 | All margins > 99% | Professional quality**

---

## ADS1015 Input RC Filter Design

### Optimal RC Filter for 3300 SPS Sampling

**Recommended: 4.7kΩ + 4.7nF**

#### Why This Combination Is Optimal:

**1. Settling Time Analysis:**
```
Sample period at 3300 SPS = 303µs
RC time constant (τ) = 4.7kΩ × 4.7nF = 22µs
Settling time (5τ) = 110µs
Margin = 303µs - 110µs = 193µs (64% spare)

✓ Fast enough for reliable conversion
✓ Good margin for component tolerances
✓ Allows ADC internal settling time
```

**2. Noise Filtering Performance:**
```
Cutoff frequency = 1 / (2π × 4.7kΩ × 4.7nF) = 7.2 kHz

Noise attenuation:
50 Hz (mains):    -0.1 dB (1% reduction)
1 kHz:            -2.4 dB (24% reduction)
10 kHz:           -9 dB (65% reduction)
100 kHz:          -29 dB (97% reduction)
1 MHz:            -49 dB (99.6% reduction)

✓ Excellent high-frequency noise rejection
✓ Moderate low-frequency filtering
✓ Preserves hall sensor signal bandwidth
```

**3. Source Impedance:**
```
Source impedance = 4.7kΩ

ADS1015 recommended: <10kΩ ✓
Input bias current error: 10nA × 4.7kΩ = 47µV (negligible)
Thermal noise: ~9µV RMS (excellent)

✓ Well within ADS1015 specifications
✓ Minimal voltage errors
✓ Low noise contribution
```

#### Complete Hall Sensor Input Circuit:

```
5V Hall Sensor (Left) ──[4.7kΩ]──┬──[4.7nF]──> ADS1015 AIN0
                                 GND

5V Hall Sensor (Right) ──[4.7kΩ]──┬──[4.7nF]──> ADS1015 AIN1
                                  GND

ADS1015 Power:
VDD ──[0.1µF]──> GND (close to IC)
    └─[10µF]──> GND (bulk cap)

ADS1015 Configuration:
- Gain: GAIN_TWOTHIRDS (±6.144V range for 5V sensors)
- Sample Rate: 3300 SPS
- Mode: Continuous conversion
```

#### RC Filter Comparison Table:

| Resistor | Capacitor | τ (RC) | Settling (5τ) | % Period | Cutoff Freq | Noise @ 1kHz | Rating |
|----------|-----------|--------|---------------|----------|-------------|--------------|--------|
| **4.7kΩ** | **4.7nF** | **22µs** | **110µs** | **36%** | **7.2 kHz** | **-2.4 dB (24%)** | **★★★★★ BEST** |
| 4.7kΩ | 2.2nF | 10.3µs | 52µs | 17% | 15.4 kHz | -0.9 dB (10%) | ★★★★ Good |
| 2.2kΩ | 10nF | 22µs | 110µs | 36% | 7.2 kHz | -2.4 dB (24%) | ★★★★ Good |
| 10kΩ | 2.2nF | 22µs | 110µs | 36% | 7.2 kHz | -2.4 dB (24%) | ★★★ OK |
| 10kΩ | 1nF | 10µs | 50µs | 16% | 15.9 kHz | -0.9 dB (10%) | ★★★ OK |

#### Component Specifications:

**Resistors (4.7kΩ):**
```
Value: 4.7kΩ ±1%
Package: 0805 or 0603
Power: 1/8W (0805) or 1/10W (0603)
Type: Thick film
LCSC: C25900 (Yageo RC0805FR-074K7L, 0805, 1%)
Quantity: 2 (one per channel)
```

**Capacitors (4.7nF):**
```
Value: 4.7nF (4700pF)
Voltage: 50V
Dielectric: X7R or C0G/NP0
Package: 0805 or 0603
Tolerance: ±10% (X7R) or ±5% (C0G)
LCSC: C1779 (Samsung CL21B472KBANNNC, 0805, 50V, X7R)
Quantity: 2 (one per channel)
```

#### Alternative RC Combinations:

**Option 2: 2.2kΩ + 10nF (Lower impedance)**
```
Hall Sensor ──[2.2kΩ]──┬──[10nF]──> ADS1015 AIN
                       GND

Advantages:
✓ Lower source impedance (2.2kΩ)
✓ Same settling time (110µs)
✓ Same cutoff frequency (7.2 kHz)
✓ Better for long cable runs

Disadvantages:
✗ Less input protection
✗ Higher current draw from sensor
```

**Option 3: 10kΩ + 2.2nF (Higher impedance)**
```
Hall Sensor ──[10kΩ]──┬──[2.2nF]──> ADS1015 AIN
                      GND

Advantages:
✓ Better input protection
✓ Lower current draw
✓ Same settling time (110µs)

Disadvantages:
✗ Higher source impedance (marginal for ADS1015)
✗ More thermal noise
```

#### ADS1015 Code Example:

```cpp
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1015 ads;  // 12-bit ADC

void setup() {
  ads.begin();
  
  // Configure for 5V hall sensors
  ads.setGain(GAIN_TWOTHIRDS);  // ±6.144V range
  ads.setDataRate(RATE_ADS1015_3300SPS);  // 3300 SPS
  
  // Start continuous conversion on AIN0
  ads.startADCReading(ADS1X15_REG_CONFIG_MUX_SINGLE_0, /*continuous=*/true);
}

void loop() {
  // Read left sensor (AIN0)
  int16_t left_raw = ads.getLastConversionResults();
  float left_voltage = ads.computeVolts(left_raw);
  
  // Switch to right sensor (AIN1)
  ads.startADCReading(ADS1X15_REG_CONFIG_MUX_SINGLE_1, /*continuous=*/true);
  delay(1);  // Wait for conversion
  
  // Read right sensor (AIN1)
  int16_t right_raw = ads.getLastConversionResults();
  float right_voltage = ads.computeVolts(right_raw);
  
  // Convert voltage to position (0-5V = 0-100%)
  float left_position = (left_voltage / 5.0) * 100.0;
  float right_position = (right_voltage / 5.0) * 100.0;
  
  // Switch back to left sensor
  ads.startADCReading(ADS1X15_REG_CONFIG_MUX_SINGLE_0, /*continuous=*/true);
}
```

#### Summary:

**For ADS1015 at 3300 SPS with 5V hall effect sensors:**

**Use 4.7kΩ + 4.7nF RC filter:**
- ✓ Optimal settling time (110µs = 36% of period)
- ✓ Good noise filtering (7.2 kHz cutoff)
- ✓ Balanced source impedance (4.7kΩ)
- ✓ Standard E12 values
- ✓ Low cost and readily available
- ✓ Excellent performance for hall effect sensors

This combination provides the best balance of speed, noise immunity, and reliability for 5V hall effect position sensors with the ADS1015 at 3300 SPS sampling rate.
