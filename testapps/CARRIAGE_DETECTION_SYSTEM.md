# Knitting Machine Carriage Detection System

## Overview
Dual ADS1015 system for detecting and identifying knitting machine carriages as they move along a rail. Uses **bipolar hall sensors** to detect magnet polarity for carriage identification and position tracking.

**System Purpose:**
- **ADS1015 #1**: Carriage detection and identification (2 hall sensors)
- **ADS1015 #2**: Encoder signals (Belt Phase, Encoder A, Encoder B)

---

## Carriage Types and Magnet Configurations

### Three Carriage Types

| Carriage Type | Magnet Configuration | Detection Pattern |
|---------------|---------------------|-------------------|
| **Lace Carriage** | Single **South pole** magnet | Left sensor: SOUTH → INACTIVE<br>Right sensor: SOUTH → INACTIVE |
| **K-Carriage** | Single **North pole** magnet | Left sensor: NORTH → INACTIVE<br>Right sensor: NORTH → INACTIVE |
| **G-Carriage** | **North + South** magnets close together | Left sensor: NORTH → SOUTH<br>Right sensor: SOUTH → NORTH |

### Physical Layout

```
Rail with two hall sensors (Left and Right):

    [Left Sensor]  ←─────────────────────→  [Right Sensor]
         │                                         │
    ═════╪═════════════════════════════════════════╪═════
         │         Carriage moves here             │
         │                                         │
         
Lace Carriage (South magnet):
    ═════════════════════════════════════════════════════
              [S]  →→→→→→→→→→→→→→→→→→→
    ═════════════════════════════════════════════════════
    
K-Carriage (North magnet):
    ═════════════════════════════════════════════════════
              [N]  →→→→→→→→→→→→→→→→→→→
    ═════════════════════════════════════════════════════
    
G-Carriage (North + South magnets):
    ═════════════════════════════════════════════════════
            [N][S]  →→→→→→→→→→→→→→→→→→
    ═════════════════════════════════════════════════════
```

---

## Detection Sequence

### Lace Carriage (South Magnet) - Moving Left to Right

```
Time    Left Sensor    Right Sensor    Interpretation
────────────────────────────────────────────────────────
  0     INACTIVE       INACTIVE        No carriage
  1     SOUTH          INACTIVE        Lace entering (left)
  2     INACTIVE       INACTIVE        Between sensors
  3     INACTIVE       SOUTH           Lace exiting (right)
  4     INACTIVE       INACTIVE        Carriage passed
```

### K-Carriage (North Magnet) - Moving Left to Right

```
Time    Left Sensor    Right Sensor    Interpretation
────────────────────────────────────────────────────────
  0     INACTIVE       INACTIVE        No carriage
  1     NORTH          INACTIVE        K-carriage entering (left)
  2     INACTIVE       INACTIVE        Between sensors
  3     INACTIVE       NORTH           K-carriage exiting (right)
  4     INACTIVE       INACTIVE        Carriage passed
```

### G-Carriage (North + South Magnets) - Moving Left to Right

```
Time    Left Sensor    Right Sensor    Interpretation
────────────────────────────────────────────────────────
  0     INACTIVE       INACTIVE        No carriage
  1     NORTH          INACTIVE        G-carriage entering (North first)
  2     SOUTH          INACTIVE        G-carriage (South magnet over left)
  3     SOUTH          NORTH           G-carriage (both magnets detected)
  4     INACTIVE       SOUTH           G-carriage (South over right)
  5     INACTIVE       INACTIVE        Carriage passed
```

**Key Signature:** G-carriage shows **both polarities** in sequence!

---

## Circuit Schematic

