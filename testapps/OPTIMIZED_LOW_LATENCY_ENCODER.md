# Optimized Low-Latency Carriage Movement Detection

## Overview
Hybrid design using **one ADS1015** for carriage position detection and **direct GPIO connections** for high-speed encoder signals. This provides the best balance of analog sensing capability and ultra-low latency for movement tracking.

**System Configuration:**
- **ADS1015 (0x48):** Carriage position detection (2 hall sensors)
- **GPIO Interrupts:** Belt phase, Encoder A, Encoder B (3 signals)

**Total Latency:**
- Carriage detection: ~1.6 ms (analog, polarity sensing)
- Encoder signals: **~10 µs** (160× faster!)

---

## Detailed Signal Descriptions

### Belt Phase Signal

**Purpose:** Indicates the absolute position/phase of the knitting machine belt.

**Signal Characteristics:**
- **Type:** Analog voltage signal (typically 0-3.3V or 0-5V)
- **Source:** Hall sensor, optical sensor, or potentiometer on belt/pulley
- **Pattern:** Varies continuously as belt rotates
- **Frequency:** 0.1-10 Hz (depending on belt speed)

**Physical Implementation:**
```
Belt rotation → Sensor → Analog voltage → Comparator → Digital signal

Example: Hall sensor near magnet on belt pulley
- Belt at position 0°: 0.5V (LOW)
- Belt at position 90°: 1.5V (MID)
- Belt at position 180°: 2.5V (HIGH)
- Belt at position 270°: 1.5V (MID)
- Belt at position 360°: 0.5V (LOW)
```

**Use Cases:**
1. **Synchronization:** Align carriage movement with belt position
2. **Home position:** Detect when belt returns to reference position
3. **Speed monitoring:** Measure belt rotation speed
4. **Phase alignment:** Ensure solenoids fire at correct belt phase

**Signal Processing:**
- Analog input → Comparator with threshold (e.g., 1.68V)
- Output: Digital HIGH/LOW indicating belt phase state
- Interrupt on edge transitions for precise timing

### Encoder A & B Signals (Quadrature Encoder)

**Purpose:** Provide high-resolution position and direction tracking of carriage movement.

**Quadrature Encoding Explained:**

A quadrature encoder uses **two signals (A and B) that are 90° out of phase**. This allows detection of both position and direction.

**Signal Characteristics:**
- **Type:** Digital pulses (0V/3.3V or 0V/5V)
- **Source:** Optical encoder, magnetic encoder, or hall effect sensors
- **Pattern:** Square waves, 90° phase shift between A and B
- **Frequency:** Depends on encoder resolution and carriage speed

**Quadrature Pattern:**

```
Forward Direction (Clockwise):
         ___     ___     ___     ___
A:   ___|   |___|   |___|   |___|   |___
           ___     ___     ___     ___
B:   _____|   |___|   |___|   |___|   |___

A leads B by 90° (A changes first)


Reverse Direction (Counter-clockwise):
         ___     ___     ___     ___
A:   ___|   |___|   |___|   |___|   |___
       ___     ___     ___     ___
B:   _|   |___|   |___|   |___|   |_____

B leads A by 90° (B changes first)
```

**State Transitions:**

| Previous State | Current State | Direction | Position Change |
|----------------|---------------|-----------|-----------------|
| A=0, B=0 | A=1, B=0 | Forward | +1 |
| A=1, B=0 | A=1, B=1 | Forward | +1 |
| A=1, B=1 | A=0, B=1 | Forward | +1 |
| A=0, B=1 | A=0, B=0 | Forward | +1 |
| A=0, B=0 | A=0, B=1 | Reverse | -1 |
| A=0, B=1 | A=1, B=1 | Reverse | -1 |
| A=1, B=1 | A=1, B=0 | Reverse | -1 |
| A=1, B=0 | A=0, B=0 | Reverse | -1 |

**Decoding Logic:**

