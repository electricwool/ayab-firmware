# Dual ADS1115 with Shared ALERT Pin

## Overview
Circuit design for using **two ADS1115 modules** (8 ADC channels total) with a **single shared ALERT/GPIO pin** on the RP2040. This saves GPIO pins while maintaining event-driven operation.

---

## How It Works

### ALERT Pin Characteristics
- **Open-drain output** - can be wire-OR'd together
- **Active LOW** - pulls to GND when triggered
- **Requires pull-up resistor** - external resistor to 3.3V

### Shared ALERT Configuration
When multiple ADS1115 ALERT pins are connected together:
- Any ADS1115 can pull the line LOW
- RP2040 sees LOW when **any** ADS1115 triggers
- Software must poll all ADS1115 modules to find which one triggered

---

## Circuit Schematic

```
Sensor 1 Input (0V / 1.68V / 3.47V)
    |
    +---[R1: 100k]---+---[C1: 10nF]---GND
                     |
                     +---> AIN0 (ADS1115 #1)
                     
Sensor 2 Input (0V / 1.68V / 3.47V)
    |
    +---[R2: 100k]---+---[C2: 10nF]---GND
                     |
                     +---> AIN1 (ADS1115 #1)

Sensor 3 Input (0V / 1.68V / 3.47V)
    |
    +---[R3: 100k]---+---[C3: 10nF]---GND
                     |
                     +---> AIN0 (ADS1115 #2)
                     
Sensor 4 Input (0V / 1.68V / 3.47V)
    |
    +---[R4: 100k]---+---[C4: 10nF]---GND
                     |
                     +---> AIN1 (ADS1115 #2)

            ADS1115 #1 (Address 0x48)
           +----------------+
    VDD ---|VDD         SDA|---+
    GND ---|GND         SCL|---+
    AIN0 --|A0        ALERT|---+
    AIN1 --|A1          ADD|---GND (addr 0x48)
    GND ---|A2             |
    GND ---|A3             |
           +----------------+
                              |
            ADS1115 #2 (Address 0x49)
           +----------------+ |
    VDD ---|VDD         SDA|--+
    GND ---|GND         SCL|--+
    AIN0 --|A0        ALERT|--+--- [R5: 10k] --- 3.3V
    AIN1 --|A1          ADD|--+                    |
    GND ---|A2             | VDD (addr 0x49)       |
    GND ---|A3             |                       |
           +----------------+                      |
                                                   |
    RP2040 I2C Bus                                 |
    SDA (GP4) <------------------------------------+
    SCL (GP5) <------------------------------------+
    ALERT_PIN (GP6) <------------------------------+
    
    I2C Pull-ups: 4.7kΩ on SDA and SCL to 3.3V
    ALERT Pull-up: 10kΩ to 3.3V (shared by both ADS1115)
```

### Key Points
1. **Both ALERT pins connected together** - wire-OR configuration
2. **Single 10kΩ pull-up** - shared by both modules
3. **Different I2C addresses** - 0x48 and 0x49
4. **Same SDA/SCL bus** - standard I2C multi-device

---

## Components Required

| Qty | Part | Description | Unit Price | Total |
|-----|------|-------------|------------|-------|
| 2 | ADS1115 | 16-bit ADC Module | $5.00 | $10.00 |
| 4 | 100kΩ | Input protection resistors | $0.05 | $0.20 |
| 4 | 10nF | Input filtering capacitors | $0.05 | $0.20 |
| 2 | 4.7kΩ | I2C pull-up resistors | $0.05 | $0.10 |
| 1 | 10kΩ | ALERT pull-up resistor | $0.05 | $0.05 |
| **TOTAL** | | | | **$10.55** |

---

## RP2040 Firmware - Shared ALERT Pin

### Complete Implementation

