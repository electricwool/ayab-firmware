# Advanced Voltage Detector Circuit - Serial Output

## Overview
Programmable digital circuit for detecting three voltage states (0V, 1.68V, 3.47V) on **two sensor inputs** simultaneously, outputting state information via a **single GPIO pin** using serial communication.

## Solution Options

---

## Option 1: ATtiny85 Microcontroller (RECOMMENDED)

### Why ATtiny85?
- **Programmable thresholds**: No manual adjustment needed
- **Dual ADC inputs**: Measures both sensors simultaneously
- **Serial output**: Sends 1-byte status via single GPIO
- **Low cost**: ~$0.80 per chip
- **Minimal external components**: Just decoupling caps

### Components Required
- **U1**: ATtiny85 Microcontroller - $0.80
- **C1**: 100nF Ceramic Capacitor (power decoupling) - $0.05
- **R1, R2**: 100kΩ Resistors (input protection) - $0.10
- **C2, C3**: 10nF Ceramic Capacitors (input filtering) - $0.10

**Total Cost**: ~$1.05

### Circuit Schematic

```
Sensor 1 Input (0V / 1.68V / 3.47V)
    |
    +---[R1: 100k]---+---[C2: 10nF]---GND
                     |
                     +---> PB3 (ADC3, Pin 2)
                     
Sensor 2 Input (0V / 1.68V / 3.47V)
    |
    +---[R2: 100k]---+---[C3: 10nF]---GND
                     |
                     +---> PB4 (ADC2, Pin 3)

                    ATtiny85
                   +--------+
    RESET  -------|1  U  8|------- VCC (3.3V)
    ADC3   -------|2     7|------- PB2 (Serial TX to RP2040)
    ADC2   -------|3     6|------- PB1 (unused)
    GND    -------|4     5|------- PB0 (unused)
                   +--------+

    VCC---[C1: 100nF]---GND  (Power decoupling)
```

### ATtiny85 Pin Connections
- **Pin 1**: RESET (pull-up via internal resistor)
- **Pin 2**: PB3/ADC3 (Sensor 1 input)
- **Pin 3**: PB4/ADC2 (Sensor 2 input)
- **Pin 4**: GND
- **Pin 5**: PB0 (unused, can be used for status LED)
- **Pin 6**: PB1 (unused)
- **Pin 7**: PB2 (Serial TX output to RP2040 GPIO)
- **Pin 8**: VCC (3.3V)

### Data Protocol

#### Output Format (1 byte via serial)
```
Bit 7-6: Sensor 1 State
Bit 5-4: Sensor 2 State
Bit 3-0: Reserved (0x0) or checksum

State Encoding:
00 = LOW (0V)
01 = INACTIVE (1.68V)
10 = HIGH (3.47V)
11 = ERROR/INVALID
```

#### Example Values
| Sensor 1 | Sensor 2 | Byte Value | Binary |
|----------|----------|------------|--------|
| LOW      | LOW      | 0x00       | 00000000 |
| LOW      | INACTIVE | 0x10       | 00010000 |
| LOW      | HIGH     | 0x20       | 00100000 |
| INACTIVE | LOW      | 0x40       | 01000000 |
| INACTIVE | INACTIVE | 0x50       | 01010000 |
| INACTIVE | HIGH     | 0x60       | 01100000 |
| HIGH     | LOW      | 0x80       | 10000000 |
| HIGH     | INACTIVE | 0x90       | 10010000 |
| HIGH     | HIGH     | 0xA0       | 10100000 |

### ATtiny85 Firmware (Arduino/C++)