```cpp
// Read current state
uint8_t a = gpio_get(ENCODER_A_PIN);
uint8_t b = gpio_get(ENCODER_B_PIN);
uint8_t current = (b << 1) | a;  // Combine into 2-bit value

// Compare with previous state
uint8_t combined = (previous << 2) | current;  // 4-bit lookup

// Lookup table for direction
const int8_t direction_table[16] = {
   0, -1,  1,  0,  // 00 -> 00, 01, 10, 11
   1,  0,  0, -1,  // 01 -> 00, 01, 10, 11
  -1,  0,  0,  1,  // 10 -> 00, 01, 10, 11
   0,  1, -1,  0   // 11 -> 00, 01, 10, 11
};

int8_t dir = direction_table[combined];
position += dir;  // Update position counter
```

**Encoder Resolution:**

| Encoder Type | PPR | Edges per Rev | Resolution |
|--------------|-----|---------------|------------|
| Low-res | 100 | 400 | 0.9° per edge |
| Medium-res | 500 | 2000 | 0.18° per edge |
| High-res | 1000 | 4000 | 0.09° per edge |
| Very high-res | 5000 | 20000 | 0.018° per edge |

**Note:** Each pulse has 4 edges (A rise, B rise, A fall, B fall), so effective resolution is 4× PPR.

**Physical Implementation Examples:**

**1. Optical Encoder:**
```
Rotating disk with slots → LED → Photodetector → Digital signal
- Disk has alternating opaque/transparent sections
- Two photodetectors offset by 90° (quarter slot)
- Output: Digital pulses on A and B channels
```

**2. Magnetic Encoder:**
```
Rotating magnet ring → Hall sensors → Digital signal
- Ring has alternating N/S poles
- Two hall sensors offset by 90° (quarter pole)
- Output: Digital pulses on A and B channels
```

**3. Incremental Encoder on Belt:**
```
Belt with markers → Sensor → Digital signal
- Markers at regular intervals on belt
- Two sensors offset by 1/4 marker spacing
- Output: Quadrature pulses as belt moves
```

**Use Cases:**

1. **Position Tracking:**
   - Count pulses to determine carriage position
   - Resolution: ±1 encoder count (e.g., ±0.1mm)

2. **Direction Detection:**
   - Phase relationship determines direction
   - Instant direction change detection

3. **Speed Measurement:**
   - Pulse frequency = speed
   - Example: 1000 pulses/sec @ 1000 PPR = 1 rev/sec

4. **Acceleration Detection:**
   - Change in pulse frequency = acceleration
   - Useful for motion control

**Signal Conditioning:**

For reliable operation, encoder signals should be:
- **Filtered:** 100nF capacitor to remove noise
- **Pull-up resistors:** 10kΩ to ensure clean HIGH state
- **Schmitt trigger:** Optional, for noisy environments
- **Comparator:** If analog signals, convert to digital

---

## System Architecture

### Signal Routing

| Signal | Connection | Latency | Purpose |
|--------|------------|---------|---------|
| **Carriage Left** | ADS1015 AIN0 | 1.6 ms | Analog hall sensor, polarity detection |
| **Carriage Right** | ADS1015 AIN1 | 1.6 ms | Analog hall sensor, polarity detection |
| **Belt Phase** | GPIO + Comparator | 10 µs | Belt position/phase monitoring |
| **Encoder A** | GPIO Interrupt | 10 µs | Quadrature channel A (position + direction) |
| **Encoder B** | GPIO Interrupt | 10 µs | Quadrature channel B (position + direction) |

### How They Work Together

**Complete Carriage Tracking System:**

```
1. CARRIAGE POSITION (ADS1015):
   - Detects which carriage type (Lace/K/G)
   - Provides absolute reference position
   - Updates when carriage passes sensors
   - Latency: 1.6ms (acceptable for 20ms window)

2. BELT PHASE (GPIO):
   - Monitors belt rotation position
   - Synchronizes carriage with belt
   - Provides timing reference
   - Latency: 10µs (ultra-fast)

3. ENCODER A/B (GPIO):
   - Tracks continuous carriage position
   - High-resolution movement detection
   - Direction and speed measurement
   - Latency: 10µs (ultra-fast)

Combined System Operation:
┌─────────────────────────────────────────────────────┐
│ Carriage passes hall sensor (ADS1015)              │
│ → Identifies carriage type (Lace/K/G)              │
│ → Sets absolute reference position                 │
│                                                     │
│ Encoder tracks relative movement (GPIO)            │
│ → Updates position continuously                    │
│ → Detects direction changes instantly              │
│                                                     │
│ Belt phase monitors synchronization (GPIO)         │
│ → Ensures timing alignment                         │
│ → Triggers solenoids at correct phase             │
└─────────────────────────────────────────────────────┘
```

