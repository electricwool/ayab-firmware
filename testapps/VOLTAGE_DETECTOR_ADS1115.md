# Voltage Detector Circuit - ADS1115 ADC Solution

## Overview
High-precision voltage detection circuit using the **ADS1115 16-bit ADC** for measuring voltage states on **two sensor inputs** simultaneously. Supports both unipolar (0-5V) and bipolar (-5V to +5V) voltage ranges.

**Key Features:**
- ✅ **16-bit resolution** (65,536 levels)
- ✅ **Programmable gain** (±6.144V to ±0.256V ranges)
- ✅ **I2C interface** (2 pins: SDA, SCL)
- ✅ **True bipolar support** (can measure negative voltages)
- ✅ **No microcontroller needed** (RP2040 reads directly via I2C)

---

## ADS1115 Specifications

| Parameter | Value |
|-----------|-------|
| **Resolution** | 16-bit (65,536 levels) |
| **Channels** | 4 single-ended or 2 differential |
| **Sample Rate** | 8 to 860 SPS |
| **Interface** | I2C (400kHz) |
| **Supply Voltage** | 2.0V to 5.5V (3.3V typical) |
| **Input Range** | ±6.144V (with PGA) |
| **Cost** | ~$5.00 |

---

## Circuit Design

### Components Required
- **U1**: ADS1115 16-bit ADC Module - $5.00
- **R1, R2**: 100kΩ Resistors (input protection) - $0.10
- **R3, R4**: 4.7kΩ Resistors (I2C pull-ups) - $0.10
- **C1**: 100nF Ceramic Capacitor (power decoupling) - $0.05
- **C2, C3**: 10nF Ceramic Capacitors (input filtering) - $0.10

**Total Cost**: ~$5.35

### Circuit Schematic

```
Sensor 1 Input (0V / 1.68V / 3.47V)
    |
    +---[R1: 100k]---+---[C2: 10nF]---GND
                     |
                     +---> AIN0 (ADS1115)
                     
Sensor 2 Input (0V / 1.68V / 3.47V)
    |
    +---[R2: 100k]---+---[C3: 10nF]---GND
                     |
                     +---> AIN1 (ADS1115)

                ADS1115 Module
               +----------------+
    VDD -------|VDD         SDA|-------[R3: 4.7k]---+---> RP2040 SDA
    GND -------|GND         SCL|-------[R4: 4.7k]---+---> RP2040 SCL
    AIN0 ------|A0        ALERT|------- (unused)
    AIN1 ------|A1          ADD|------- GND (I2C addr 0x48)
    GND -------|A2             |
    GND -------|A3             |
               +----------------+

    VDD = 3.3V
    VDD---[C1: 100nF]---GND  (Power decoupling)
```

### Pin Connections

#### ADS1115 Module
- **VDD**: 3.3V power supply
- **GND**: Ground
- **SDA**: I2C data line (to RP2040 SDA with 4.7kΩ pull-up)
- **SCL**: I2C clock line (to RP2040 SCL with 4.7kΩ pull-up)
- **ADDR**: I2C address select (GND = 0x48, VDD = 0x49, SDA = 0x4A, SCL = 0x4B)
- **ALERT**: Alert/ready pin (optional, can be used for interrupt-driven reading)
- **A0-A3**: Analog inputs

#### RP2040 Connections
- **I2C0 SDA** (e.g., GP4): Connect to ADS1115 SDA
- **I2C0 SCL** (e.g., GP5): Connect to ADS1115 SCL
- **3.3V**: Connect to ADS1115 VDD
- **GND**: Connect to ADS1115 GND

---

## Data Protocol

### State Encoding (Same as ATtiny412)
```
Bit 7-6: Sensor 1 State
Bit 5-4: Sensor 2 State
Bit 3-0: Reserved (0x0)

State Encoding:
00 = LOW (0V)
01 = INACTIVE (1.68V)
10 = HIGH (3.47V)
11 = ERROR/INVALID
```

### Voltage Thresholds
```
LOW:      < 1.0V
INACTIVE: 1.0V - 2.5V
HIGH:     > 2.5V
```

---

## RP2040 Firmware

### Complete Implementation