```cpp
// ATtiny85 Voltage Detector Firmware
// Compile with ATTinyCore: https://github.com/SpenceKonde/ATTinyCore

#include <SoftwareSerial.h>

// Pin definitions
#define SENSOR1_PIN A3  // PB3, ADC3
#define SENSOR2_PIN A2  // PB4, ADC2
#define TX_PIN 2        // PB2

// Voltage thresholds (ADC values for 3.3V reference)
// ADC = (Vin / Vref) * 1023
#define THRESHOLD_LOW_INACTIVE  512   // ~1.0V threshold
#define THRESHOLD_INACTIVE_HIGH 768   // ~2.5V threshold

// State definitions
#define STATE_LOW      0b00
#define STATE_INACTIVE 0b01
#define STATE_HIGH     0b10
#define STATE_ERROR    0b11

SoftwareSerial serial(255, TX_PIN); // RX unused, TX on PB2

void setup() {
  // Configure ADC reference to VCC (3.3V)
  analogReference(DEFAULT);
  
  // Initialize serial at 9600 baud
  serial.begin(9600);
  
  // Small delay for stability
  delay(100);
}

uint8_t readSensorState(uint8_t pin) {
  int adcValue = analogRead(pin);
  
  if (adcValue < THRESHOLD_LOW_INACTIVE) {
    return STATE_LOW;
  } else if (adcValue < THRESHOLD_INACTIVE_HIGH) {
    return STATE_INACTIVE;
  } else {
    return STATE_HIGH;
  }
}

void loop() {
  // Read both sensors
  uint8_t sensor1State = readSensorState(SENSOR1_PIN);
  uint8_t sensor2State = readSensorState(SENSOR2_PIN);
  
  // Pack into single byte
  uint8_t statusByte = (sensor1State << 6) | (sensor2State << 4);
  
  // Send via serial
  serial.write(statusByte);
  
  // Update rate: 100Hz (10ms)
  delay(10);
}
```

### RP2040 Receiver Code

```cpp
// RP2040 Receiver Code
#include "hardware/uart.h"
#include "hardware/gpio.h"

#define RX_PIN 16  // Connect to ATtiny85 TX pin

// State definitions
typedef enum {
  STATE_LOW = 0,
  STATE_INACTIVE = 1,
  STATE_HIGH = 2,
  STATE_ERROR = 3
} SensorState;

void setup_serial_receiver() {
  // Initialize UART0 at 9600 baud
  uart_init(uart0, 9600);
  gpio_set_function(RX_PIN, GPIO_FUNC_UART);
  
  // Enable UART RX
  uart_set_hw_flow(uart0, false, false);
  uart_set_format(uart0, 8, 1, UART_PARITY_NONE);
  uart_set_fifo_enabled(uart0, true);
}

void read_sensor_states(SensorState *sensor1, SensorState *sensor2) {
  if (uart_is_readable(uart0)) {
    uint8_t statusByte = uart_getc(uart0);
    
    // Extract states
    *sensor1 = (SensorState)((statusByte >> 6) & 0x03);
    *sensor2 = (SensorState)((statusByte >> 4) & 0x03);
  }
}

// Usage example
void loop() {
  SensorState sensor1, sensor2;
  read_sensor_states(&sensor1, &sensor2);
  
  // Process states
  if (sensor1 == STATE_HIGH && sensor2 == STATE_HIGH) {
    // Both sensors high
  }
}
```

---

## Option 2: 74HC4051 Analog Multiplexer + Shared ADC

### Concept
Use analog multiplexer to route both sensor inputs to RP2040's ADC, then read states via software.

### Advantages
- **No programming needed**: Pure hardware solution
- **Uses RP2040's ADC**: No external microcontroller
- **Low cost**: ~$0.50

### Disadvantages
- **Requires 1 ADC pin + 1 control GPIO**: Not truly single-pin
- **Software overhead**: Must read ADC and decode states
- **Slower**: Sequential reading of sensors

### Not Recommended
This doesn't meet the "single GPIO" requirement as effectively as the ATtiny85 solution.

---

## Option 3: I2C ADC (ADS1015/ADS1115)

### Components
- **ADS1015**: 12-bit, 4-channel I2C ADC - $3.50
- **ADS1115**: 16-bit, 4-channel I2C ADC - $5.00

### Advantages
- **High precision**: 12-bit or 16-bit resolution
- **I2C interface**: Only 2 pins (SDA, SCL)
- **Multiple channels**: Can read 4 sensors
- **Programmable gain**: Software-configurable

### Disadvantages
- **Higher cost**: $3.50-$5.00
- **Requires I2C**: Not single GPIO pin
- **Overkill**: Too much precision for 3-state detection

### Not Recommended
Too expensive and complex for this application.

---

## Option 4: Shift Register with Comparators

### Concept
Use dual comparators (LM393) for each sensor, then shift out results via 74HC165 shift register.