```
Carriage Detection System (ADS1015 #1)

    Left Hall Sensor (Bipolar, 0V / 1.68V / 3.47V)
        |
        +---[R1: 100k]---+---[C1: 10nF]---GND
                         |
                         +---> AIN0 (ADS1015 #1)
                         
    Right Hall Sensor (Bipolar, 0V / 1.68V / 3.47V)
        |
        +---[R2: 100k]---+---[C2: 10nF]---GND
                         |
                         +---> AIN1 (ADS1015 #1)

    ADS1015 #1 (Address 0x48) - CARRIAGE DETECTION
   +----------------+
   |VDD         SDA|---+
   |GND         SCL|---+
   |A0        ALERT|---+
   |A1          ADD|---GND (addr 0x48)
   |A2             |
   |A3             |
   +----------------+
   
   Window Comparator: 1.0V - 2.5V (inactive range)
   ALERT triggers when: voltage < 1.0V (SOUTH) or > 2.5V (NORTH)


Encoder System (ADS1015 #2)

    Belt Phase Signal (analog)
        |
        +---[R3: 100k]---+---[C3: 10nF]---GND
                         |
                         +---> AIN0 (ADS1015 #2)
                         
    Encoder A Signal (quadrature)
        |
        +---[R4: 100k]---+---[C4: 10nF]---GND
                         |
                         +---> AIN1 (ADS1015 #2)
                         
    Encoder B Signal (quadrature)
        |
        +---[R5: 100k]---+---[C5: 10nF]---GND
                         |
                         +---> AIN2 (ADS1015 #2)

    ADS1015 #2 (Address 0x49) - ENCODER SIGNALS
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

## Carriage Identification Algorithm

### State Machine for Carriage Detection

```cpp
typedef enum {
  CARRIAGE_NONE = 0,
  CARRIAGE_LACE = 1,
  CARRIAGE_K = 2,
  CARRIAGE_G = 3,
  CARRIAGE_UNKNOWN = 4
} CarriageType;

typedef enum {
  SENSOR_LEFT = 0,
  SENSOR_RIGHT = 1
} SensorPosition;

typedef struct {
  CarriageType type;
  SensorPosition last_sensor;
  uint32_t timestamp_ms;
  bool north_detected;
  bool south_detected;
} CarriageState;
```

### Detection Logic

```cpp
void update_carriage_detection(SensorPosition sensor, SensorState state) {
  static CarriageState carriage = {CARRIAGE_NONE, SENSOR_LEFT, 0, false, false};
  uint32_t now = time_ms();
  
  // Reset if timeout (carriage passed)
  if (now - carriage.timestamp_ms > 500) {
    carriage.north_detected = false;
    carriage.south_detected = false;
    carriage.type = CARRIAGE_NONE;
  }
  
  // Update timestamp
  carriage.timestamp_ms = now;
  
  // Track which polarities we've seen
  if (state == STATE_NORTH) {
    carriage.north_detected = true;
  } else if (state == STATE_SOUTH) {
    carriage.south_detected = true;
  }
  
  // Identify carriage type
  if (carriage.north_detected && carriage.south_detected) {
    carriage.type = CARRIAGE_G;  // Both polarities = G-carriage
  } else if (carriage.north_detected) {
    carriage.type = CARRIAGE_K;  // Only North = K-carriage
  } else if (carriage.south_detected) {
    carriage.type = CARRIAGE_LACE;  // Only South = Lace carriage
  }
  
  // Track position
  carriage.last_sensor = sensor;
  
  // Report detection
  printf("Carriage: %s at %s sensor\n", 
         carriage_type_name(carriage.type),
         sensor == SENSOR_LEFT ? "LEFT" : "RIGHT");
}
```

---

## Timing Requirements for Fast-Moving Carriages

### Critical Timing Parameters

| Parameter | Value | Notes |
|-----------|-------|-------|
| **Carriage Speed** | ~1 m/s | Typical knitting speed |
| **Sensor Spacing** | ~10 cm | Distance between left/right sensors |
| **Magnet Width** | ~2 cm | Approximate magnet size |
| **Detection Window** | ~20 ms | Time magnet is over sensor |
| **ADS1015 Sample Rate** | 3300 SPS | 0.3 ms per sample |
| **Samples per Detection** | ~66 samples | During 20ms window |

**Conclusion:** ADS1015 at 3300 SPS provides **66 samples** during a 20ms detection window - plenty of resolution for accurate detection!

### Why ADS1015 is Perfect for This Application

✅ **Fast sampling:** 3300 SPS = 0.3ms response time  
✅ **Event-driven:** ALERT pin triggers immediately when magnet detected  
✅ **No polling:** Interrupt-based, CPU sleeps until carriage passes  
✅ **Accurate timing:** Microsecond-level timestamps for position calculation  
✅ **Low latency:** < 1ms from magnet detection to firmware notification  

---

## RP2040 Firmware - Carriage Detection System

### Complete Implementation

```cpp
// Knitting Machine Carriage Detection System
// Dual ADS1015: Carriage detection + Encoder wheel
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
#define ADS1015_CARRIAGE 0x48  // Carriage detection
#define ADS1015_ENCODER  0x49  // Encoder wheel

