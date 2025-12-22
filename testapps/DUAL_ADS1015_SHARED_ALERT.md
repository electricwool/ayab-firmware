# Dual ADS1015 with Shared ALERT Pin

## Overview
Circuit design using **two ADS1015 modules** (5 sensor inputs total) with a **single shared ALERT/GPIO pin** on the RP2040. The ADS1015 is a 12-bit, 3.3kSPS ADC - faster and cheaper than ADS1115 (16-bit, 860SPS).

**Key Configuration:**
- **ADS1015 #1**: 2 inputs configured as **2 differential comparators** using internal MUX
  - Comparator 1: AIN0 - AIN1 (detects when Sensor 1 > Sensor 2)
  - Comparator 2: AIN1 - AIN0 (detects when Sensor 2 > Sensor 1)
- **ADS1015 #2**: 3 inputs configured as **single-ended** (3 independent channels)
- **Total**: 5 sensor inputs monitored (2 differential + 3 single-ended)

**Key Insight:** By switching the internal MUX between `AIN0-AIN1` and `AIN1-AIN0`, one ADC acts as **TWO independent comparators**!

---

## Why ADS1015 Instead of ADS1115?

| Feature | ADS1015 | ADS1115 | Advantage |
|---------|---------|---------|-----------|
| **Resolution** | 12-bit (4096 levels) | 16-bit (65536 levels) | ADS1115 |
| **Sample Rate** | 3300 SPS | 860 SPS | **ADS1015 (3.8× faster)** |
| **Price** | ~$3.50 | ~$5.00 | **ADS1015 (30% cheaper)** |
| **Power** | 150µA | 150µA | Equal |
| **Accuracy** | ±3 LSB | ±2 LSB | ADS1115 |

**For hall sensor detection:**
- 12-bit resolution = 2mV per step @ ±4.096V range
- Sensor voltage swing: 1.63V - 1.79V (from measurements)
- Detection margin: >800 ADC counts - **plenty of resolution**
- **Faster sampling = better response time**

✅ **ADS1015 is ideal for this application**

---

## Circuit Schematic

```
Differential Sensor Pair (K-Carriage Left & Right)
    Sensor 1 (0V / 1.68V / 3.47V)
        |
        +---[R1: 100k]---+---[C1: 10nF]---GND
                         |
                         +---> AIN0 (ADS1015 #1)
                         
    Sensor 2 (0V / 1.68V / 3.47V)
        |
        +---[R2: 100k]---+---[C2: 10nF]---GND
                         |
                         +---> AIN1 (ADS1015 #1)

    ADS1015 #1 (Address 0x48) - DUAL DIFFERENTIAL COMPARATORS
   +----------------+
   |VDD         SDA|---+
   |GND         SCL|---+
   |A0        ALERT|---+
   |A1          ADD|---GND (addr 0x48)
   |A2             |
   |A3             |
   +----------------+
   
   Internal MUX Configuration (firmware switches between):
   - Comparator 1: AIN0 - AIN1 (detects Sensor 1 > Sensor 2)
   - Comparator 2: AIN1 - AIN0 (detects Sensor 2 > Sensor 1)
   
   By switching MUX between readings, one ADC acts as TWO comparators!


Single-Ended Sensors (Lace Carriage + Spare)
    Sensor 3 (0V / 1.68V / 3.47V)
        |
        +---[R3: 100k]---+---[C3: 10nF]---GND
                         |
                         +---> AIN0 (ADS1015 #2)
                         
    Sensor 4 (0V / 1.68V / 3.47V)
        |
        +---[R4: 100k]---+---[C4: 10nF]---GND
                         |
                         +---> AIN1 (ADS1015 #2)
                         
    Sensor 5 (0V / 1.68V / 3.47V)
        |
        +---[R5: 100k]---+---[C5: 10nF]---GND
                         |
                         +---> AIN2 (ADS1015 #2)

    ADS1015 #2 (Address 0x49) - SINGLE-ENDED
   +----------------+
   |VDD         SDA|---+
   |GND         SCL|---+
   |A0        ALERT|---+
   |A1          ADD|---+--- VDD (addr 0x49)
   |A2             |   |
   |A3             |   |
   +----------------+   |
                        |
                        +--- [R6: 10k] --- 3.3V
                        |
    RP2040 I2C Bus      |
    SDA (GP4) <---------+
    SCL (GP5) <---------+
    ALERT_PIN (GP6) <---+
    
    I2C Pull-ups: 4.7kΩ on SDA and SCL to 3.3V
    ALERT Pull-up: 10kΩ to 3.3V (shared by both ADS1015)
```