**Example Operation Sequence:**

```
Time 0ms: Carriage passes left hall sensor
  → ADS1015 detects North pole (K-carriage)
  → Sets reference position: X = 0mm
  → Encoder position reset to 0

Time 5ms: Carriage moving right
  → Encoder A/B pulses: +150 counts
  → Position: X = 0 + (150 × 0.1mm) = 15mm
  → Direction: Forward (A leads B)
  → Speed: 150 counts / 5ms = 30,000 counts/sec

Time 10ms: Belt phase changes
  → Belt phase signal goes HIGH
  → Trigger solenoid actuation
  → Synchronized with carriage position

Time 20ms: Carriage passes right hall sensor
  → ADS1015 confirms K-carriage (North pole)
  → Validates encoder position
  → Total distance: 200mm (2000 encoder counts)
```

---

## Circuit Schematic

```
Carriage Position Detection (ADS1015)

    Left Hall Sensor (Bipolar, 0V / 1.68V / 3.47V)
        |
        +---[R1: 100k]---+---[C1: 10nF]---GND
                         |
                         +---> AIN0 (ADS1015)
                         
    Right Hall Sensor (Bipolar, 0V / 1.68V / 3.47V)
        |
        +---[R2: 100k]---+---[C2: 10nF]---GND
                         |
                         +---> AIN1 (ADS1015)

    ADS1015 (Address 0x48) - CARRIAGE POSITION
   +----------------+
   |VDD         SDA|---+
   |GND         SCL|---+
   |A0        ALERT|---+
   |A1          ADD|---GND (addr 0x48)
   |A2             |
   |A3             |
   +----------------+
                      |
                      +--- [R3: 10k] --- 3.3V (ALERT pull-up)
                      |
    RP2040 I2C Bus    |
    SDA (GP4) <-------+
    SCL (GP5) <-------+
    ALERT (GP6) <-----+


High-Speed Encoder Signals (Direct GPIO)

    Belt Phase Signal (analog)
        |
        +---[R4: 10k]---+---[C4: 100nF]---GND
                        |
                        +---> Comparator IN+
                        
    Comparator (LM393 or similar)
   +----------------+
   |IN+         OUT|---+---> GP10 (Belt Phase)
   |IN-         VCC|   |
   |GND            |   +---[R5: 10k]---3.3V (pull-up)
   +----------------+
     |
     +--- 1.68V reference (voltage divider)
     
    
    Encoder A Signal (digital or analog)
        |
        +---[R6: 10k]---+---[C6: 100nF]---GND
        |               |
        |               +---> Comparator/Schmitt IN+
        |
        +---> Comparator OUT ---+---> GP11 (Encoder A)
                                |
                                +---[R7: 10k]---3.3V
    
    
    Encoder B Signal (digital or analog)
        |
        +---[R8: 10k]---+---[C8: 100nF]---GND
        |               |
        |               +---> Comparator/Schmitt IN+
        |
        +---> Comparator OUT ---+---> GP12 (Encoder B)
                                |
                                +---[R9: 10k]---3.3V


Alternative: Direct Digital Connection (if encoder outputs are digital)

    Encoder A (digital 3.3V/5V)
        |
        +---[R6: 1k]---+---[C6: 100nF]---GND
                       |
                       +---> GP11 (Encoder A)
                       
    Encoder B (digital 3.3V/5V)
        |
        +---[R8: 1k]---+---[C8: 100nF]---GND
                       |
                       +---> GP12 (Encoder B)
```

---

## Components Required

### Carriage Detection (ADS1015)

| Qty | Part | Description | Unit Price | Total |
|-----|------|-------------|------------|-------|
| 1 | ADS1015 | 12-bit ADC Module | $3.50 | $3.50 |
| 2 | 100kΩ | Input protection resistors | $0.05 | $0.10 |
| 2 | 10nF | Input filtering capacitors | $0.05 | $0.10 |
| 2 | 4.7kΩ | I2C pull-up resistors | $0.05 | $0.10 |
| 1 | 10kΩ | ALERT pull-up resistor | $0.05 | $0.05 |

