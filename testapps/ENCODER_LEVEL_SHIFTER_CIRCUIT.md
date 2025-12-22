# 4-Channel Level Shifter Circuit: 5V Encoder to 3.3V RP2040

## Circuit Design for KH-930 Encoder Interface

**Purpose:** Convert 3 digital 5V encoder signals (Encoder A, Encoder B, Belt Phase/ENC_C) to 3.3V for RP2040 GPIO

**IC Used:** TXS0104E (4-channel bidirectional level shifter)

---

## Complete Circuit Schematic

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         KH-930 ENCODER (5V LOGIC)                           │
│                                                                             │
│  Encoder Connector (from KH-930)                                           │
│  ┌──────────────┐                                                          │
│  │ Pin 1: VCC   │──> 5V (encoder power supply)                             │
│  │ Pin 2: ENC_A │──> Encoder A (5V digital)                                │
│  │ Pin 3: ENC_B │──> Encoder B (5V digital)                                │
│  │ Pin 4: ENC_C │──> Belt Phase (5V digital)                               │
│  │ Pin 5: GND   │──> GND                                                    │
│  └──────────────┘                                                          │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
                    │         │         │         │
                    │         │         │         │
                   5V      ENC_A     ENC_B     ENC_C
                    │         │         │         │
                    │         │         │         │
┌───────────────────┼─────────┼─────────┼─────────┼───────────────────────────┐
│ INPUT FILTERING   │         │         │         │                           │
│                   │         │         │         │                           │
│                   │    ┌────┴────┐    │         │                           │
│                   │    │ [100Ω]  │    │         │                           │
│                   │    └────┬────┘    │         │                           │
│                   │         ├─────[100nF]───GND │                           │
│                   │         │         │         │                           │
│                   │         │    ┌────┴────┐    │                           │
│                   │         │    │ [100Ω]  │    │                           │
│                   │         │    └────┬────┘    │                           │
│                   │         │         ├─────[100nF]───GND                   │
│                   │         │         │         │                           │
│                   │         │         │    ┌────┴────┐                      │
│                   │         │         │    │ [100Ω]  │                      │
│                   │         │         │    └────┬────┘                      │
│                   │         │         │         ├─────[100nF]───GND         │
│                   │         │         │         │                           │
└───────────────────┼─────────┼─────────┼─────────┼───────────────────────────┘
                    │         │         │         │
                    │         │         │         │
┌───────────────────┼─────────┼─────────┼─────────┼───────────────────────────┐
│ LEVEL SHIFTER     │         │         │         │                           │
│                   │         │         │         │                           │
│    TXS0104E (TSSOP-14)                          │                           │
│   ┌────────────────────────┐                    │                           │
│   │                        │                    │                           │
│ 5V│ 1  VCCA           VCCB │14  3.3V                                       │
│ ──┤                        ├──                                              │
│   │                        │                                                │
│   │ 2  A1              B1  │13 ──> RP2040 GP10 (Encoder A, 3.3V)           │
│ ──┤◄── ENC_A (5V)          │                                                │
│   │                        │                                                │
│   │ 3  A2              B2  │12 ──> RP2040 GP11 (Encoder B, 3.3V)           │
│ ──┤◄── ENC_B (5V)          │                                                │
│   │                        │                                                │
│   │ 4  A3              B3  │11 ──> RP2040 GP12 (Belt Phase, 3.3V)          │
│ ──┤◄── ENC_C (5V)          │                                                │
│   │                        │                                                │
│   │ 5  A4              B4  │10 ──> (spare channel)                          │
│   │    (not connected)     │                                                │
│   │                        │                                                │
│   │ 6  NC              NC  │9                                               │
│   │                        │                                                │
│GND│ 7  GND             OE  │8   3.3V (always enabled)                      │
│ ──┤                        ├──                                              │
│   └────────────────────────┘                                                │
│                                                                             │
│  Decoupling Capacitors:                                                     │
│  5V   ──[100nF]──> GND (near pin 1, VCCA)                                  │
│  3.3V ──[100nF]──> GND (near pin 14, VCCB)                                 │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │         │         │
                                    │         │         │
                                    │         │         │