```cpp
// RP2040 Voltage Detector using ADS1115
// Reads two sensors and outputs state changes via serial or GPIO

#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <stdio.h>

// I2C Configuration
#define I2C_PORT i2c0
#define I2C_SDA 4
#define I2C_SCL 5
#define I2C_FREQ 400000  // 400kHz

// ADS1115 I2C Address
#define ADS1115_ADDR 0x48

// ADS1115 Registers
#define ADS1115_REG_CONVERSION  0x00
#define ADS1115_REG_CONFIG      0x01

// ADS1115 Configuration
#define ADS1115_CONFIG_OS_SINGLE    0x8000  // Start single conversion
#define ADS1115_CONFIG_MUX_AIN0_GND 0x4000  // AIN0 vs GND
#define ADS1115_CONFIG_MUX_AIN1_GND 0x5000  // AIN1 vs GND
#define ADS1115_CONFIG_PGA_6_144V   0x0000  // ±6.144V range
#define ADS1115_CONFIG_PGA_4_096V   0x0200  // ±4.096V range
#define ADS1115_CONFIG_MODE_SINGLE  0x0100  // Single-shot mode
#define ADS1115_CONFIG_DR_860SPS    0x00E0  // 860 samples/sec
#define ADS1115_CONFIG_COMP_QUE_DIS 0x0003  // Disable comparator

// Voltage thresholds (in millivolts)
#define THRESHOLD_LOW_INACTIVE  1000   // 1.0V
#define THRESHOLD_INACTIVE_HIGH 2500   // 2.5V

// State definitions
typedef enum {
  STATE_LOW = 0,
  STATE_INACTIVE = 1,
  STATE_HIGH = 2,
  STATE_ERROR = 3
} SensorState;

// Previous states for change detection
SensorState prevSensor1State = STATE_ERROR;
SensorState prevSensor2State = STATE_ERROR;

// I2C write register
void ads1115_write_register(uint8_t reg, uint16_t value) {
  uint8_t data[3] = {reg, (value >> 8) & 0xFF, value & 0xFF};
  i2c_write_blocking(I2C_PORT, ADS1115_ADDR, data, 3, false);
}

// I2C read register
uint16_t ads1115_read_register(uint8_t reg) {
  uint8_t data[2];
  i2c_write_blocking(I2C_PORT, ADS1115_ADDR, &reg, 1, true);
  i2c_read_blocking(I2C_PORT, ADS1115_ADDR, data, 2, false);
  return (data[0] << 8) | data[1];
}

// Read voltage from ADS1115 channel (0-3)
int16_t ads1115_read_voltage_mv(uint8_t channel) {
  // Configure for single-ended read on specified channel
  uint16_t config = ADS1115_CONFIG_OS_SINGLE |
                    ADS1115_CONFIG_MODE_SINGLE |
                    ADS1115_CONFIG_PGA_4_096V |  // ±4.096V range (0.125mV/bit)
                    ADS1115_CONFIG_DR_860SPS |
                    ADS1115_CONFIG_COMP_QUE_DIS;
  
  // Set MUX based on channel
  switch (channel) {
    case 0: config |= ADS1115_CONFIG_MUX_AIN0_GND; break;
    case 1: config |= ADS1115_CONFIG_MUX_AIN1_GND; break;
    case 2: config |= 0x6000; break;  // AIN2 vs GND
    case 3: config |= 0x7000; break;  // AIN3 vs GND
    default: return -1;
  }
  
  // Write config to start conversion
  ads1115_write_register(ADS1115_REG_CONFIG, config);
  
  // Wait for conversion (max 2ms at 860 SPS)
  sleep_ms(2);
  
  // Read conversion result
  int16_t raw = ads1115_read_register(ADS1115_REG_CONVERSION);
  
  // Convert to millivolts (±4.096V range = 0.125mV per bit)
  int16_t voltage_mv = (raw * 125) / 1000;
  
  return voltage_mv;
}

// Determine sensor state from voltage
SensorState voltage_to_state(int16_t voltage_mv) {
  if (voltage_mv < 0) {
    return STATE_ERROR;  // Negative voltage (shouldn't happen)
  } else if (voltage_mv < THRESHOLD_LOW_INACTIVE) {
    return STATE_LOW;
  } else if (voltage_mv < THRESHOLD_INACTIVE_HIGH) {
    return STATE_INACTIVE;
  } else {
    return STATE_HIGH;
  }
}

// Send state update (pack into single byte)
void send_state_update(SensorState sensor1, SensorState sensor2) {
  uint8_t statusByte = (sensor1 << 6) | (sensor2 << 4);
  
  // Output via serial
  printf("State: 0x%02X (S1=%d, S2=%d)\n", statusByte, sensor1, sensor2);
  
  // Or output via GPIO (bit-bang serial, SPI, etc.)
  // gpio_put(TX_PIN, statusByte);
}

// Initialize I2C
void init_i2c() {
  i2c_init(I2C_PORT, I2C_FREQ);
  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA);
  gpio_pull_up(I2C_SCL);
}

// Initialize ADS1115
bool init_ads1115() {
  // Read config register to verify communication
  uint16_t config = ads1115_read_register(ADS1115_REG_CONFIG);
  
  // Check if we got a valid response (not 0x0000 or 0xFFFF)
  if (config == 0x0000 || config == 0xFFFF) {
    printf("ERROR: ADS1115 not found at address 0x%02X\n", ADS1115_ADDR);
    return false;
  }
  
  printf("ADS1115 initialized successfully\n");
  return true;
}

int main() {
  // Initialize stdio for USB serial
  stdio_init_all();
  sleep_ms(2000);  // Wait for USB connection
  
  printf("ADS1115 Voltage Detector Starting...\n");
  
  // Initialize I2C
  init_i2c();
  
  // Initialize ADS1115
  if (!init_ads1115()) {
    printf("Failed to initialize ADS1115\n");
    while (1) {
      sleep_ms(1000);
    }
  }
  
  // Read and send initial state
  int16_t v1 = ads1115_read_voltage_mv(0);
  int16_t v2 = ads1115_read_voltage_mv(1);
  
  prevSensor1State = voltage_to_state(v1);
  prevSensor2State = voltage_to_state(v2);
  
  printf("Initial: S1=%dmV (%d), S2=%dmV (%d)\n", 
         v1, prevSensor1State, v2, prevSensor2State);
  send_state_update(prevSensor1State, prevSensor2State);
  
  // Main loop - event-driven (only transmit on state change)
  while (1) {
    // Read both sensors
    int16_t voltage1 = ads1115_read_voltage_mv(0);
    int16_t voltage2 = ads1115_read_voltage_mv(1);
    
    // Determine states
    SensorState sensor1State = voltage_to_state(voltage1);
    SensorState sensor2State = voltage_to_state(voltage2);
    
    // Check for state changes
    if (sensor1State != prevSensor1State || sensor2State != prevSensor2State) {
      // Debounce: wait and re-read
      sleep_ms(10);
      voltage1 = ads1115_read_voltage_mv(0);
      voltage2 = ads1115_read_voltage_mv(1);
      sensor1State = voltage_to_state(voltage1);
      sensor2State = voltage_to_state(voltage2);
      
      // Confirm state change
      if (sensor1State != prevSensor1State || sensor2State != prevSensor2State) {
        printf("Change: S1=%dmV (%d->%d), S2=%dmV (%d->%d)\n",
               voltage1, prevSensor1State, sensor1State,
               voltage2, prevSensor2State, sensor2State);
        
        prevSensor1State = sensor1State;
        prevSensor2State = sensor2State;
        
        // Send update (asynchronous - only on change)
        send_state_update(sensor1State, sensor2State);
      }
    }
    
    // Check every 10ms
    sleep_ms(10);
  }
  
  return 0;
}
```