### Encoder Signal Conditioning

| Qty | Part | Description | Unit Price | Total |
|-----|------|-------------|------------|-------|
| 3 | LM393 | Dual comparator (2 ICs) | $0.30 | $0.90 |
| 3 | 10kΩ | Input resistors | $0.05 | $0.15 |
| 3 | 100nF | Input filtering capacitors | $0.05 | $0.15 |
| 3 | 10kΩ | Pull-up resistors | $0.05 | $0.15 |
| 3 | 10kΩ | Reference voltage divider | $0.05 | $0.15 |

**TOTAL COST:** $5.35 (vs $7.65 for dual ADS1015)

---

## Latency Comparison

### Carriage Position Detection (ADS1015)

**Same as before:**
- Total latency: ~1.6 ms
- Detection window: 20 ms
- Margin: 92%

### Encoder Signals (GPIO Interrupts)

**Ultra-low latency path:**

| Step | Operation | Time (µs) | Cumulative (µs) |
|------|-----------|-----------|-----------------|
| 1 | Encoder signal changes | 0 | 0 |
| 2 | Comparator response | 1 | 1 |
| 3 | GPIO input propagation | 0.5 | 1.5 |
| 4 | RP2040 interrupt fires | 2 | 3.5 |
| 5 | ISR reads GPIO state | 1 | 4.5 |
| 6 | Update position counter | 3 | 7.5 |
| 7 | Return from ISR | 1 | 8.5 |
| | **TOTAL LATENCY** | **~10 µs** | **~10 µs** |

**Comparison:**

| Method | Latency | Improvement |
|--------|---------|-------------|
| ADS1015 (I2C) | 1600 µs | Baseline |
| GPIO Interrupt | 10 µs | **160× faster!** |

---

## Performance Analysis

### Encoder Resolution Support

**With 10 µs latency:**

| PPR | Max Speed | Pulse Frequency | Pulse Period | Latency | Margin | Status |
|-----|-----------|-----------------|--------------|---------|--------|--------|
| 100 | 1 m/s | 100 Hz | 10 ms | 10 µs | 99.9% | ✅ Excellent |
| 500 | 1 m/s | 500 Hz | 2 ms | 10 µs | 99.5% | ✅ Excellent |
| 1000 | 1 m/s | 1 kHz | 1 ms | 10 µs | 99.0% | ✅ Excellent |
| 5000 | 1 m/s | 5 kHz | 200 µs | 10 µs | 95.0% | ✅ Excellent |
| 10000 | 1 m/s | 10 kHz | 100 µs | 10 µs | 90.0% | ✅ Good |
| 50000 | 1 m/s | 50 kHz | 20 µs | 10 µs | 50.0% | ⚠️ Marginal |

**Conclusion:** GPIO interrupts support encoders up to **10,000 PPR** with excellent margin!

### Belt Phase Detection

**Belt phase signal:**
- Update rate: 10-100 Hz
- Period: 10-100 ms
- GPIO latency: 10 µs
- **Margin: >99.9%**

✅ **Extremely fast response** for belt synchronization

---

## RP2040 Firmware - Optimized Hybrid System

### Complete Implementation

