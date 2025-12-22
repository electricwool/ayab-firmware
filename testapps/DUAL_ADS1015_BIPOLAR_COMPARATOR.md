# Dual ADS1015 Bipolar Hall Sensor Comparators

## Overview
Circuit design using **two ADS1015 modules** to detect **bipolar hall sensor states** (North/South pole detection) with a **single shared ALERT/GPIO pin** on the RP2040.

**Key Requirement:** Each hall sensor outputs a **single analog voltage** that varies based on magnetic field polarity:
- **South pole (LOW):** ~0V - 0.5V
- **No magnet (INACTIVE):** ~1.68V (mid-range)
- **North pole (HIGH):** ~3.0V - 3.47V

**Solution:** Use ADS1015 differential comparators with **two threshold windows** per sensor to detect all three states.

---

## Hall Sensor Behavior (Bipolar)

### Typical Bipolar Hall Sensor Output

```
Voltage vs Magnetic Field:

3.47V ─────────────────────────────── North Pole (HIGH)
      │
2.5V  ├─────────── Upper Threshold
      │
1.68V ├─────────── No Magnet (INACTIVE)
      │
1.0V  ├─────────── Lower Threshold
      │
0V    ─────────────────────────────── South Pole (LOW)
```

**Single Wire Per Sensor:**
- One analog output wire per hall sensor
- Voltage indicates magnetic polarity
- Three distinct states to detect

---

## Detection Strategy

### Problem: Detecting 3 States with Window Comparator

The ADS1015 comparator has **one window** (low threshold to high threshold). To detect 3 states, we need **two windows**:

1. **Window 1:** Detect if voltage is BELOW inactive range (South pole)
2. **Window 2:** Detect if voltage is ABOVE inactive range (North pole)

### Solution: Use Differential Input with Reference Voltage

**For each sensor, we need 2 differential channels:**

**Channel 1:** `Sensor - GND` (absolute voltage measurement)
- Detects if sensor voltage > upper threshold (North pole)
- Detects if sensor voltage < lower threshold (South pole)

**Channel 2:** Not needed if we use traditional comparator mode

**Better Solution:** Use **two ADS1015 comparators per sensor** by utilizing the MUX:

---

## Circuit Configuration

### ADS1015 #1: Two Bipolar Sensors (2 wires)

**Sensor 1 (K-Carriage Left):**
- **Input:** Single wire from hall sensor → AIN0
- **Reference:** GND
- **Detection:** Use window comparator to detect outside inactive range

**Sensor 2 (K-Carriage Right):**
- **Input:** Single wire from hall sensor → AIN1
- **Reference:** GND
- **Detection:** Use window comparator to detect outside inactive range

### ADS1015 #2: Three Bipolar Sensors (3 wires)

**Sensor 3 (Lace Left):** AIN0 → GND  
**Sensor 4 (Lace Right):** AIN1 → GND  
**Sensor 5 (Spare):** AIN2 → GND

---

## Circuit Schematic