// Shared ALERT pin
#define ALERT_PIN 6

// ADS1015 Registers
#define ADS1015_REG_CONVERSION  0x00
#define ADS1015_REG_CONFIG      0x01
#define ADS1015_REG_LO_THRESH   0x02
#define ADS1015_REG_HI_THRESH   0x03

// Config bits
#define ADS1015_CONFIG_MUX_AIN0_GND 0x4000
#define ADS1015_CONFIG_MUX_AIN1_GND 0x5000
#define ADS1015_CONFIG_MUX_AIN2_GND 0x6000
#define ADS1015_CONFIG_PGA_4_096V   0x0200
#define ADS1015_CONFIG_MODE_CONT    0x0000
#define ADS1015_CONFIG_DR_3300SPS   0x00E0  // Maximum speed!
#define ADS1015_CONFIG_COMP_MODE_WINDOW 0x0010
#define ADS1015_CONFIG_COMP_POL_LOW 0x0000
#define ADS1015_CONFIG_COMP_LAT_ON  0x0004
#define ADS1015_CONFIG_COMP_QUE_1   0x0000

// Thresholds (12-bit ADC counts)
#define THRESHOLD_LOW     500    // 1.0V (South pole)
#define THRESHOLD_HIGH    1250   // 2.5V (North pole)

// Sensor states
typedef enum {
  STATE_SOUTH = 0,      // South pole detected
  STATE_INACTIVE = 1,   // No magnet
  STATE_NORTH = 2,      // North pole detected
  STATE_ERROR = 3
} SensorState;

// Carriage types
typedef enum {
  CARRIAGE_NONE = 0,
  CARRIAGE_LACE = 1,    // South magnet
  CARRIAGE_K = 2,       // North magnet
  CARRIAGE_G = 3,       // North + South magnets
  CARRIAGE_UNKNOWN = 4
} CarriageType;

// Sensor positions
typedef enum {
  SENSOR_LEFT = 0,
  SENSOR_RIGHT = 1
} SensorPosition;

// Carriage state tracking
typedef struct {
  CarriageType type;
  SensorPosition last_sensor;
  uint32_t timestamp_us;
  bool north_detected;
  bool south_detected;
  int8_t direction;  // -1 = left, 0 = unknown, +1 = right
} CarriageState;

// Global state
volatile bool alert_triggered = false;
SensorState sensor_states[2] = {STATE_INACTIVE, STATE_INACTIVE};
CarriageState carriage = {CARRIAGE_NONE, SENSOR_LEFT, 0, false, false, 0};

// I2C functions
void ads1015_write_register(uint8_t addr, uint8_t reg, uint16_t value) {
  uint8_t data[3] = {reg, (value >> 8) & 0xFF, value & 0xFF};
  i2c_write_blocking(I2C_PORT, addr, data, 3, false);
}

uint16_t ads1015_read_register(uint8_t addr, uint8_t reg) {
  uint8_t data[2];
  i2c_write_blocking(I2C_PORT, addr, &reg, 1, true);
  i2c_read_blocking(I2C_PORT, addr, data, 2, false);
  return (data[0] << 8) | data[1];
}

void ads1015_set_thresholds(uint8_t addr, int16_t lo, int16_t hi) {
  ads1015_write_register(addr, ADS1015_REG_LO_THRESH, lo << 4);
  ads1015_write_register(addr, ADS1015_REG_HI_THRESH, hi << 4);
}