---

## How the Dual Differential Comparator Works

### ADS1015 Internal MUX

The ADS1015 has a **programmable multiplexer** that can select different input configurations:

| MUX Setting | Measures | Use Case |
|-------------|----------|----------|
| `000` | AIN0 - AIN1 | Differential: Sensor 1 vs Sensor 2 |
| `001` | AIN0 - AIN3 | Differential: Other pair |
| `010` | AIN1 - AIN3 | Differential: Other pair |
| `011` | AIN2 - AIN3 | Differential: Other pair |
| `100` | AIN0 - GND | Single-ended: AIN0 |
| `101` | AIN1 - GND | Single-ended: AIN1 |
| `110` | AIN2 - GND | Single-ended: AIN2 |
| `111` | AIN3 - GND | Single-ended: AIN3 |

**Key Discovery:** MUX setting `011` (binary) = `AIN1 - AIN0` (reversed differential)!

### Creating Two Comparators from One ADC

**Configuration 1:** MUX = `000` → Measures `AIN0 - AIN1`
- When Sensor 1 (AIN0) goes HIGH: differential = +1.79V → **ALERT triggers**
- Threshold: +1.0V
- Detects: Sensor 1 activation

**Configuration 2:** MUX = `011` → Measures `AIN1 - AIN0`  
- When Sensor 2 (AIN1) goes HIGH: differential = +1.79V → **ALERT triggers**
- Threshold: +1.0V
- Detects: Sensor 2 activation

**Example Readings:**

| Sensor 1 (AIN0) | Sensor 2 (AIN1) | AIN0-AIN1 | AIN1-AIN0 | Which Triggers? |
|-----------------|-----------------|-----------|-----------|-----------------|
| 1.68V (inactive) | 1.68V (inactive) | 0V | 0V | Neither |
| 3.47V (active) | 1.68V (inactive) | **+1.79V** ✓ | -1.79V | Comparator 1 |
| 1.68V (inactive) | 3.47V (active) | -1.79V | **+1.79V** ✓ | Comparator 2 |
| 3.47V (active) | 3.47V (active) | 0V | 0V | Neither |

**Firmware Operation:**
1. Configure MUX to `AIN0-AIN1`, set threshold to +1.0V
2. When Sensor 1 goes HIGH → ALERT triggers
3. Firmware reads, detects Sensor 1 active
4. Configure MUX to `AIN1-AIN0`, set threshold to +1.0V  
5. When Sensor 2 goes HIGH → ALERT triggers
6. Firmware reads, detects Sensor 2 active

**Result:** Two independent comparators monitoring two sensors with one ADC chip!

---

## Differential vs Single-Ended Configuration

### ADS1015 #1: Dual Differential Comparator Mode

**Threshold Settings (for both MUX configs):**
- **Upper threshold:** +1.0V (sensor active)
- **Lower threshold:** -0.5V (below inactive)
- **Window:** -0.5V to +1.0V (inactive state)

**Advantages:**
1. **Independent monitoring** - each sensor tracked separately
2. **Noise immunity** - common-mode noise cancels out in differential mode
3. **Direction detection** - know which sensor triggered first
4. **Efficient** - 2 sensors monitored with 1 ADC chip

### ADS1015 #2: Single-Ended Mode

**Configuration:** `AIN0`, `AIN1`, `AIN2` (three independent channels)

**Measures:** Absolute voltage of each sensor vs GND

**Use Cases:**
1. **Independent sensors** - lace carriage left/right
2. **Absolute position** - detect HIGH or LOW state
3. **Spare channel** - future expansion

**Threshold Settings (per channel):**
- **Lower threshold:** 1.0V (below inactive)
- **Upper threshold:** 2.5V (above inactive)
- **Window:** 1.0V to 2.5V (inactive state)

---

## Components Required