```
Bipolar Hall Sensors (Single Wire Each)
    
    Sensor 1 - K-Carriage Left (0V / 1.68V / 3.47V)
        |
        +---[R1: 100k]---+---[C1: 10nF]---GND
                         |
                         +---> AIN0 (ADS1015 #1)
                         
    Sensor 2 - K-Carriage Right (0V / 1.68V / 3.47V)
        |
        +---[R2: 100k]---+---[C2: 10nF]---GND
                         |
                         +---> AIN1 (ADS1015 #1)

    ADS1015 #1 (Address 0x48) - 2 BIPOLAR SENSORS
   +----------------+
   |VDD         SDA|---+
   |GND         SCL|---+
   |A0        ALERT|---+
   |A1          ADD|---GND (addr 0x48)
   |A2             |
   |A3             |
   +----------------+
   
   Configuration:
   - AIN0 vs GND: Sensor 1 (window comparator)
   - AIN1 vs GND: Sensor 2 (window comparator)


    Sensor 3 - Lace Left (0V / 1.68V / 3.47V)
        |
        +---[R3: 100k]---+---[C3: 10nF]---GND
                         |
                         +---> AIN0 (ADS1015 #2)
                         
    Sensor 4 - Lace Right (0V / 1.68V / 3.47V)
        |
        +---[R4: 100k]---+---[C4: 10nF]---GND
                         |
                         +---> AIN1 (ADS1015 #2)
                         
    Sensor 5 - Spare (0V / 1.68V / 3.47V)
        |
        +---[R5: 100k]---+---[C5: 10nF]---GND
                         |
                         +---> AIN2 (ADS1015 #2)

    ADS1015 #2 (Address 0x49) - 3 BIPOLAR SENSORS
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

## Three-State Detection Method

### Window Comparator Configuration

**Thresholds:**
- **Lower Threshold:** 1.0V (below inactive)
- **Upper Threshold:** 2.5V (above inactive)
- **Window:** 1.0V to 2.5V (inactive range)

**Comparator Mode:** Window mode with ALERT active when **outside window**

### State Detection Logic

When ALERT triggers, read ADC value:

| ADC Reading | Voltage | State | Interpretation |
|-------------|---------|-------|----------------|
| < 500 counts | < 1.0V | **LOW** | South pole detected |
| 500-1250 counts | 1.0V-2.5V | **INACTIVE** | No magnet (shouldn't trigger ALERT) |
| > 1250 counts | > 2.5V | **HIGH** | North pole detected |

**Firmware Logic:**
```cpp
if (adc_value < THRESHOLD_LOW) {
    state = STATE_LOW;  // South pole
} else if (adc_value > THRESHOLD_HIGH) {
    state = STATE_HIGH;  // North pole
} else {
    state = STATE_INACTIVE;  // No magnet (within window)
}
```

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

---

## RP2040 Firmware - Bipolar Hall Sensor Detection

### Complete Implementation

```cpp
// Dual ADS1015 Bipolar Hall Sensor Comparators
// Detects North/South pole with single wire per sensor
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
#define ADS1015_ADDR_1 0x48  // ADDR → GND (sensors 1-2)
#define ADS1015_ADDR_2 0x49  // ADDR → VDD (sensors 3-5)

// Shared ALERT pin
#define ALERT_PIN 6

// ADS1015 Registers
#define ADS1015_REG_CONVERSION  0x00
#define ADS1015_REG_CONFIG      0x01
#define ADS1015_REG_LO_THRESH   0x02
#define ADS1015_REG_HI_THRESH   0x03

// Config bits
#define ADS1015_CONFIG_OS_SINGLE    0x8000
#define ADS1015_CONFIG_MUX_AIN0_GND 0x4000   // Single-ended: AIN0
#define ADS1015_CONFIG_MUX_AIN1_GND 0x5000   // Single-ended: AIN1
#define ADS1015_CONFIG_MUX_AIN2_GND 0x6000   // Single-ended: AIN2
#define ADS1015_CONFIG_PGA_4_096V   0x0200
#define ADS1015_CONFIG_MODE_CONT    0x0000
#define ADS1015_CONFIG_DR_1600SPS   0x0080   // 1600 SPS for ADS1015
#define ADS1015_CONFIG_COMP_MODE_WINDOW 0x0010  // Window comparator mode
#define ADS1015_CONFIG_COMP_POL_LOW 0x0000
#define ADS1015_CONFIG_COMP_LAT_ON  0x0004
#define ADS1015_CONFIG_COMP_QUE_1   0x0000

// Voltage thresholds (12-bit ADC counts for ±4.096V range)
// ADS1015: 12-bit = 4096 counts, 2mV per count @ 4.096V range
// For single-ended: 0 to 4096 counts (0V to 4.096V)

// Bipolar hall sensor thresholds
#define THRESHOLD_LOW     500    // 1.0V (below = South pole)
#define THRESHOLD_HIGH    1250   // 2.5V (above = North pole)
// Window: 1.0V to 2.5V (inactive state)

// State definitions
typedef enum {
  STATE_LOW = 0,        // South pole
  STATE_INACTIVE = 1,   // No magnet
  STATE_HIGH = 2,       // North pole
  STATE_ERROR = 3
} SensorState;

// Global state
volatile bool alert_triggered = false;
SensorState sensor_states[5] = {STATE_ERROR, STATE_ERROR, STATE_ERROR, 
                                STATE_ERROR, STATE_ERROR};

// Sensor names for debugging
const char* sensor_names[] = {"S1_KLeft", "S2_KRight", "S3_LLeft", 
                              "S4_LRight", "S5_Spare"};

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

