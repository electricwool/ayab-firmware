# High-Speed SPI Design for Ultra-Low Latency

## Overview

Complete redesign using **SPI instead of I2C** for dramatically improved latency. SPI is 10-100× faster than I2C for this application.

**System Components:**
- **MCP3204:** 4-channel SPI ADC for 2 hall sensors (carriage detection)
- **LS7366R:** Hardware quadrature encoder counter (Encoder A, B, Belt Phase/INDEX)
- **MCP23S17:** 16-pin SPI GPIO expander for 16 solenoids (output)

**Target:** < 100 µs total system latency (vs 1600 µs with I2C)

---

## SPI vs I2C Performance Comparison

| Parameter | I2C @ 400kHz | I2C @ 1MHz | SPI @ 10MHz | SPI @ 20MHz |
|-----------|--------------|------------|-------------|-------------|
| **Clock Speed** | 400 kHz | 1 MHz | 10 MHz | 20 MHz |
| **Byte Transfer** | 25 µs | 10 µs | 1 µs | 0.5 µs |
| **16-bit Read** | 50 µs | 20 µs | 2 µs | 1 µs |
| **Overhead** | High (start/stop) | High | Low | Low |
| **Total Latency** | 1600 µs | 1000 µs | **100 µs** | **50 µs** |

**SPI is 16-32× faster!**

---

## Part 1: SPI ADCs (ADS1015 Alternatives)

### Option 1: MCP3202 ⭐ BEST VALUE

**Manufacturer:** Microchip  
**Price:** $1.50  
**Interface:** SPI

**Specifications:**
- **Resolution:** 12-bit (same as ADS1015)
- **Sample Rate:** 100 kSPS (30× faster than ADS1015!)
- **Channels:** 2 (differential or single-ended)
- **SPI Speed:** Up to 1.8 MHz
- **Supply:** 2.7-5.5V (works with 3.3V)
- **Package:** 8-pin SOIC/DIP

**Latency:**
- Sample time: 10 µs (@ 100 kSPS)
- SPI read: 2 µs (@ 10 MHz)
- **Total: ~15 µs** (100× faster than ADS1015!)

**Advantages:**
- ✅ **Cheapest:** $1.50 (vs $3.50 for ADS1015)
- ✅ **Fastest:** 100 kSPS
- ✅ **Simple:** Easy SPI protocol
- ✅ **Low power:** 500 µA active

**Disadvantages:**
- ❌ **No comparator:** Must poll (no ALERT pin)
- ❌ **Only 2 channels:** Need 1 IC per 2 sensors

---

### Option 2: MCP3204

**Same as MCP3202 but 4 channels**

**Price:** $2.00  
**Channels:** 4 (can monitor 4 sensors)

**Perfect for:** 2 hall sensors + 2 spare channels

---

### Option 3: MCP3208

**Price:** $2.50  
**Channels:** 8 (can monitor 8 sensors!)

**Perfect for:** Future expansion

---

### Option 4: LTC1867 ⭐ HIGH PERFORMANCE

**Manufacturer:** Analog Devices  
**Price:** $5.50  
**Interface:** SPI

**Specifications:**
- **Resolution:** 16-bit (better than ADS1015!)
- **Sample Rate:** 200 kSPS
- **Channels:** 8 (differential or single-ended)
- **SPI Speed:** Up to 20 MHz
- **Supply:** 2.7-5.5V

**Latency:**
- Sample time: 5 µs (@ 200 kSPS)
- SPI read: 1 µs (@ 20 MHz)
- **Total: ~10 µs** (160× faster than ADS1015!)

**Advantages:**
- ✅ **Highest resolution:** 16-bit
- ✅ **Fastest:** 200 kSPS
- ✅ **Most channels:** 8
- ✅ **Best performance**

**Disadvantages:**
- ❌ **Expensive:** $5.50
- ❌ **No comparator:** Must poll

---

### Option 5: AD7606 (Simultaneous Sampling)

**Price:** $12.00  
**Channels:** 8 (simultaneous!)  
**Resolution:** 16-bit  
**Sample Rate:** 200 kSPS per channel

**Overkill for this application** - but excellent for high-speed multi-channel

---

## SPI ADC Comparison Table