void ads1015_configure_channel(uint8_t addr, uint8_t channel) {
  uint16_t config = ADS1015_CONFIG_MODE_CONT |
                    ADS1015_CONFIG_PGA_4_096V |
                    ADS1015_CONFIG_DR_3300SPS |  // Max speed!
                    ADS1015_CONFIG_COMP_MODE_WINDOW |
                    ADS1015_CONFIG_COMP_POL_LOW |
                    ADS1015_CONFIG_COMP_LAT_ON |
                    ADS1015_CONFIG_COMP_QUE_1;
  
  switch (channel) {
    case 0: config |= ADS1015_CONFIG_MUX_AIN0_GND; break;
    case 1: config |= ADS1015_CONFIG_MUX_AIN1_GND; break;
    case 2: config |= ADS1015_CONFIG_MUX_AIN2_GND; break;
  }
  
  ads1015_write_register(addr, ADS1015_REG_CONFIG, config);
  ads1015_set_thresholds(addr, THRESHOLD_LOW, THRESHOLD_HIGH);
}

int16_t ads1015_read_adc(uint8_t addr) {
  uint16_t raw = ads1015_read_register(addr, ADS1015_REG_CONVERSION);
  return (int16_t)raw >> 4;
}

SensorState adc_to_state(int16_t adc_value) {
  if (adc_value < THRESHOLD_LOW) return STATE_SOUTH;
  if (adc_value > THRESHOLD_HIGH) return STATE_NORTH;
  return STATE_INACTIVE;
}

const char* carriage_type_name(CarriageType type) {
  switch (type) {
    case CARRIAGE_LACE: return "LACE";
    case CARRIAGE_K: return "K-CARRIAGE";
    case CARRIAGE_G: return "G-CARRIAGE";
    case CARRIAGE_NONE: return "NONE";
    default: return "UNKNOWN";
  }
}

const char* state_name(SensorState state) {
  switch (state) {
    case STATE_SOUTH: return "SOUTH";
    case STATE_NORTH: return "NORTH";
    case STATE_INACTIVE: return "INACTIVE";
    default: return "ERROR";
  }
}

// Carriage detection logic
void update_carriage_detection(SensorPosition sensor, SensorState new_state) {
  uint32_t now = time_us_32();
  
  // Reset if timeout (500ms = carriage passed)
  if ((now - carriage.timestamp_us) > 500000) {
    carriage.north_detected = false;
    carriage.south_detected = false;
    carriage.type = CARRIAGE_NONE;
    carriage.direction = 0;
  }
  
  // Update timestamp
  carriage.timestamp_us = now;
  
  // Track polarity detections
  if (new_state == STATE_NORTH) {
    carriage.north_detected = true;
  } else if (new_state == STATE_SOUTH) {
    carriage.south_detected = true;
  }
  
  // Identify carriage type
  CarriageType old_type = carriage.type;
  if (carriage.north_detected && carriage.south_detected) {
    carriage.type = CARRIAGE_G;
  } else if (carriage.north_detected) {
    carriage.type = CARRIAGE_K;
  } else if (carriage.south_detected) {
    carriage.type = CARRIAGE_LACE;
  }
  
  // Determine direction
  if (carriage.last_sensor == SENSOR_LEFT && sensor == SENSOR_RIGHT) {
    carriage.direction = 1;  // Moving right
  } else if (carriage.last_sensor == SENSOR_RIGHT && sensor == SENSOR_LEFT) {
    carriage.direction = -1;  // Moving left
  }
  
  carriage.last_sensor = sensor;
  
  // Report if carriage type identified or changed
  if (carriage.type != CARRIAGE_NONE && carriage.type != old_type) {
    printf("*** CARRIAGE IDENTIFIED: %s ***\n", carriage_type_name(carriage.type));
  }
  
  // Report position
  const char* dir_str = carriage.direction == 1 ? "→" : 
                       carriage.direction == -1 ? "←" : "?";
  printf("[%lu us] %s sensor: %s | Carriage: %s %s\n",
         now, sensor == SENSOR_LEFT ? "LEFT " : "RIGHT",
         state_name(new_state),
         carriage_type_name(carriage.type), dir_str);
}