---

## Arduino/Platform IO Version

For easier development, use the Adafruit ADS1X15 library:

```cpp
// Arduino/PlatformIO version using Adafruit library
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;

#define THRESHOLD_LOW_INACTIVE  1000   // 1.0V in mV
#define THRESHOLD_INACTIVE_HIGH 2500   // 2.5V in mV

typedef enum {
  STATE_LOW = 0,
  STATE_INACTIVE = 1,
  STATE_HIGH = 2,
  STATE_ERROR = 3
} SensorState;

SensorState prevSensor1State = STATE_ERROR;
SensorState prevSensor2State = STATE_ERROR;

void setup() {
  Serial.begin(115200);
  
  // Initialize ADS1115
  if (!ads.begin()) {
    Serial.println("Failed to initialize ADS1115!");
    while (1);
  }
  
  // Set gain to ±4.096V range (0.125mV per bit)
  ads.setGain(GAIN_ONE);
  
  // Set data rate to 860 SPS
  ads.setDataRate(RATE_ADS1115_860SPS);
  
  Serial.println("ADS1115 initialized");
  
  // Read initial state
  int16_t v1 = ads.readADC_SingleEnded(0);
  int16_t v2 = ads.readADC_SingleEnded(1);
  
  prevSensor1State = voltageToState(ads.computeVolts(v1) * 1000);
  prevSensor2State = voltageToState(ads.computeVolts(v2) * 1000);
  
  sendStateUpdate(prevSensor1State, prevSensor2State);
}

SensorState voltageToState(int16_t voltage_mv) {
  if (voltage_mv < 0) return STATE_ERROR;
  if (voltage_mv < THRESHOLD_LOW_INACTIVE) return STATE_LOW;
  if (voltage_mv < THRESHOLD_INACTIVE_HIGH) return STATE_INACTIVE;
  return STATE_HIGH;
}

void sendStateUpdate(SensorState s1, SensorState s2) {
  uint8_t statusByte = (s1 << 6) | (s2 << 4);
  Serial.printf("State: 0x%02X (S1=%d, S2=%d)\n", statusByte, s1, s2);
}

void loop() {
  // Read both sensors
  int16_t adc1 = ads.readADC_SingleEnded(0);
  int16_t adc2 = ads.readADC_SingleEnded(1);
  
  int16_t voltage1 = ads.computeVolts(adc1) * 1000;  // Convert to mV
  int16_t voltage2 = ads.computeVolts(adc2) * 1000;
  
  SensorState sensor1State = voltageToState(voltage1);
  SensorState sensor2State = voltageToState(voltage2);
  
  // Check for state changes
  if (sensor1State != prevSensor1State || sensor2State != prevSensor2State) {
    // Debounce
    delay(10);
    adc1 = ads.readADC_SingleEnded(0);
    adc2 = ads.readADC_SingleEnded(1);
    voltage1 = ads.computeVolts(adc1) * 1000;
    voltage2 = ads.computeVolts(adc2) * 1000;
    sensor1State = voltageToState(voltage1);
    sensor2State = voltageToState(voltage2);
    
    // Confirm change
    if (sensor1State != prevSensor1State || sensor2State != prevSensor2State) {
      Serial.printf("Change: S1=%dmV (%d->%d), S2=%dmV (%d->%d)\n",
                    voltage1, prevSensor1State, sensor1State,
                    voltage2, prevSensor2State, sensor2State);
      
      prevSensor1State = sensor1State;
      prevSensor2State = sensor2State;
      
      sendStateUpdate(sensor1State, sensor2State);
    }
  }
  
  delay(10);
}
```