| IC | Price | Channels | Resolution | Sample Rate | Latency | Best For |
|----|-------|----------|------------|-------------|---------|----------|
| **MCP3202** | $1.50 | 2 | 12-bit | 100 kSPS | 15 µs | **Budget** ⭐ |
| **MCP3204** | $2.00 | 4 | 12-bit | 100 kSPS | 15 µs | **Best value** ⭐ |
| **MCP3208** | $2.50 | 8 | 12-bit | 100 kSPS | 15 µs | Expansion |
| **LTC1867** | $5.50 | 8 | 16-bit | 200 kSPS | 10 µs | **Performance** |
| **ADS1015** (I2C) | $3.50 | 4 | 12-bit | 3.3 kSPS | 1600 µs | Baseline |

---

## Part 2: SPI GPIO Expanders (MCP23017 Alternatives)

### Option 1: MCP23S17 ⭐ PERFECT

**Manufacturer:** Microchip  
**Price:** $1.20  
**Interface:** SPI

**Specifications:**
- **GPIO Pins:** 16 (8× Port A + 8× Port B)
- **SPI Speed:** Up to 10 MHz
- **Interrupts:** 2 pins (INTA, INTB)
- **Supply:** 1.8-5.5V
- **Package:** 28-pin SOIC/DIP

**Latency:**
- SPI read (2 bytes): 2 µs (@ 10 MHz)
- **Total: ~5 µs** (320× faster than I2C MCP23017!)

**Advantages:**
- ✅ **Drop-in replacement:** Same as MCP23017 but SPI
- ✅ **Fast:** 10 MHz SPI
- ✅ **Cheap:** $1.20
- ✅ **Interrupts:** Can trigger on pin change
- ✅ **Familiar:** Same register map as MCP23017

**Perfect for:** Reading multiple digital inputs (encoder, buttons, etc.)

---

### Option 2: MCP23S08

**Price:** $0.80  
**GPIO Pins:** 8  
**Same as MCP23S17 but half the pins**

---

### Option 3: 74HC165 (Shift Register)

**Price:** $0.40  
**Type:** Parallel-in, Serial-out shift register  
**Pins:** 8 inputs

**Latency:** ~1 µs (@ 10 MHz SPI)

**Advantages:**
- ✅ **Cheapest:** $0.40
- ✅ **Fastest:** ~1 µs
- ✅ **Simple:** Just shift in data

**Disadvantages:**
- ❌ **No interrupts:** Must poll
- ❌ **Input only:** Can't output

**Perfect for:** Reading encoder + belt phase (3 inputs)

---

### Option 4: 74HC595 (Shift Register - Output)

**Price:** $0.30  
**Type:** Serial-in, Parallel-out  
**Pins:** 8 outputs

**Perfect for:** Controlling solenoids, LEDs, etc.

---

## SPI GPIO Expander Comparison

| IC | Price | Pins | Direction | Interrupts | Latency | Best For |
|----|-------|------|-----------|------------|---------|----------|
| **MCP23S17** | $1.20 | 16 | I/O | Yes | 5 µs | **Full-featured** ⭐ |
| **MCP23S08** | $0.80 | 8 | I/O | Yes | 5 µs | **Good value** |
| **74HC165** | $0.40 | 8 | Input | No | 1 µs | **Fastest/cheapest** ⭐ |
| **74HC595** | $0.30 | 8 | Output | No | 1 µs | Output only |
| **MCP23017** (I2C) | $1.20 | 16 | I/O | Yes | 1600 µs | Baseline |

---

## Part 3: Complete High-Speed SPI Design

### Recommended Configuration

**For Carriage Detection + Encoder:**

1. **MCP3204** (SPI ADC) - $2.00
   - 2 channels for hall sensors
   - 2 spare channels
   - 15 µs latency

2. **LS7366R** (Encoder Counter) - $4.50
   - Hardware quadrature decoding
   - 32-bit position counter
   - < 1 µs latency

3. **74HC165** (Shift Register) - $0.40
   - Read belt phase + status signals
   - 1 µs latency

**Total Cost:** $6.90  
**Total Latency:** < 20 µs (80× faster than I2C design!)

---