// GPIO interrupt handler
void alert_irq_handler(uint gpio, uint32_t events) {
  if (gpio == ALERT_PIN && (events & GPIO_IRQ_EDGE_FALL)) {
    alert_triggered = true;
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
  
  printf("\n===========================================\n");
  printf("Knitting Machine Carriage Detection System\n");
  printf("===========================================\n");
  printf("Carriage types:\n");
  printf("  - Lace: South magnet\n");
  printf("  - K-Carriage: North magnet\n");
  printf("  - G-Carriage: North + South magnets\n\n");
  
  // Initialize hardware
  init_i2c();
  init_alert_pin();
  
  if (!init_ads1015(ADS1015_CARRIAGE) || !init_ads1015(ADS1015_ENCODER)) {
    while (1) sleep_ms(1000);
  }
  
  // Configure carriage detection sensors (left and right)
  const char* sensor_names[] = {"LEFT", "RIGHT"};
  for (int ch = 0; ch < 2; ch++) {
    ads1015_configure_channel(ADS1015_CARRIAGE, ch);
    sleep_ms(10);
    int16_t adc = ads1015_read_adc(ADS1015_CARRIAGE);
    sensor_states[ch] = adc_to_state(adc);
    printf("%s sensor initial: %dmV (%s)\n", 
           sensor_names[ch], adc, state_name(sensor_states[ch]));
  }
  
  printf("\nWaiting for carriage...\n\n");
  
  // Main loop - event-driven
  while (1) {
    if (alert_triggered) {
      alert_triggered = false;
      
      // Check both carriage sensors
      for (int ch = 0; ch < 2; ch++) {
        ads1015_configure_channel(ADS1015_CARRIAGE, ch);
        sleep_us(500);  // Brief settling time
        
        int16_t adc = ads1015_read_adc(ADS1015_CARRIAGE);
        SensorState new_state = adc_to_state(adc);
        
        if (new_state != sensor_states[ch]) {
          sensor_states[ch] = new_state;
          update_carriage_detection((SensorPosition)ch, new_state);
        }
      }
      
      // Clear latch
      ads1015_read_register(ADS1015_CARRIAGE, ADS1015_REG_CONFIG);
    }
    
    // Sleep until next interrupt
    __wfi();
  }
  
  return 0;
}
```

---

## Key Features

### ✅ Carriage Identification
- **Lace Carriage:** Detects South pole only
- **K-Carriage:** Detects North pole only
- **G-Carriage:** Detects both North and South poles in sequence

### ✅ Position Tracking
- **Left sensor:** Detects carriage entering
- **Right sensor:** Detects carriage exiting
- **Direction:** Calculated from sensor sequence
- **Timestamp:** Microsecond-precision timing for position calculation

### ✅ Fast Response
- **3300 SPS sampling:** 0.3ms per sample
- **66 samples per detection:** During 20ms magnet pass
- **Event-driven:** Interrupt triggers immediately
- **Low latency:** < 1ms from detection to firmware notification

### ✅ Robust Detection
- **Window comparator:** Automatic ALERT on magnet detection
- **Timeout handling:** Resets state after carriage passes
- **State machine:** Tracks carriage type across multiple sensor readings
- **Direction detection:** Knows if carriage moving left or right

---

## Summary

This carriage detection system provides:

✅ **Carriage identification** - Distinguishes Lace/K/G carriages by magnet polarity  
✅ **Position tracking** - Left/right sensors provide reference position  
✅ **Direction detection** - Knows which way carriage is moving  
✅ **Fast response** - 3300 SPS for accurate fast-moving carriage detection  
✅ **Event-driven** - Interrupt-based, CPU sleeps until carriage passes  
✅ **Microsecond timing** - Precise timestamps for position calculation  
✅ **Scalable** - Second ADS1015 available for encoder wheel  

**Total cost:** $7.65 for dual ADS1015 system  
**GPIO pins used:** 3 (SDA, SCL, ALERT)  
**Carriage types detected:** 3 (Lace, K-Carriage, G-Carriage)