// Configure ADS1015 channel with window comparator
void ads1015_configure_channel(uint8_t addr, uint8_t channel) {
  uint16_t config = ADS1015_CONFIG_MODE_CONT |
                    ADS1015_CONFIG_PGA_4_096V |
                    ADS1015_CONFIG_DR_1600SPS |
                    ADS1015_CONFIG_COMP_MODE_WINDOW |  // Window mode
                    ADS1015_CONFIG_COMP_POL_LOW |
                    ADS1015_CONFIG_COMP_LAT_ON |
                    ADS1015_CONFIG_COMP_QUE_1;
  
  // Set MUX for channel (single-ended)
  switch (channel) {
    case 0: config |= ADS1015_CONFIG_MUX_AIN0_GND; break;
    case 1: config |= ADS1015_CONFIG_MUX_AIN1_GND; break;
    case 2: config |= ADS1015_CONFIG_MUX_AIN2_GND; break;
  }
  
  ads1015_write_register(addr, ADS1015_REG_CONFIG, config);
  
  // Set window thresholds (ALERT when outside 1.0V-2.5V)
  ads1015_set_thresholds(addr, THRESHOLD_LOW, THRESHOLD_HIGH);
}

// Read ADC value (12-bit, right-justified)
int16_t ads1015_read_adc(uint8_t addr) {
  uint16_t raw = ads1015_read_register(addr, ADS1015_REG_CONVERSION);
  // ADS1015 returns 12-bit value left-shifted by 4 bits
  return (int16_t)raw >> 4;
}

// Convert ADC to bipolar state
SensorState adc_to_state(int16_t adc_value) {
  if (adc_value < 0) return STATE_ERROR;
  
  if (adc_value < THRESHOLD_LOW) {
    return STATE_LOW;  // South pole (< 1.0V)
  } else if (adc_value > THRESHOLD_HIGH) {
    return STATE_HIGH;  // North pole (> 2.5V)
  } else {
    return STATE_INACTIVE;  // No magnet (1.0V - 2.5V)
  }
}

// Convert ADC to millivolts
int16_t adc_to_mv(int16_t adc_value) {
  // 12-bit @ 4.096V range: 1mV per count
  return adc_value;
}

// Check if channel triggered alert
bool ads1015_check_alert(uint8_t addr) {
  int16_t adc = ads1015_read_adc(addr);
  // Alert triggers when outside window
  return (adc < THRESHOLD_LOW || adc > THRESHOLD_HIGH);
}

// GPIO interrupt handler for shared ALERT pin
void alert_irq_handler(uint gpio, uint32_t events) {
  if (gpio == ALERT_PIN && (events & GPIO_IRQ_EDGE_FALL)) {
    alert_triggered = true;
  }
}