## Complete Circuit Schematic

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         RP2040 Pico W                                       │
│                                                                             │
│  SPI0 Bus (Shared):                                                         │
│  GP16 (MISO) ◄──────────────────────┬──────────────┬──────────────┐        │
│  GP17 (CS_ADC) ──────────────────┐  │              │              │        │
│  GP18 (SCK) ─────────────────┐   │  │              │              │        │
│  GP19 (MOSI) ──────────────┐ │   │  │              │              │        │
│  GP20 (CS_ENC) ──────────┐ │ │   │  │              │              │        │
│  GP21 (CS_GPIO) ────────┐│ │ │   │  │              │              │        │
│                         ││ │ │   │  │              │              │        │
└─────────────────────────┼┼─┼─┼───┼──┼──────────────┼──────────────┼────────┘
                          ││ │ │   │  │              │              │
┌─────────────────────────┼┼─┼─┼───┼──┼──────────────┼──────────────┼────────┐
│ CARRIAGE DETECTION (MCP3204 SPI ADC)                │              │        │
│                         ││ │ │   │  │              │              │        │
│  Left Hall Sensor       ││ │ │   │  │              │              │        │
│      |                  ││ │ │   │  │              │              │        │
│      +---[100k]---+---[10nF]---GND  │              │              │        │
│                   |     ││ │ │   │  │              │              │        │
│                   +---> CH0 ││ │   │  │              │              │        │
│                         ││ │ │   │  │              │              │        │
│  Right Hall Sensor      ││ │ │   │  │              │              │        │
│      |                  ││ │ │   │  │              │              │        │
│      +---[100k]---+---[10nF]---GND  │              │              │        │
│                   |     ││ │ │   │  │              │              │        │
│                   +---> CH1 ││ │   │  │              │              │        │
│                         ││ │ │   │  │              │              │        │
│    MCP3204              ││ │ │   │  │              │              │        │
│   +----------------+    ││ │ │   │  │              │              │        │
│   |VDD         DOUT|────┘│ │ │   │  │              │              │        │
│   |GND          DIN|─────┘ │ │   │  │              │              │        │
│   |CH0         CLK|────────┘ │   │  │              │              │        │
│   |CH1          CS|──────────┘   │  │              │              │        │
│   |CH2             |              │  │              │              │        │
│   |CH3             |              │  │              │              │        │
│   +----------------+              │  │              │              │        │
│                                   │  │              │              │        │
│  Latency: 15 µs (100× faster!)   │  │              │              │        │
└───────────────────────────────────┼──┼──────────────┼──────────────┼────────┘


┌───────────────────────────────────┼──┼──────────────┼──────────────┼────────┐
│ ENCODER INTERFACE (LS7366R)       │  │              │              │        │
│                                   │  │              │              │        │
│  Encoder A ──[100Ω]──+──[100nF]──┼──┼──> A         │              │        │
│                      │            │  │              │              │        │
│  Encoder B ──[100Ω]──+──[100nF]──┼──┼──> B         │              │        │
│                      │            │  │              │              │        │
│  Belt Phase ─[100Ω]──+──[100nF]──┼──┼──> INDEX     │              │        │
│                      │            │  │              │              │        │
│                     GND           │  │              │              │        │
│                                   │  │              │              │        │
│    LS7366R                        │  │              │              │        │
│   +----------------+              │  │              │              │        │
│   |VDD        MISO|───────────────┘  │              │              │        │
│   |GND        MOSI|──────────────────┘              │              │        │
│   |A           CLK|─────────────────────────────────┘              │        │
│   |B            SS|────────────────────────────────────────────────┘        │
│   |INDEX          |                                                         │
│   +----------------+                                                        │
│                                                                             │
│  32-bit position counter                                                    │
│  Latency: < 1 µs (hardware counting)                                       │
└─────────────────────────────────────────────────────────────────────────────┘


┌─────────────────────────────────────────────────────────────────────────────┐
│ OPTIONAL: DIGITAL INPUTS (74HC165 Shift Register)                          │
│                                                                             │
│  Input 0 ──> D0                                                             │
│  Input 1 ──> D1                                                             │
│  Input 2 ──> D2                                                             │
│  Input 3 ──> D3                                                             │
│  Input 4 ──> D4                                                             │
│  Input 5 ──> D5                                                             │
│  Input 6 ──> D6                                                             │
│  Input 7 ──> D7                                                             │
│                                                                             │
│    74HC165                                                                  │
│   +----------------+                                                        │
│   |VCC        DOUT|──> MISO (shared)                                       │
│   |GND         CLK|──> SCK (shared)                                        │
│   |D0-D7       PL |──> CS_GPIO                                             │
│   +----------------+                                                        │
│                                                                             │
│  Latency: 1 µs (fastest!)                                                  │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Bill of Materials