```cpp
// Optimized Low-Latency Carriage Movement Detection
// ADS1015: Carriage position (analog, polarity detection)
// GPIO: Encoder signals (digital, ultra-low latency)
// RP2040 Pico SDK

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include <stdio.h>

// I2C Configuration
#define I2C_PORT i2c0
#define I2C_SDA 4
#define I2C_SCL 5
#define I2C_FREQ 400000

// ADS1015 Configuration
#define ADS1015_ADDR 0x48
#define ALERT_PIN 6

// Encoder GPIO Pins
#define BELT_PHASE_PIN 10
#define ENCODER_A_PIN 11
#define ENCODER_B_PIN 12

// ADS1015 Registers
#define ADS1015_REG_CONVERSION  0x00
#define ADS1015_REG_CONFIG      0x01
#define ADS1015_REG_LO_THRESH   0x02
#define ADS1015_REG_HI_THRESH   0x03

// Config bits
#define ADS1015_CONFIG_MUX_AIN0_GND 0x4000
#define ADS1015_CONFIG_MUX_AIN1_GND 0x5000
#define ADS1015_CONFIG_PGA_4_096V   0x0200
#define ADS1015_CONFIG_MODE_CONT    0x0000
#define ADS1015_CONFIG_DR_3300SPS   0x00E0
#define ADS1015_CONFIG_COMP_MODE_WINDOW 0x0010
#define ADS1015_CONFIG_COMP_POL_LOW 0x0000
#define ADS1015_CONFIG_COMP_LAT_ON  0x0004
#define ADS1015_CONFIG_COMP_QUE_1   0x0000

// Thresholds
#define THRESHOLD_LOW     500    // 1.0V
#define THRESHOLD_HIGH    1250   // 2.5V

// State definitions
typedef enum {
  STATE_SOUTH = 0,
  STATE_INACTIVE = 1,
  STATE_NORTH = 2,
  STATE_ERROR = 3
} SensorState;

typedef enum {
  CARRIAGE_NONE = 0,
  CARRIAGE_LACE = 1,
  CARRIAGE_K = 2,
  CARRIAGE_G = 3
} CarriageType;

// Global state
volatile bool alert_triggered = false;
volatile bool belt_phase_changed = false;
volatile bool encoder_changed = false;

SensorState sensor_states[2] = {STATE_INACTIVE, STATE_INACTIVE};
CarriageType carriage_type = CARRIAGE_NONE;

// Encoder state
volatile int32_t encoder_position = 0;
volatile int8_t encoder_direction = 0;  // -1, 0, +1
volatile bool belt_phase_state = false;
volatile uint8_t encoder_state = 0;  // Bits: [B][A]

// Quadrature lookup table for direction
// Previous state (bits 3:2), Current state (bits 1:0)
const int8_t quadrature_table[16] = {
   0, -1,  1,  0,  // 00 -> 00, 01, 10, 11
   1,  0,  0, -1,  // 01 -> 00, 01, 10, 11
  -1,  0,  0,  1,  // 10 -> 00, 01, 10, 11
   0,  1, -1,  0   // 11 -> 00, 01, 10, 11
};

// I2C functions
void ads1015_write_register(uint8_t reg, uint16_t value) {
  uint8_t data[3] = {reg, (value >> 8) & 0xFF, value & 0xFF};
  i2c_write_blocking(I2C_PORT, ADS1015_ADDR, data, 3, false);
}

uint16_t ads1015_read_register(uint8_t reg) {
  uint8_t data[2];
  i2c_write_blocking(I2C_PORT, ADS1015_ADDR, &reg, 1, true);
  i2c_read_blocking(I2C_PORT, ADS1015_ADDR, data, 2, false);
  return (data[0] << 8) | data[1];
}

void ads1015_configure_channel(uint8_t channel) {
  uint16_t config = ADS1015_CONFIG_MODE_CONT |
                    ADS1015_CONFIG_PGA_4_096V |
                    ADS1015_CONFIG_DR_3300SPS |
                    ADS1015_CONFIG_COMP_MODE_WINDOW |
                    ADS1015_CONFIG_COMP_POL_LOW |
                    ADS1015_CONFIG_COMP_LAT_ON |
                    ADS1015_CONFIG_COMP_QUE_1;
  
  config |= (channel == 0) ? ADS1015_CONFIG_MUX_AIN0_GND : 
                             ADS1015_CONFIG_MUX_AIN1_GND;
  
  ads1015_write_register(ADS1015_REG_CONFIG, config);
  
  // Set thresholds
  ads1015_write_register(ADS1015_REG_LO_THRESH, THRESHOLD_LOW << 4);
  ads1015_write_register(ADS1015_REG_HI_THRESH, THRESHOLD_HIGH << 4);
}

int16_t ads1015_read_adc() {
  uint16_t raw = ads1015_read_register(ADS1015_REG_CONVERSION);
  return (int16_t)raw >> 4;
}

SensorState adc_to_state(int16_t adc_value) {
  if (adc_value < THRESHOLD_LOW) return STATE_SOUTH;
  if (adc_value > THRESHOLD_HIGH) return STATE_NORTH;
  return STATE_INACTIVE;
}

// GPIO interrupt handlers
void alert_irq_handler(uint gpio, uint32_t events) {
  if (gpio == ALERT_PIN && (events & GPIO_IRQ_EDGE_FALL)) {
    alert_triggered = true;
  }
}

void belt_phase_irq_handler(uint gpio, uint32_t events) {
  if (gpio == BELT_PHASE_PIN) {
    belt_phase_state = gpio_get(BELT_PHASE_PIN);
    belt_phase_changed = true;
  }
}

void encoder_irq_handler(uint gpio, uint32_t events) {
  // Read both encoder pins
  uint8_t prev_state = encoder_state & 0x03;
  uint8_t a = gpio_get(ENCODER_A_PIN);
  uint8_t b = gpio_get(ENCODER_B_PIN);
  uint8_t curr_state = (b << 1) | a;
  
  // Update encoder state
  encoder_state = (prev_state << 2) | curr_state;
  
  // Lookup direction
  int8_t dir = quadrature_table[encoder_state & 0x0F];
  
  if (dir != 0) {
    encoder_position += dir;
    encoder_direction = dir;
    encoder_changed = true;
  }
}

// Initialize hardware
void init_i2c() {
  i2c_init(I2C_PORT, I2C_FREQ);
  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA);
  gpio_pull_up(I2C_SCL);
}

void init_alert_pin() {
  gpio_init(ALERT_PIN);
  gpio_set_dir(ALERT_PIN, GPIO_IN);
  gpio_pull_up(ALERT_PIN);
  gpio_set_irq_enabled_with_callback(ALERT_PIN, GPIO_IRQ_EDGE_FALL, 
                                     true, &alert_irq_handler);
}

void init_encoder_pins() {
  // Belt phase
  gpio_init(BELT_PHASE_PIN);
  gpio_set_dir(BELT_PHASE_PIN, GPIO_IN);
  gpio_pull_up(BELT_PHASE_PIN);
  gpio_set_irq_enabled(BELT_PHASE_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 
                       true);
  gpio_add_raw_irq_handler(BELT_PHASE_PIN, belt_phase_irq_handler);
  
  // Encoder A
  gpio_init(ENCODER_A_PIN);
  gpio_set_dir(ENCODER_A_PIN, GPIO_IN);
  gpio_pull_up(ENCODER_A_PIN);
  gpio_set_irq_enabled(ENCODER_A_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 
                       true);
  gpio_add_raw_irq_handler(ENCODER_A_PIN, encoder_irq_handler);
  
  // Encoder B
  gpio_init(ENCODER_B_PIN);
  gpio_set_dir(ENCODER_B_PIN, GPIO_IN);
  gpio_pull_up(ENCODER_B_PIN);
  gpio_set_irq_enabled(ENCODER_B_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 
                       true);
  gpio_add_raw_irq_handler(ENCODER_B_PIN, encoder_irq_handler);
  
  // Enable IRQs
  irq_set_enabled(IO_IRQ_BANK0, true);
  
  // Read initial state
  encoder_state = (gpio_get(ENCODER_B_PIN) << 1) | gpio_get(ENCODER_A_PIN);
  belt_phase_state = gpio_get(BELT_PHASE_PIN);
}

bool init_ads1015() {
  uint16_t config = ads1015_read_register(ADS1015_REG_CONFIG);
  if (config == 0x0000 || config == 0xFFFF) {
    printf("ERROR: ADS1015 not found\n");
    return false;
  }
  printf("ADS1015 initialized\n");
  return true;
}

int main() {
  stdio_init_all();
  sleep_ms(2000);
  
  printf("\n================================================\n");
  printf("Optimized Low-Latency Carriage Movement Detection\n");
  printf("================================================\n");
  printf("ADS1015: Carriage position (analog, 1.6ms latency)\n");
  printf("GPIO: Encoder signals (digital, 10µs latency)\n\n");
  
  // Initialize hardware
  init_i2c();
  init_alert_pin();
  init_encoder_pins();
  
  if (!init_ads1015()) {
    while (1) sleep_ms(1000);
  }
  
  // Configure carriage sensors
  for (int ch = 0; ch < 2; ch++) {
    ads1015_configure_channel(ch);
    sleep_ms(10);
    int16_t adc = ads1015_read_adc();
    sensor_states[ch] = adc_to_state(adc);
    printf("Sensor %d initial: %dmV (%d)\n", ch, adc, sensor_states[ch]);
  }
  
  printf("\nEncoder position: %ld\n", encoder_position);
  printf("Belt phase: %s\n\n", belt_phase_state ? "HIGH" : "LOW");
  printf("Monitoring...\n\n");
  
  // Main loop
  uint32_t last_report = 0;
  
  while (1) {
    // Handle carriage detection (ADS1015)
    if (alert_triggered) {
      alert_triggered = false;
      
      for (int ch = 0; ch < 2; ch++) {
        ads1015_configure_channel(ch);
        sleep_us(500);
        int16_t adc = ads1015_read_adc();
        SensorState new_state = adc_to_state(adc);
        
        if (new_state != sensor_states[ch]) {
          sensor_states[ch] = new_state;
          printf("[CARRIAGE] Sensor %d: %dmV (%d)\n", ch, adc, new_state);
        }
      }
      
      ads1015_read_register(ADS1015_REG_CONFIG);  // Clear latch
    }
    
    // Handle encoder changes (GPIO - ultra-fast!)
    if (encoder_changed) {
      encoder_changed = false;
      printf("[ENCODER] Position: %ld (dir: %s)\n", 
             encoder_position,
             encoder_direction > 0 ? "→" : encoder_direction < 0 ? "←" : "?");
    }
    
    // Handle belt phase changes (GPIO - ultra-fast!)
    if (belt_phase_changed) {
      belt_phase_changed = false;
      printf("[BELT] Phase: %s\n", belt_phase_state ? "HIGH" : "LOW");
    }
    
    // Periodic status report
    uint32_t now = time_us_32();
    if (now - last_report > 1000000) {  // Every 1 second
      last_report = now;
      printf("Status: Encoder=%ld, Belt=%s, Carriage=%d/%d\n",
             encoder_position,
             belt_phase_state ? "HIGH" : "LOW",
             sensor_states[0], sensor_states[1]);
    }
    
    // Sleep until next interrupt
    __wfi();
  }
  
  return 0;
}
```