---

## Advantages Over ATtiny412 Solution

| Feature | ATtiny412 | ADS1115 |
|---------|-----------|---------|
| **Resolution** | 10-bit (1024 levels) | 16-bit (65,536 levels) |
| **Accuracy** | ±2 LSB | ±0.5 LSB |
| **Voltage Range** | 0-3.3V only | ±6.144V (bipolar) |
| **Channels** | 2 ADC pins | 4 channels |
| **Interface** | Serial (1 pin) | I2C (2 pins) |
| **Programming** | Requires UPDI | No programming needed |
| **Cost** | $1.15 | $5.35 |
| **Complexity** | Medium | Low |
| **Field Updates** | Firmware update | No firmware |

---

## When to Use ADS1115

### ✅ Use ADS1115 When:
- Need **high precision** (16-bit vs 10-bit)
- Need **bipolar voltage** measurement (-5V to +5V)
- Want **no programming** (RP2040 reads directly)
- Need **multiple channels** (up to 4 sensors)
- Budget allows (~$5 vs ~$1)

### ❌ Use ATtiny412 When:
- Budget is tight ($1 vs $5)
- Only need **unipolar** measurement (0-5V)
- Want **single GPIO output** (vs 2-pin I2C)
- 10-bit resolution is sufficient
- Want **programmable logic** on sensor board

---

## PCB Layout Recommendations

1. **Keep I2C traces short** and equal length
2. **Place pull-up resistors close** to RP2040
3. **Add decoupling cap** close to ADS1115 VDD pin
4. **Use ground plane** for stable ADC readings
5. **Separate analog and digital grounds** if possible
6. **Add test points** for SDA, SCL, and analog inputs

---

## Troubleshooting

### I2C Communication Issues
- Verify I2C address (0x48 with ADDR to GND)
- Check pull-up resistors (4.7kΩ)
- Verify SDA/SCL connections
- Check power supply (3.3V)
- Use I2C scanner to detect device

### Incorrect Readings
- Verify input voltage range
- Check PGA gain setting
- Verify reference voltage
- Check for noise on inputs
- Add input filtering capacitors

### No State Changes Detected
- Verify voltage thresholds in code
- Check sensor connections
- Test with known voltages
- Add debug output for raw ADC values

---

## Bill of Materials (BOM)

| Qty | Part Number | Description | Unit Price | Total |
|-----|-------------|-------------|------------|-------|
| 1   | ADS1115     | 16-bit ADC Module | $5.00 | $5.00 |
| 2   | 100kΩ 1/4W  | Resistor (input protection) | $0.05 | $0.10 |
| 2   | 4.7kΩ 1/4W  | Resistor (I2C pull-up) | $0.05 | $0.10 |
| 1   | 100nF       | Ceramic Cap (power) | $0.05 | $0.05 |
| 2   | 10nF        | Ceramic Cap (filtering) | $0.05 | $0.10 |
| **TOTAL** | | | | **$5.35** |

---

## Summary

The ADS1115 provides a high-precision, no-programming-required solution for voltage detection:

✅ **16-bit resolution** for accurate threshold detection  
✅ **I2C interface** - RP2040 reads directly (no intermediate MCU)  
✅ **Bipolar support** - can measure -5V to +5V if needed  
✅ **Event-driven** - only transmits on state changes  
✅ **4 channels** - can expand to 4 sensors  
✅ **No firmware** - all logic in RP2040  

This solution is ideal when precision and bipolar voltage support are required, despite the higher cost compared to the ATtiny412 solution.