### Components
- **2x LM393**: Dual comparators - $0.60
- **74HC165**: 8-bit shift register - $0.40
- **Resistors/caps**: ~$0.50

**Total Cost**: ~$1.50

### Advantages
- **True single GPIO**: Serial data output
- **No programming**: Pure hardware
- **Parallel capture**: Reads both sensors simultaneously

### Disadvantages
- **More components**: Higher complexity
- **Fixed thresholds**: Requires resistor changes to adjust
- **Larger PCB footprint**

### Circuit Overview
```
Sensor 1 ---> [Comparator 1] ---> Bit 7
          ---> [Comparator 2] ---> Bit 6
Sensor 2 ---> [Comparator 3] ---> Bit 5
          ---> [Comparator 4] ---> Bit 4
                    |
                    v
              [74HC165 Shift Register]
                    |
                    v
              Serial Out ---> RP2040 GPIO
```

---

## Comparison Table

| Solution | Cost | GPIO Pins | Programmable | Complexity | Recommended |
|----------|------|-----------|--------------|------------|-------------|
| **ATtiny85** | $1.05 | 1 (serial) | ✅ Yes | Low | ⭐ **YES** |
| 74HC4051 Mux | $0.50 | 2 (ADC+ctrl) | ✅ Yes | Medium | ❌ No |
| I2C ADC | $3.50+ | 2 (I2C) | ✅ Yes | Low | ❌ No (expensive) |
| Shift Register | $1.50 | 1 (serial) | ❌ No | High | ⚠️ Maybe |

---

## Recommendation: ATtiny85 Solution

The **ATtiny85 microcontroller** is the best choice because:

1. ✅ **Single GPIO pin**: Serial output to RP2040
2. ✅ **Dual sensor support**: Reads both sensors simultaneously
3. ✅ **Programmable thresholds**: No manual adjustment needed
4. ✅ **Low cost**: ~$1.05 total
5. ✅ **Simple interface**: Standard serial communication
6. ✅ **Compact**: Minimal external components
7. ✅ **Flexible**: Can add features via firmware updates

### Programming the ATtiny85

#### Option 1: Traditional ISP Programming (6-pin)
**Using Arduino IDE + USBasp programmer**:
```bash
# Install ATTinyCore board support
# Tools -> Board -> Boards Manager -> Search "ATTinyCore"

# Settings:
# Board: ATtiny25/45/85
# Chip: ATtiny85
# Clock: 8 MHz (internal)
# Programmer: USBasp

# Upload firmware
# Sketch -> Upload Using Programmer
```

**ISP Connections**:
- MOSI (to PB0)
- MISO (to PB1)
- SCK (to PB2)
- RESET (to Pin 1)
- VCC, GND

#### Option 2: Single-Wire Programming?

**⚠️ ATtiny85 does NOT support true single-wire programming.** However, there are workarounds:

##### A) Micronucleus Bootloader (USB Programming)
The ATtiny85 can be pre-programmed with the **Micronucleus** bootloader, enabling USB programming via a single data line (D-).

**Requirements**:
- Pre-program Micronucleus bootloader via ISP (one-time)
- Add USB interface circuit (D- on PB3, D+ on PB4)
- Uses 2 GPIO pins for USB, but programming is via USB cable

**Micronucleus Circuit**:
```
ATtiny85 USB Interface:
PB3 (Pin 2) ---[68Ω]--- USB D-
PB4 (Pin 3) ---[68Ω]--- USB D+
PB3 --------[1.5kΩ]--- VCC (pull-up for USB detection)
USB VCC --- [3.6V Zener] --- GND (voltage protection)
```

**Advantages**:
- ✅ Program via USB cable (no external programmer needed after initial setup)
- ✅ Widely supported (Digispark boards use this)
- ✅ Can reprogram in-circuit

**Disadvantages**:
- ❌ Requires initial ISP programming to install bootloader
- ❌ Uses 2 GPIO pins (PB3, PB4) for USB - **conflicts with ADC inputs!**
- ❌ Bootloader takes ~2KB of flash
- ❌ Not truly "single wire"