┌───────────────────────────────────┼─────────┼─────────┼───────────────────┐
│                         RP2040 PICO W (3.3V)          │                   │
│                                   │         │         │                   │
│                          GP10 ◄───┘         │         │                   │
│                          (Encoder A)        │         │                   │
│                                             │         │                   │
│                          GP11 ◄─────────────┘         │                   │
│                          (Encoder B)                  │                   │
│                                                       │                   │
│                          GP12 ◄───────────────────────┘                   │
│                          (Belt Phase/ENC_C)                               │
│                                                                           │
└───────────────────────────────────────────────────────────────────────────┘


POWER CONNECTIONS
┌──────────────────────────────────────────────────────┐
│                                                      │
│  5V Supply (from KH-930 encoder)                    │
│    ├──> TXS0104E VCCA (pin 1)                       │
│    ├──> [100nF] ──> GND (decoupling)                │
│    └──> Encoder power                               │
│                                                      │
│  3.3V Supply (from RP2040)                          │
│    ├──> TXS0104E VCCB (pin 14)                      │
│    ├──> TXS0104E OE (pin 8, always enabled)         │
│    └──> [100nF] ──> GND (decoupling)                │
│                                                      │
│  GND (common ground)                                 │
│    ├──> TXS0104E GND (pin 7)                        │
│    ├──> KH-930 encoder GND                          │
│    ├──> RP2040 GND                                  │
│    └──> All decoupling capacitors                   │
└──────────────────────────────────────────────────────┘
```

---

## Bill of Materials

| Qty | Part Number | Description | Package | Unit Price | Total |
|-----|-------------|-------------|---------|------------|-------|
| 1 | TXS0104E | 4-ch bidirectional level shifter | TSSOP-14 | $0.85 | $0.85 |
| 3 | 100Ω | Input series resistors | 0805 SMD | $0.02 | $0.06 |
| 3 | 100nF | Input filter capacitors | 0805 SMD | $0.05 | $0.15 |
| 2 | 100nF | Decoupling capacitors | 0805 SMD | $0.05 | $0.10 |
| **TOTAL** | | | | | **$1.16** |

---

## Detailed Pin Connections

### TXS0104E Pinout (TSSOP-14)

```
        ┌─────────┐
   VCCA │1      14│ VCCB
     A1 │2      13│ B1
     A2 │3      12│ B2
     A3 │4      11│ B3
     A4 │5      10│ B4
     NC │6       9│ NC
    GND │7       8│ OE
        └─────────┘