// Send state update via serial
void send_state_update() {
  // Pack 5 sensor states (2 bits each) = 10 bits
  uint16_t statusWord = (sensor_states[0] << 8) | 
                        (sensor_states[1] << 6) |
                        (sensor_states[2] << 4) |
                        (sensor_states[3] << 2) |
                        (sensor_states[4] << 0);
  
  printf("State: 0x%04X ", statusWord);
  for (int i = 0; i < 5; i++) {
    const char* state_str;
    switch (sensor_states[i]) {
      case STATE_LOW: state_str = "SOUTH"; break;
      case STATE_INACTIVE: state_str = "NONE"; break;
      case STATE_HIGH: state_str = "NORTH"; break;
      default: state_str = "ERROR"; break;
    }
    printf("%s=%s ", sensor_names[i], state_str);
  }
  printf("\n");
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
  
  printf("Dual ADS1015 Bipolar Hall Sensor Comparators\n");
  printf("5 sensors: 2 on ADS1015 #1, 3 on ADS1015 #2\n");
  printf("Detection: South pole / No magnet / North pole\n\n");
  
  // Initialize hardware
  init_i2c();
  init_alert_pin();
  
  // Initialize both ADS1015 modules
  if (!init_ads1015(ADS1015_ADDR_1) || !init_ads1015(ADS1015_ADDR_2)) {
    while (1) sleep_ms(1000);
  }
  
  // Configure ADS1015 #1 - Sensors 1-2
  for (int ch = 0; ch < 2; ch++) {
    ads1015_configure_channel(ADS1015_ADDR_1, ch);
    sleep_ms(10);
    int16_t adc = ads1015_read_adc(ADS1015_ADDR_1);
    sensor_states[ch] = adc_to_state(adc);
    printf("%s initial: %dmV (%d)\n", sensor_names[ch], 
           adc_to_mv(adc), sensor_states[ch]);
  }
  
  // Configure ADS1015 #2 - Sensors 3-5
  for (int ch = 0; ch < 3; ch++) {
    ads1015_configure_channel(ADS1015_ADDR_2, ch);
    sleep_ms(10);
    int16_t adc = ads1015_read_adc(ADS1015_ADDR_2);
    sensor_states[ch + 2] = adc_to_state(adc);
    printf("%s initial: %dmV (%d)\n", sensor_names[ch + 2], 
           adc_to_mv(adc), sensor_states[ch + 2]);
  }
  
  send_state_update();
  
  // Main loop - event-driven with shared ALERT
  while (1) {
    if (alert_triggered) {
      alert_triggered = false;
      
      bool state_changed = false;
      
      // Check ADS1015 #1 (Sensors 1-2)
      for (int ch = 0; ch < 2; ch++) {
        ads1015_configure_channel(ADS1015_ADDR_1, ch);
        sleep_ms(1);  // Brief settling time
        
        if (ads1015_check_alert(ADS1015_ADDR_1)) {
          int16_t adc = ads1015_read_adc(ADS1015_ADDR_1);
          SensorState new_state = adc_to_state(adc);
          
          if (new_state != sensor_states[ch]) {
            printf("%s: %dmV (%d->%d)\n", sensor_names[ch], 
                   adc_to_mv(adc), sensor_states[ch], new_state);
            sensor_states[ch] = new_state;
            state_changed = true;
          }
        }
      }
      
      // Clear latch
      ads1015_read_register(ADS1015_ADDR_1, ADS1015_REG_CONFIG);
      
      // Check ADS1015 #2 (Sensors 3-5)
      for (int ch = 0; ch < 3; ch++) {
        ads1015_configure_channel(ADS1015_ADDR_2, ch);
        sleep_ms(1);  // Brief settling time
        
        if (ads1015_check_alert(ADS1015_ADDR_2)) {
          int16_t adc = ads1015_read_adc(ADS1015_ADDR_2);
          SensorState new_state = adc_to_state(adc);
          
          if (new_state != sensor_states[ch + 2]) {
            printf("%s: %dmV (%d->%d)\n", sensor_names[ch + 2], 
                   adc_to_mv(adc), sensor_states[ch + 2], new_state);
            sensor_states[ch + 2] = new_state;
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

## How It Works

### Window Comparator Mode

1. **Configure thresholds:** Low = 1.0V, High = 2.5V
2. **ALERT triggers when:** Voltage < 1.0V OR Voltage > 2.5V
3. **RP2040 receives interrupt** on ALERT pin (falling edge)
4. **Firmware reads I2C registers** to get ADC value
5. **Determine state:**
   - ADC < 500 (< 1.0V) → South pole
   - ADC > 1250 (> 2.5V) → North pole
   - ADC 500-1250 (1.0V-2.5V) → No magnet

### Event Flow

```
1. Hall sensor detects magnet
2. Voltage changes (e.g., 1.68V → 3.47V)
3. ADS1015 comparator detects voltage > 2.5V
4. ALERT pin goes LOW
5. RP2040 GPIO interrupt fires
6. Firmware reads I2C registers
7. ADC value = 1735 counts (3.47V)
8. State = STATE_HIGH (North pole)
9. Send state update via serial
```

---

## Advantages

✅ **Single wire per sensor** - minimal wiring  
✅ **Three-state detection** - South/None/North  
✅ **Event-driven** - interrupt-based, not polling  
✅ **Shared ALERT** - only 1 GPIO pin for 5 sensors  
✅ **Fast response** - 3300 SPS sampling rate  
✅ **Low cost** - $7.65 total for 5 sensors  
✅ **Scalable** - can add more ADS1015 modules  

---

## Summary

This design provides **bipolar hall sensor detection** for 5 sensors using:
- **2 ADS1015 modules** with window comparators
- **1 shared ALERT pin** for interrupt-driven operation
- **Single wire per sensor** for minimal wiring
- **Three-state detection** per sensor (South/None/North)
- **Only 3 GPIO pins** total (SDA, SCL, ALERT)

**Total cost:** $7.65  
**Total sensors:** 5 bipolar hall sensors  
**GPIO pins used:** 3 (I2C + ALERT)