---

## Advantages of Hybrid Design

### ✅ Best of Both Worlds

**ADS1015 for Carriage Detection:**
- ✅ Analog sensing (detects magnet polarity)
- ✅ Distinguishes 3 carriage types (Lace/K/G)
- ✅ Window comparator (automatic ALERT)
- ✅ Noise filtering
- ✅ 1.6ms latency (sufficient for 20ms window)

**GPIO for Encoder Signals:**
- ✅ Ultra-low latency (10µs vs 1600µs)
- ✅ Supports high-resolution encoders (>10,000 PPR)
- ✅ Hardware quadrature decoding
- ✅ No I2C overhead
- ✅ Deterministic timing

### ✅ Cost Savings

**Component cost:**
- Dual ADS1015: $7.65
- Single ADS1015 + comparators: $5.35
- **Savings: $2.30 (30% cheaper)**

### ✅ Performance Improvement

**Encoder latency:**
- ADS1015: 1600 µs
- GPIO: 10 µs
- **Improvement: 160× faster!**

**Supported encoder resolution:**
- ADS1015: Up to 100 PPR (marginal at 500 PPR)
- GPIO: Up to 10,000 PPR (excellent margin)
- **Improvement: 100× higher resolution**

---

## Summary

**Optimized hybrid system provides:**

✅ **Carriage position detection** - Analog sensing with polarity identification (1.6ms latency)  
✅ **Ultra-fast encoder tracking** - 10µs latency, supports >10,000 PPR  
✅ **Belt phase monitoring** - 10µs latency, >99.9% margin  
✅ **Lower cost** - $5.35 vs $7.65 (30% savings)  
✅ **Better performance** - 160× faster encoder response  
✅ **Scalable** - Can add more GPIO encoders if needed  

**GPIO pins used:** 6 total (SDA, SCL, ALERT, Belt Phase, Encoder A, Encoder B)

**Recommended for:** Applications requiring high-resolution position tracking with fast response times while maintaining analog carriage identification capability.