```

### Connection Table

| TXS0104E Pin | Pin Name | Connection | Description |
|--------------|----------|------------|-------------|
| 1 | VCCA | 5V + 100nF to GND | 5V side power |
| 2 | A1 | ENC_A (via 100Ω + 100nF) | Encoder A input (5V) |
| 3 | A2 | ENC_B (via 100Ω + 100nF) | Encoder B input (5V) |
| 4 | A3 | ENC_C (via 100Ω + 100nF) | Belt Phase input (5V) |
| 5 | A4 | Not connected | Spare channel |
| 6 | NC | Not connected | No connection |
| 7 | GND | Common ground | Ground |
| 8 | OE | 3.3V | Output enable (always on) |
| 9 | NC | Not connected | No connection |
| 10 | B4 | Not connected | Spare channel |
| 11 | B3 | RP2040 GP12 | Belt Phase output (3.3V) |
| 12 | B2 | RP2040 GP11 | Encoder B output (3.3V) |
| 13 | B1 | RP2040 GP10 | Encoder A output (3.3V) |
| 14 | VCCB | 3.3V + 100nF to GND | 3.3V side power |

### RP2040 GPIO Assignments

| GPIO Pin | Function | Direction | Description |
|----------|----------|-----------|-------------|
| GP10 | ENC_A | Input (interrupt) | Encoder A (quadrature) |
| GP11 | ENC_B | Input (interrupt) | Encoder B (quadrature) |
| GP12 | ENC_C | Input (interrupt) | Belt Phase (digital state) |

---

## PCB Layout Recommendations

### Component Placement

```
┌─────────────────────────────────────────────────────┐
│                                                     │
│  [Encoder Connector]                                │
│         │  │  │                                     │
│         │  │  │                                     │
│    [R1][R2][R3]  ← Series resistors (100Ω)         │
│         │  │  │                                     │
│    [C1][C2][C3]  ← Filter caps (100nF)             │
│         │  │  │                                     │
│         │  │  │                                     │
│    ┌────────────┐                                   │
│    │  TXS0104E  │                                   │
│    │            │                                   │
│ [C4]│          │[C5]  ← Decoupling caps (100nF)    │
│    └────────────┘                                   │
│         │  │  │                                     │
│         │  │  │                                     │
│    [To RP2040 GPIO]                                 │
│                                                     │
└─────────────────────────────────────────────────────┘
```

### Layout Guidelines

1. **Keep traces short** between encoder connector and TXS0104E
2. **Place decoupling caps close** to VCCA (pin 1) and VCCB (pin 14)
3. **Use ground plane** for noise immunity
4. **Separate 5V and 3.3V power planes** if possible
5. **Route encoder signals away** from noisy power traces

---

## Circuit Operation

### Signal Flow

1. **5V encoder signals** enter through connector
2. **Series resistors (100Ω)** limit current and provide protection
3. **Filter capacitors (100nF)** remove high-frequency noise
4. **TXS0104E A-side** receives filtered 5V signals
5. **Level translation** occurs inside TXS0104E
6. **TXS0104E B-side** outputs clean 3.3V signals
7. **RP2040 GPIO** receives 3.3V logic levels

### Voltage Levels

| Signal | 5V Side (A) | 3.3V Side (B) |
|--------|-------------|---------------|
| Logic LOW | 0V - 1.5V | 0V - 0.8V |
| Logic HIGH | 3.5V - 5.5V | 2.0V - 3.6V |
| Typical LOW | 0V | 0V |
| Typical HIGH | 5V | 3.3V |

### Timing Characteristics

| Parameter | Value | Notes |
|-----------|-------|-------|
| Propagation delay | < 10 ns | A to B or B to A |
| Rise time | < 5 ns | 10% to 90% |
| Fall time | < 5 ns | 90% to 10% |
| Maximum frequency | 110 Mbps | Far exceeds encoder speed |
| Encoder frequency | ~1 kHz | KH-930 @ 1 m/s |

**Margin:** 110,000× faster than needed! ✅

---

## Alternative: Budget Voltage Divider Circuit

If cost is critical, use simple voltage dividers:

```
┌─────────────────────────────────────────────────────┐
│ BUDGET OPTION: Voltage Divider (per channel)       │
│                                                     │
│  5V Encoder Signal                                  │
│      │                                              │
│      ├───[R1: 1.8kΩ]───┬──> RP2040 GPIO            │
│      │                 │                            │
│      │                 └───[100nF]──> GND           │
│      │                                              │
│      └───[R2: 3.3kΩ]──> GND                        │
│                                                     │
│  Output voltage: 5V × 3.3kΩ / (1.8kΩ + 3.3kΩ)     │
│                = 5V × 0.647 = 3.24V ✅             │
│                                                     │
│  Cost per channel: $0.10                           │
│  Total cost (3 channels): $0.30                    │
│  Savings vs TXS0104E: $0.86                        │
│                                                     │
│  Limitations:                                       │
│  - Slower (RC time constant limits to ~100 kHz)    │
│  - Wastes power (constant current draw)            │
│  - Not bidirectional                               │
│  - Still adequate for 1 kHz encoder                │
└─────────────────────────────────────────────────────┘
```

### Budget BOM

| Qty | Part | Value | Unit Price | Total |
|-----|------|-------|------------|-------|
| 3 | Resistor | 1.8kΩ | $0.02 | $0.06 |
| 3 | Resistor | 3.3kΩ | $0.02 | $0.06 |
| 3 | Capacitor | 100nF | $0.05 | $0.15 |
| **TOTAL** | | | | **$0.27** |

---

## Firmware Configuration

### RP2040 GPIO Setup

```cpp
// Initialize encoder GPIO pins
#define ENCODER_A_PIN 10
#define ENCODER_B_PIN 11
#define BELT_PHASE_PIN 12