| Qty | Part | Description | Unit Price | Total |
|-----|------|-------------|------------|-------|
| 2 | ADS1015 | 12-bit ADC Module | $3.50 | $7.00 |
| 5 | 100kΩ | Input protection resistors | $0.05 | $0.25 |
| 5 | 10nF | Input filtering capacitors | $0.05 | $0.25 |
| 2 | 4.7kΩ | I2C pull-up resistors | $0.05 | $0.10 |
| 1 | 10kΩ | ALERT pull-up resistor | $0.05 | $0.05 |
| **TOTAL** | | | | **$7.65** |

**Cost Savings vs ADS1115:** $10.55 - $7.65 = **$2.90 saved (27% cheaper)**

---

## RP2040 Firmware - Dual ADS1015

### Complete Implementation

```cpp
// Dual ADS1015 with Shared ALERT Pin
// ADS1015 #1: Dual differential comparators (2 inputs)
// ADS1015 #2: Single-ended (3 inputs)
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

// ADS1015 Addresses
#define ADS1015_ADDR_1 0x48  // ADDR → GND (differential)
#define ADS1015_ADDR_2 0x49  // ADDR → VDD (single-ended)

// Shared ALERT pin
#define ALERT_PIN 6

// ADS1015 Registers (same as ADS1115)
#define ADS1015_REG_CONVERSION  0x00
#define ADS1015_REG_CONFIG      0x01
#define ADS1015_REG_LO_THRESH   0x02
#define ADS1015_REG_HI_THRESH   0x03

// Config bits
#define ADS1015_CONFIG_OS_SINGLE    0x8000
#define ADS1015_CONFIG_MUX_AIN0_AIN1 0x0000  // Differential: AIN0 - AIN1
#define ADS1015_CONFIG_MUX_AIN1_AIN0 0x3000  // Differential: AIN1 - AIN0 (reversed!)
#define ADS1015_CONFIG_MUX_AIN0_GND 0x4000   // Single-ended: AIN0
#define ADS1015_CONFIG_MUX_AIN1_GND 0x5000   // Single-ended: AIN1
#define ADS1015_CONFIG_MUX_AIN2_GND 0x6000   // Single-ended: AIN2
#define ADS1015_CONFIG_PGA_4_096V   0x0200
#define ADS1015_CONFIG_MODE_CONT    0x0000
#define ADS1015_CONFIG_DR_1600SPS   0x0080   // 1600 SPS for ADS1015
#define ADS1015_CONFIG_COMP_MODE_WINDOW 0x0010
#define ADS1015_CONFIG_COMP_POL_LOW 0x0000
#define ADS1015_CONFIG_COMP_LAT_ON  0x0004
#define ADS1015_CONFIG_COMP_QUE_1   0x0000

// Voltage thresholds (12-bit ADC counts for ±4.096V range)
// ADS1015: 12-bit = 4096 counts, 2mV per count
// For differential: ±4.096V range = -2048 to +2047 counts

// Differential thresholds (ADS1015 #1)
// Both comparators use same threshold since we reverse the MUX
#define DIFF_THRESHOLD_LOW   -250   // -0.5V (below inactive)
#define DIFF_THRESHOLD_HIGH  +500   // +1.0V (sensor active)

// Single-ended thresholds (ADS1015 #2)
#define SE_THRESHOLD_LOW     500    // 1.0V
#define SE_THRESHOLD_HIGH    1250   // 2.5V

// State definitions
typedef enum {
  STATE_LOW = 0,
  STATE_INACTIVE = 1,
  STATE_HIGH = 2,
  STATE_ERROR = 3
} SensorState;

// Differential sensor states (independent)
typedef struct {
  SensorState sensor1;  // AIN0
  SensorState sensor2;  // AIN1
} DiffSensors;

// Global state
volatile bool alert_triggered = false;
DiffSensors diff_sensors = {STATE_ERROR, STATE_ERROR};
SensorState sensor_states[3] = {STATE_ERROR, STATE_ERROR, STATE_ERROR};

// I2C write register
void ads1015_write_register(uint8_t addr, uint8_t reg, uint16_t value) {
  uint8_t data[3] = {reg, (value >> 8) & 0xFF, value & 0xFF};
  i2c_write_blocking(I2C_PORT, addr, data, 3, false);
}

// I2C read register
uint16_t ads1015_read_register(uint8_t addr, uint8_t reg) {
  uint8_t data[2];
  i2c_write_blocking(I2C_PORT, addr, &reg, 1, true);
  i2c_read_blocking(I2C_PORT, addr, data, 2, false);
  return (data[0] << 8) | data[1];
}

// Configure comparator thresholds
void ads1015_set_thresholds(uint8_t addr, int16_t lo_thresh, int16_t hi_thresh) {
  // ADS1015 uses 12-bit values left-shifted by 4 bits
  int16_t lo_shifted = lo_thresh << 4;
  int16_t hi_shifted = hi_thresh << 4;
  ads1015_write_register(addr, ADS1015_REG_LO_THRESH, lo_shifted);
  ads1015_write_register(addr, ADS1015_REG_HI_THRESH, hi_shifted);
}

// Configure ADS1015 #1 - Differential comparator (specify which MUX)
void ads1015_configure_differential(uint8_t addr, uint16_t mux_config) {
  uint16_t config = ADS1015_CONFIG_MODE_CONT |
                    ADS1015_CONFIG_PGA_4_096V |
                    ADS1015_CONFIG_DR_1600SPS |
                    ADS1015_CONFIG_COMP_MODE_WINDOW |
                    ADS1015_CONFIG_COMP_POL_LOW |
                    ADS1015_CONFIG_COMP_LAT_ON |
                    ADS1015_CONFIG_COMP_QUE_1 |
                    mux_config;  // AIN0-AIN1 or AIN1-AIN0
  
  ads1015_write_register(addr, ADS1015_REG_CONFIG, config);
  ads1015_set_thresholds(addr, DIFF_THRESHOLD_LOW, DIFF_THRESHOLD_HIGH);
}

// Configure ADS1015 #2 - Single-ended channel
void ads1015_configure_single_ended(uint8_t addr, uint8_t channel) {
  uint16_t config = ADS1015_CONFIG_MODE_CONT |
                    ADS1015_CONFIG_PGA_4_096V |
                    ADS1015_CONFIG_DR_1600SPS |
                    ADS1015_CONFIG_COMP_MODE_WINDOW |
                    ADS1015_CONFIG_COMP_POL_LOW |
                    ADS1015_CONFIG_COMP_LAT_ON |
                    ADS1015_CONFIG_COMP_QUE_1;
  
  // Set MUX for channel
  switch (channel) {
    case 0: config |= ADS1015_CONFIG_MUX_AIN0_GND; break;
    case 1: config |= ADS1015_CONFIG_MUX_AIN1_GND; break;
    case 2: config |= ADS1015_CONFIG_MUX_AIN2_GND; break;
  }
  
  ads1015_write_register(addr, ADS1015_REG_CONFIG, config);
  ads1015_set_thresholds(addr, SE_THRESHOLD_LOW, SE_THRESHOLD_HIGH);
}

// Read ADC value (12-bit, right-justified)
int16_t ads1015_read_adc(uint8_t addr) {
  uint16_t raw = ads1015_read_register(addr, ADS1015_REG_CONVERSION);
  // ADS1015 returns 12-bit value left-shifted by 4 bits
  return (int16_t)raw >> 4;
}

// Convert differential ADC to sensor state
// When reading AIN0-AIN1: positive value means AIN0 > AIN1 (sensor 1 active)
// When reading AIN1-AIN0: positive value means AIN1 > AIN0 (sensor 2 active)
SensorState diff_adc_to_state(int16_t adc_value) {
  if (adc_value > DIFF_THRESHOLD_HIGH) {
    return STATE_HIGH;  // Differential voltage high (sensor active)
  } else if (adc_value < DIFF_THRESHOLD_LOW) {
    return STATE_LOW;   // Differential voltage low (opposite sensor active)
  } else {
    return STATE_INACTIVE;  // Within window (both same state)
  }
}

// Convert single-ended ADC to state
SensorState se_adc_to_state(int16_t adc_value) {
  if (adc_value < 0) return STATE_ERROR;
  if (adc_value < SE_THRESHOLD_LOW) return STATE_LOW;
  if (adc_value < SE_THRESHOLD_HIGH) return STATE_INACTIVE;
  return STATE_HIGH;
}

// Convert ADC to millivolts (12-bit, ±4.096V range)
int16_t adc_to_mv(int16_t adc_value) {
  // 12-bit: 2mV per count
  return (adc_value * 2);
}

// Check which ADS1015 triggered the alert (reads current MUX setting)
bool ads1015_check_alert(uint8_t addr, bool is_differential) {
  int16_t adc = ads1015_read_adc(addr);
  
  // Check if value is outside window
  if (is_differential) {
    // Differential mode
    return (adc < DIFF_THRESHOLD_LOW || adc > DIFF_THRESHOLD_HIGH);
  } else {
    // Single-ended mode
    return (adc < SE_THRESHOLD_LOW || adc > SE_THRESHOLD_HIGH);
  }
}

// GPIO interrupt handler for shared ALERT pin
void alert_irq_handler(uint gpio, uint32_t events) {
  if (gpio == ALERT_PIN && (events & GPIO_IRQ_EDGE_FALL)) {
    alert_triggered = true;
  }
}

// Send state update
void send_state_update() {
  // Pack 5 sensor states (2 bits each) = 10 bits (use 2 bytes)
  uint16_t statusWord = (diff_sensors.sensor1 << 8) | 
                        (diff_sensors.sensor2 << 6) |
                        (sensor_states[0] << 4) |
                        (sensor_states[1] << 2) |
                        (sensor_states[2] << 0);
  
  printf("State: 0x%04X (S1=%d, S2=%d, S3=%d, S4=%d, S5=%d)\n", 
         statusWord, diff_sensors.sensor1, diff_sensors.sensor2,
         sensor_states[0], sensor_states[1], sensor_states[2]);
}

// Initialize I2C
void init_i2c() {
  i2c_init(I2C_PORT, I2C_FREQ);
  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA);
  gpio_pull_up(I2C_SCL);
}

// Initialize ALERT pin
void init_alert_pin() {
  gpio_init(ALERT_PIN);
  gpio_set_dir(ALERT_PIN, GPIO_IN);
  gpio_pull_up(ALERT_PIN);
  
  gpio_set_irq_enabled_with_callback(ALERT_PIN, GPIO_IRQ_EDGE_FALL, 
                                     true, &alert_irq_handler);
}

// Initialize ADS1015
bool init_ads1015(uint8_t addr) {
  uint16_t config = ads1015_read_register(addr, ADS1015_REG_CONFIG);
  
  if (config == 0x0000 || config == 0xFFFF) {
    printf("ERROR: ADS1015 at 0x%02X not found\n", addr);
    return false;
  }
  
  printf("ADS1015 at 0x%02X initialized\n", addr);
  return true;
}

int main() {
  stdio_init_all();
  sleep_ms(2000);
  
  printf("Dual ADS1015 with Shared ALERT Pin\n");
  printf("ADS1015 #1: Dual differential comparators (2 sensors)\n");
  printf("ADS1015 #2: Single-ended (3 channels)\n\n");
  
  // Initialize hardware
  init_i2c();
  init_alert_pin();
  
  // Initialize both ADS1015 modules
  if (!init_ads1015(ADS1015_ADDR_1) || !init_ads1015(ADS1015_ADDR_2)) {
    while (1) sleep_ms(1000);
  }
  
  // Configure ADS1015 #1 - Differential Comparator 1 (AIN0-AIN1)
  ads1015_configure_differential(ADS1015_ADDR_1, ADS1015_CONFIG_MUX_AIN0_AIN1);
  sleep_ms(10);
  int16_t diff_adc1 = ads1015_read_adc(ADS1015_ADDR_1);
  diff_sensors.sensor1 = diff_adc_to_state(diff_adc1);
  printf("S1 (AIN0-AIN1) initial: %dmV (%d)\n", adc_to_mv(diff_adc1), diff_sensors.sensor1);
  
  // Configure ADS1015 #1 - Differential Comparator 2 (AIN1-AIN0)
  ads1015_configure_differential(ADS1015_ADDR_1, ADS1015_CONFIG_MUX_AIN1_AIN0);
  sleep_ms(10);
  int16_t diff_adc2 = ads1015_read_adc(ADS1015_ADDR_1);
  diff_sensors.sensor2 = diff_adc_to_state(diff_adc2);
  printf("S2 (AIN1-AIN0) initial: %dmV (%d)\n", adc_to_mv(diff_adc2), diff_sensors.sensor2);
  
  // Configure ADS1015 #2 - Single-ended channels
  const char* sensor_names[] = {"S3", "S4", "S5"};
  for (int i = 0; i < 3; i++) {
    ads1015_configure_single_ended(ADS1015_ADDR_2, i);
    sleep_ms(10);
    int16_t adc = ads1015_read_adc(ADS1015_ADDR_2);
    sensor_states[i] = se_adc_to_state(adc);
    printf("%s initial: %dmV (%d)\n", sensor_names[i], adc_to_mv(adc), sensor_states[i]);
  }
  
  send_state_update();
  
  // Main loop - event-driven with shared ALERT
  while (1) {
    if (alert_triggered) {
      alert_triggered = false;
      
      bool state_changed = false;
      
      // Check ADS1015 #1 (Dual Differential Comparators)
      // Must check both MUX configurations
      
      // Check Comparator 1: AIN0-AIN1 (Sensor 1)
      ads1015_configure_differential(ADS1015_ADDR_1, ADS1015_CONFIG_MUX_AIN0_AIN1);
      sleep_ms(1);
      if (ads1015_check_alert(ADS1015_ADDR_1, true)) {
        int16_t diff_adc1 = ads1015_read_adc(ADS1015_ADDR_1);
        SensorState new_state1 = diff_adc_to_state(diff_adc1);
        
        if (new_state1 != diff_sensors.sensor1) {
          printf("S1: %dmV (%d->%d)\n", adc_to_mv(diff_adc1), 
                 diff_sensors.sensor1, new_state1);
          diff_sensors.sensor1 = new_state1;
          state_changed = true;
        }
      }
      
      // Check Comparator 2: AIN1-AIN0 (Sensor 2)
      ads1015_configure_differential(ADS1015_ADDR_1, ADS1015_CONFIG_MUX_AIN1_AIN0);
      sleep_ms(1);
      if (ads1015_check_alert(ADS1015_ADDR_1, true)) {
        int16_t diff_adc2 = ads1015_read_adc(ADS1015_ADDR_1);
        SensorState new_state2 = diff_adc_to_state(diff_adc2);
        
        if (new_state2 != diff_sensors.sensor2) {
          printf("S2: %dmV (%d->%d)\n", adc_to_mv(diff_adc2), 
                 diff_sensors.sensor2, new_state2);
          diff_sensors.sensor2 = new_state2;
          state_changed = true;
        }
      }
      
      // Clear latch
      ads1015_read_register(ADS1015_ADDR_1, ADS1015_REG_CONFIG);
      
      // Check ADS1015 #2 (Single-ended)
      for (int i = 0; i < 3; i++) {
        ads1015_configure_single_ended(ADS1015_ADDR_2, i);
        sleep_ms(1);  // Brief settling time
        
        if (ads1015_check_alert(ADS1015_ADDR_2, false)) {
          int16_t adc = ads1015_read_adc(ADS1015_ADDR_2);
          SensorState new_state = se_adc_to_state(adc);
          
          if (new_state != sensor_states[i]) {
            printf("%s: %dmV (%d->%d)\n", sensor_names[i], adc_to_mv(adc), 
                   sensor_states[i], new_state);
            sensor_states[i] = new_state;
            state_changed = true;
          }
        }
      }
      
      // Clear latch
      ads1015_read_register(ADS1015_ADDR_2, ADS1015_REG_CONFIG);
      
      // Send update if any state changed
      if (state_changed) {
        send_state_update();
      }
    }
    
    // Sleep until next interrupt
    __wfi();
  }
  
  return 0;
}
```

---

## Dual Differential Comparator Advantages

### Why Use Two Differential Comparators for K-Carriage Sensors?

**Problem:** K-carriage has two hall sensors (left and right) that need independent monitoring.

**Traditional Approach:** Use 2 ADC channels (one per sensor)
- Requires 2 channels
- No noise immunity
- Susceptible to common-mode interference

**Dual Differential Approach:** Use 1 ADC with MUX switching
- **Independent monitoring** - each sensor tracked separately
- **Noise immunity** - differential mode cancels common-mode noise
- **Efficient** - 2 sensors with 1 ADC chip
- **Direction detection** - know which sensor triggered first

**Example Carriage Movement:**

```
Carriage moving LEFT to RIGHT:
Time 0: Both inactive → Both comparators inactive
Time 1: Left sensor (S1) triggers → Comparator 1 ALERT!
Time 2: Both active → Both comparators inactive (differential = 0)
Time 3: Right sensor (S2) triggers →