```cpp
// Dual ADS1115 with Shared ALERT Pin
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

// ADS1115 Addresses
#define ADS1115_ADDR_1 0x48  // ADDR → GND
#define ADS1115_ADDR_2 0x49  // ADDR → VDD

// Shared ALERT pin
#define ALERT_PIN 6

// ADS1115 Registers
#define ADS1115_REG_CONVERSION  0x00
#define ADS1115_REG_CONFIG      0x01
#define ADS1115_REG_LO_THRESH   0x02
#define ADS1115_REG_HI_THRESH   0x03

// Config bits
#define ADS1115_CONFIG_OS_SINGLE    0x8000
#define ADS1115_CONFIG_MUX_AIN0_GND 0x4000
#define ADS1115_CONFIG_MUX_AIN1_GND 0x5000
#define ADS1115_CONFIG_PGA_4_096V   0x0200
#define ADS1115_CONFIG_MODE_CONT    0x0000
#define ADS1115_CONFIG_DR_128SPS    0x0080
#define ADS1115_CONFIG_COMP_MODE_WINDOW 0x0010
#define ADS1115_CONFIG_COMP_POL_LOW 0x0000
#define ADS1115_CONFIG_COMP_LAT_ON  0x0004
#define ADS1115_CONFIG_COMP_QUE_1   0x0000

// Voltage thresholds (ADC counts for ±4.096V range)
#define THRESHOLD_LOW_INACTIVE  2520   // 1.0V
#define THRESHOLD_INACTIVE_HIGH 19660  // 2.5V

// State definitions
typedef enum {
  STATE_LOW = 0,
  STATE_INACTIVE = 1,
  STATE_HIGH = 2,
  STATE_ERROR = 3
} SensorState;

// Global state
volatile bool alert_triggered = false;
SensorState sensor_states[4] = {STATE_ERROR, STATE_ERROR, STATE_ERROR, STATE_ERROR};

// I2C write register
void ads1115_write_register(uint8_t addr, uint8_t reg, uint16_t value) {
  uint8_t data[3] = {reg, (value >> 8) & 0xFF, value & 0xFF};
  i2c_write_blocking(I2C_PORT, addr, data, 3, false);
}

// I2C read register
uint16_t ads1115_read_register(uint8_t addr, uint8_t reg) {
  uint8_t data[2];
  i2c_write_blocking(I2C_PORT, addr, &reg, 1, true);
  i2c_read_blocking(I2C_PORT, addr, data, 2, false);
  return (data[0] << 8) | data[1];
}

// Configure comparator thresholds
void ads1115_set_thresholds(uint8_t addr, int16_t lo_thresh, int16_t hi_thresh) {
  ads1115_write_register(addr, ADS1115_REG_LO_THRESH, lo_thresh);
  ads1115_write_register(addr, ADS1115_REG_HI_THRESH, hi_thresh);
}

// Configure ADS1115 channel with comparator
void ads1115_configure_channel(uint8_t addr, uint8_t channel) {
  uint16_t config = ADS1115_CONFIG_MODE_CONT |
                    ADS1115_CONFIG_PGA_4_096V |
                    ADS1115_CONFIG_DR_128SPS |
                    ADS1115_CONFIG_COMP_MODE_WINDOW |
                    ADS1115_CONFIG_COMP_POL_LOW |
                    ADS1115_CONFIG_COMP_LAT_ON |
                    ADS1115_CONFIG_COMP_QUE_1;
  
  // Set MUX for channel
  if (channel == 0) {
    config |= ADS1115_CONFIG_MUX_AIN0_GND;
  } else {
    config |= ADS1115_CONFIG_MUX_AIN1_GND;
  }
  
  ads1115_write_register(addr, ADS1115_REG_CONFIG, config);
  ads1115_set_thresholds(addr, THRESHOLD_LOW_INACTIVE, THRESHOLD_INACTIVE_HIGH);
}

// Read ADC value
int16_t ads1115_read_adc(uint8_t addr) {
  return ads1115_read_register(addr, ADS1115_REG_CONVERSION);
}

// Convert ADC to state
SensorState adc_to_state(int16_t adc_value) {
  if (adc_value < 0) return STATE_ERROR;
  if (adc_value < THRESHOLD_LOW_INACTIVE) return STATE_LOW;
  if (adc_value < THRESHOLD_INACTIVE_HIGH) return STATE_INACTIVE;
  return STATE_HIGH;
}

// Convert ADC to millivolts
int16_t adc_to_mv(int16_t adc_value) {
  return (adc_value * 125) / 1000;
}

// Check which ADS1115 triggered the alert
bool ads1115_check_alert(uint8_t addr) {
  // Read config register - bit 15 indicates conversion ready
  uint16_t config = ads1115_read_register(addr, ADS1115_REG_CONFIG);
  
  // Check if comparator is active (ALERT triggered)
  // We can also just read the conversion and check if it's outside thresholds
  int16_t adc = ads1115_read_adc(addr);
  
  // Check if value is outside window (triggered comparator)
  if (adc < THRESHOLD_LOW_INACTIVE || adc > THRESHOLD_INACTIVE_HIGH) {
    return true;
  }
  
  return false;
}

// GPIO interrupt handler for shared ALERT pin
void alert_irq_handler(uint gpio, uint32_t events) {
  if (gpio == ALERT_PIN && (events & GPIO_IRQ_EDGE_FALL)) {
    alert_triggered = true;
  }
}

// Send state update
void send_state_update() {
  // Pack all 4 sensor states into 1 byte
  uint8_t statusByte = (sensor_states[0] << 6) | 
                       (sensor_states[1] << 4) |
                       (sensor_states[2] << 2) |
                       (sensor_states[3] << 0);
  
  printf("State: 0x%02X (S1=%d, S2=%d, S3=%d, S4=%d)\n", 
         statusByte, sensor_states[0], sensor_states[1], 
         sensor_states[2], sensor_states[3]);
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

// Initialize ADS1115
bool init_ads1115(uint8_t addr) {
  uint16_t config = ads1115_read_register(addr, ADS1115_REG_CONFIG);
  
  if (config == 0x0000 || config == 0xFFFF) {
    printf("ERROR: ADS1115 at 0x%02X not found\n", addr);
    return false;
  }
  
  printf("ADS1115 at 0x%02X initialized\n", addr);
  return true;
}

int main() {
  stdio_init_all();
  sleep_ms(2000);
  
  printf("Dual ADS1115 with Shared ALERT Pin\n");
  
  // Initialize hardware
  init_i2c();
  init_alert_pin();
  
  // Initialize both ADS1115 modules
  if (!init_ads1115(ADS1115_ADDR_1) || !init_ads1115(ADS1115_ADDR_2)) {
    while (1) sleep_ms(1000);
  }
  
  // Configure ADS1115 #1 channels
  ads1115_configure_channel(ADS1115_ADDR_1, 0);  // Sensor 1
  sleep_ms(10);
  int16_t adc = ads1115_read_adc(ADS1115_ADDR_1);
  sensor_states[0] = adc_to_state(adc);
  printf("S1 initial: %dmV (%d)\n", adc_to_mv(adc), sensor_states[0]);
  
  ads1115_configure_channel(ADS1115_ADDR_1, 1);  // Sensor 2
  sleep_ms(10);
  adc = ads1115_read_adc(ADS1115_ADDR_1);
  sensor_states[1] = adc_to_state(adc);
  printf("S2 initial: %dmV (%d)\n", adc_to_mv(adc), sensor_states[1]);
  
  // Configure ADS1115 #2 channels
  ads1115_configure_channel(ADS1115_ADDR_2, 0);  // Sensor 3
  sleep_ms(10);
  adc = ads1115_read_adc(ADS1115_ADDR_2);
  sensor_states[2] = adc_to_state(adc);
  printf("S3 initial: %dmV (%d)\n", adc_to_mv(adc), sensor_states[2]);
  
  ads1115_configure_channel(ADS1115_ADDR_2, 1);  // Sensor 4
  sleep_ms(10);
  adc = ads1115_read_adc(ADS1115_ADDR_2);
  sensor_states[3] = adc_to_state(adc);
  printf("S4 initial: %dmV (%d)\n", adc_to_mv(adc), sensor_states[3]);
  
  send_state_update();
  
  // Main loop - event-driven with shared ALERT
  while (1) {
    if (alert_triggered) {
      alert_triggered = false;
      
      bool state_changed = false;
      
      // Check ADS1115 #1 (Sensors 1 & 2)
      if (ads1115_check_alert(ADS1115_ADDR_1)) {
        // Read both channels
        ads1115_configure_channel(ADS1115_ADDR_1, 0);
        sleep_ms(2);
        int16_t adc1 = ads1115_read_adc(ADS1115_ADDR_1);
        SensorState new_state1 = adc_to_state(adc1);
        
        ads1115_configure_channel(ADS1115_ADDR_1, 1);
        sleep_ms(2);
        int16_t adc2 = ads1115_read_adc(ADS1115_ADDR_1);
        SensorState new_state2 = adc_to_state(adc2);
        
        if (new_state1 != sensor_states[0]) {
          printf("S1: %dmV (%d->%d)\n", adc_to_mv(adc1), 
                 sensor_states[0], new_state1);
          sensor_states[0] = new_state1;
          state_changed = true;
        }
        
        if (new_state2 != sensor_states[1]) {
          printf("S2: %dmV (%d->%d)\n", adc_to_mv(adc2), 
                 sensor_states[1], new_state2);
          sensor_states[1] = new_state2;
          state_changed = true;
        }
        
        // Clear latch
        ads1115_read_register(ADS1115_ADDR_1, ADS1115_REG_CONFIG);
      }
      
      // Check ADS1115 #2 (Sensors 3 & 4)
      if (ads1115_check_alert(ADS1115_ADDR_2)) {
        // Read both channels
        ads1115_configure_channel(ADS1115_ADDR_2, 0);
        sleep_ms(2);
        int16_t adc3 = ads1115_read_adc(ADS1115_ADDR_2);
        SensorState new_state3 = adc_to_state(adc3);
        
        ads1115_configure_channel(ADS1115_ADDR_2, 1);
        sleep_ms(2);
        int16_t adc4 = ads1115_read_adc(ADS1115_ADDR_2);
        SensorState new_state4 = adc_to_state(adc4);
        
        if (new_state3 != sensor_states[2]) {
          printf("S3: %dmV (%d->%d)\n", adc_to_mv(adc3), 
                 sensor_states[2], new_state3);
          sensor_states[2] = new_state3;
          state_changed = true;
        }
        
        if (new_state4 != sensor_states[3]) {
          printf("S4: %dmV (%d->%d)\n", adc_to_mv(adc4), 
                 sensor_states[3], new_state4);
          sensor_states[3] = new_state4;
          state_changed = true;
        }
        
        // Clear latch
        ads1115_read_register(ADS1115_ADDR_2, ADS1115_REG_CONFIG);
      }
      
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

## Advantages of Shared ALERT Pin

### ✅ Benefits
1. **Saves GPIO pins** - only 1 pin for 2 ADS1115 modules (4 sensors)
2. **Simple wiring** - just connect ALERT pins together
3. **Event-driven** - still interrupt-based operation
4. **Scalable** - can add up to 4 ADS1115 (16 sensors) on same ALERT pin

### ⚠️ Trade-offs
1. **Must poll all devices** - when ALERT triggers, check which ADS1115 caused it
2. **Slightly slower** - need to query multiple devices
3. **More I2C traffic** - checking multiple devices on each alert

---

## Alternative: Separate ALERT Pins

If you have spare GPIO pins, use separate ALERT pins for faster response:

```
ADS1115 #1 ALERT → RP2040 GP6
ADS1115 #2 ALERT → RP2040 GP7
```

**Advantages:**
- ✅ Know immediately which ADS1115 triggered
- ✅ Faster response time
- ✅ Less I2C traffic

**Disadvantages:**
- ❌ Uses 2 GPIO pins instead of 1

---

## Summary

The shared ALERT pin configuration:

✅ **Works perfectly** - open-drain outputs can be wire-OR'd  
✅ **Saves GPIO** - only 1 pin for multiple ADS1115 modules  
✅ **Event-driven** - still interrupt-based, not polling  
✅ **Scalable** - can add up to 4 ADS1115 (16 sensors total)  

**Best for:** Applications where GPIO pins are limited and slight latency increase is acceptable.

**Total Channels**: 8 ADC channels (2 ADS1115 × 4 channels each) using only 3 GPIO pins (SDA, SCL, ALERT).