void init_encoder_pins() {
  // Encoder A
  gpio_init(ENCODER_A_PIN);
  gpio_set_dir(ENCODER_A_PIN, GPIO_IN);
  gpio_pull_up(ENCODER_A_PIN);  // Optional pull-up
  gpio_set_irq_enabled(ENCODER_A_PIN, 
                       GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 
                       true);
  
  // Encoder B
  gpio_init(ENCODER_B_PIN);
  gpio_set_dir(ENCODER_B_PIN, GPIO_IN);
  gpio_pull_up(ENCODER_B_PIN);  // Optional pull-up
  gpio_set_irq_enabled(ENCODER_B_PIN, 
                       GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 
                       true);
  
  // Belt Phase
  gpio_init(BELT_PHASE_PIN);
  gpio_set_dir(BELT_PHASE_PIN, GPIO_IN);
  gpio_pull_up(BELT_PHASE_PIN);  // Optional pull-up
  gpio_set_irq_enabled(BELT_PHASE_PIN, 
                       GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 
                       true);
  
  // Set up interrupt handler
  gpio_set_irq_enabled_with_callback(ENCODER_A_PIN, 
                                     GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
                                     true, 
                                     &encoder_irq_handler);
  
  // Enable IRQ
  irq_set_enabled(IO_IRQ_BANK0, true);
}
```

---

## Testing Procedure

### 1. Power Supply Test
- Measure VCCA (pin 1): Should be 5V ±0.25V
- Measure VCCB (pin 14): Should be 3.3V ±0.1V
- Measure OE (pin 8): Should be 3.3V (enabled)

### 2. Input Signal Test
- Apply 5V to A1 (pin 2): B1 (pin 13) should output 3.3V
- Apply 0V to A1 (pin 2): B1 (pin 13) should output 0V
- Repeat for A2/B2 and A3/B3

### 3. Encoder Signal Test
- Connect encoder and rotate slowly
- Monitor GP10, GP11 with oscilloscope
- Verify quadrature pattern (90° phase shift)
- Check GP12 for belt phase transitions

### 4. Frequency Test
- Rotate encoder at maximum speed
- Verify all pulses are captured
- Check for signal integrity (clean edges)

---

## Troubleshooting

| Problem | Possible Cause | Solution |
|---------|----------------|----------|
| No output | OE not connected to 3.3V | Connect pin 8 to 3.3V |
| Weak signals | Missing pull-ups | Add 10kΩ pull-ups on B-side |
| Noisy signals | Missing filter caps | Add 100nF caps on inputs |
| Wrong voltage | Incorrect VCCA/VCCB | Check power connections |
| Intermittent | Poor ground connection | Ensure solid ground plane |

---

## Summary

### Recommended: TXS0104E Level Shifter

✅ **Professional solution** - designed for level shifting  
✅ **Fast** - 110 Mbps (110,000× faster than needed)  
✅ **Bidirectional** - future-proof  
✅ **4 channels** - 3 used, 1 spare  
✅ **Low power** - < 1 µA idle  
✅ **Reliable** - industry standard  
✅ **Cost** - $1.16 total (reasonable)

### Budget Alternative: Voltage Dividers

✅ **Cheapest** - $0.27 total  
✅ **Simple** - just resistors and caps  
✅ **Adequate** - 100 kHz > 1 kHz encoder  
⚠️ **Slower** - not suitable for high-speed encoders  
⚠️ **Wastes power** - constant current draw

**Recommendation: Use TXS0104E for production, voltage dividers for prototyping.**