##### B) UPDI Programming (Not Available on ATtiny85)
**UPDI (Unified Program and Debug Interface)** is a true single-wire programming interface, but it's only available on **newer ATtiny chips**:
- ATtiny202, 402, 804, 1604, etc. (0-series, 1-series, 2-series)
- **NOT available on ATtiny85** (classic ATtiny)

##### C) Alternative: Use ATtiny412 with UPDI ⭐ RECOMMENDED

If single-wire programming is critical, consider switching to **ATtiny412** or **ATtiny1614**:

**ATtiny412 Advantages**:
- ✅ True single-wire programming via UPDI
- ✅ Same price as ATtiny85 (~$0.80)
- ✅ More modern architecture
- ✅ Better ADC (10-bit)
- ✅ Can be programmed with RP2040 as UPDI programmer
- ✅ No bootloader needed - full flash available

**UPDI Programming with RP2040**:
```
RP2040 GPIO --[4.7kΩ]-- ATtiny412 UPDI pin
                |
              [10nF]
                |
               GND
```

The RP2040 can act as a UPDI programmer using **pymcuprog** or **SerialUPDI**.

**ATtiny412 Circuit** (replaces ATtiny85):
```
                    ATtiny412
                   +--------+
    VCC    -------|1  U  8|------- VCC (3.3V)
    GND    -------|2     7|------- UPDI (programming)
    ADC1   -------|3     6|------- TX (Serial out)
    ADC0   -------|4     5|------- (unused)
                   +--------+
```

**ATtiny412 Firmware** (same logic, different pin names):
```cpp
// ATtiny412 Voltage Detector Firmware
// Compile with megaTinyCore: https://github.com/SpenceKonde/megaTinyCore

#include <SoftwareSerial.h>

// Pin definitions for ATtiny412
#define SENSOR1_PIN PIN_PA6  // ADC0
#define SENSOR2_PIN PIN_PA7  // ADC1
#define TX_PIN PIN_PA1       // Serial TX

// Rest of code identical to ATtiny85 version
```

**Programming ATtiny412 via RP2040**:
```bash
# Install pymcuprog
pip install pymcuprog

# Program via RP2040 UPDI interface
pymcuprog write -t uart -u /dev/ttyACM0 -d attiny412 -f firmware.hex

# Or use SerialUPDI in Arduino IDE
# Tools -> Programmer -> SerialUPDI
# Tools -> Port -> (RP2040 serial port)
# Sketch -> Upload Using Programmer
```

#### Option 3: Pre-programmed Chips
Order pre-programmed ATtiny85 or ATtiny412 chips from suppliers:
- Provide hex file to manufacturer
- Chips arrive ready to use
- No programming hardware needed

---

## Recommended Programming Strategy

### For Production:
1. **Pre-program chips** before soldering (via ISP programmer for ATtiny85, or UPDI for ATtiny412)
2. Or order **pre-programmed chips** from supplier
3. Or use **ATtiny412 with UPDI** for in-circuit programming via RP2040

### For Development:
1. **ATtiny85**: Use ISP programmer (USBasp, Arduino as ISP)
2. **ATtiny412**: Use RP2040 as UPDI programmer (single wire!)
3. Or use **Digispark board** (ATtiny85 with USB bootloader pre-installed)

### For In-Field Updates:
- **ATtiny85**: Not practical without ISP access (6 pins)
- **ATtiny412**: ✅ Can be reprogrammed via single UPDI wire from RP2040

### Summary: Single-Wire Programming

| Chip | Single-Wire Programming | Method | Recommended |
|------|------------------------|--------|-------------|
| ATtiny85 | ❌ No | Requires 6-pin ISP | For pre-programmed only |
| ATtiny412 | ✅ Yes | UPDI via RP2040 | ⭐ **Best for field updates** |

---

## Enhanced Protocol (Optional)

For more robust communication, add framing and checksums:

```cpp
// Enhanced protocol with start byte and checksum
void sendStatus(uint8_t sensor1State, uint8_t sensor2State) {
  uint8_t statusByte = (sensor1State << 6) | (sensor2State << 4);
  uint8_t checksum = ~statusByte; // Simple inversion checksum
  
  serial.write(0xFF);        // Start byte
  serial.write(statusByte);  // Data byte
  serial.write(checksum);    // Checksum byte
}
```

This provides error detection and frame synchronization for noisy environments.