### High-Speed SPI Design

| Qty | Part | Description | Unit Price | Total |
|-----|------|-------------|------------|-------|
| 1 | MCP3204 | 4-ch 12-bit SPI ADC | $2.00 | $2.00 |
| 1 | LS7366R | Quadrature counter | $4.50 | $4.50 |
| 1 | 74HC165 | 8-bit shift register | $0.40 | $0.40 |
| 5 | 100kΩ | Input resistors | $0.05 | $0.25 |
| 5 | 10nF/100nF | Filter caps | $0.05 | $0.25 |
| 3 | 100nF | Decoupling caps | $0.05 | $0.15 |
| **TOTAL** | | | | **$7.55** |

**Part Count:** 16 components

---

## Performance Comparison

| Design | ADC Latency | Encoder Latency | Total Latency | Cost |
|--------|-------------|-----------------|---------------|------|
| **I2C (ADS1015)** | 1600 µs | 1600 µs | 3200 µs | $7.65 |
| **SPI (MCP3204 + LS7366R)** | 15 µs | < 1 µs | **< 20 µs** | $7.55 |
| **Improvement** | **100×** | **1600×** | **160×** | Cheaper! |

---

## RP2040 SPI Configuration

### GPIO Pin Assignment

| GPIO | Function | Device |
|------|----------|--------|
| GP16 | SPI MISO | All (shared) |
| GP17 | SPI CS (ADC) | MCP3204 |
| GP18 | SPI SCK | All (shared) |
| GP19 | SPI MOSI | All (shared) |
| GP20 | SPI CS (Encoder) | LS7366R |
| GP21 | SPI CS (GPIO) | 74HC165 (optional) |

**Total GPIO pins:** 6 (same as I2C design!)

### SPI Speed Settings

```cpp
// Initialize SPI @ 10 MHz
spi_init(spi0, 10000000);
gpio_set_function(GP16, GPIO_FUNC_SPI);  // MISO
gpio_set_function(GP18, GPIO_FUNC_SPI);  // SCK
gpio_set_function(GP19, GPIO_FUNC_SPI);  // MOSI

// CS pins as GPIO
gpio_init(GP17);  // CS_ADC
gpio_init(GP20);  // CS_ENC
gpio_init(GP21);  // CS_GPIO
gpio_set_dir(GP17, GPIO_OUT);
gpio_set_dir(GP20, GPIO_OUT);
gpio_set_dir(GP21, GPIO_OUT);
gpio_put(GP17, 1);  // Deselect all
gpio_put(GP20, 1);
gpio_put(GP21, 1);
```

---

## Firmware Latency Breakdown

### Reading Hall Sensors (MCP3204)

```cpp
// Read 2 channels from MCP3204
gpio_put(CS_ADC, 0);  // Select
spi_write_read_blocking(spi0, cmd, data, 3);  // 3 bytes @ 10MHz = 3µs
gpio_put(CS_ADC, 1);  // Deselect

// Total: ~5 µs per channel, 10 µs for both
```

### Reading Encoder Position (LS7366R)

```cpp
// Read 32-bit counter from LS7366R
gpio_put(CS_ENC, 0);
spi_write_blocking(spi0, 0x60, 1);  // Read command
spi_read_blocking(spi0, 0, data, 4);  // 4 bytes
gpio_put(CS_ENC, 1);

// Total: ~5 µs
```

### Total System Latency

| Operation | Time |
|-----------|------|
| Interrupt fires | 2 µs |
| Read hall sensors (2×) | 10 µs |
| Read encoder position | 5 µs |
| Process data | 3 µs |
| **TOTAL** | **20 µs** |

**160× faster than I2C design!**

---

## Recommendations

### For Maximum Performance: MCP3204 + LS7366R

**Cost:** $7.55  
**Latency:** < 20 µs  
**Best for:** High-speed applications

### For Budget: MCP3202 + Direct GPIO

**Cost:** $1.80  
**Latency:** < 30 µs  
**Best for:** Cost-sensitive designs

### For Expansion: MCP3208 + LS7366R + MCP23S17

**Cost:** $8.20  
**Latency:** < 25 µs  
**Best for:** Future-proof design with many I/O

---

## Conclusion

**SPI design is 160× faster than I2C** with similar cost!

**Recommended:** MCP3204 + LS7366R for best balance of performance and cost.
