# Voltage Detector Circuit - ATtiny412 with UPDI Programming

## Overview
Programmable digital circuit for detecting three voltage states (0V, 1.68V, 3.47V) on **two sensor inputs** simultaneously, outputting state information via a **single GPIO pin** using serial communication.

**Key Feature**: Single-wire UPDI programming interface allows in-field firmware updates via RP2040!

---

## ATtiny412 Microcontroller Solution

### Why ATtiny412?
- ✅ **Programmable thresholds**: No manual adjustment needed
- ✅ **Dual ADC inputs**: Measures both sensors simultaneously (10-bit resolution)
- ✅ **Serial output**: Sends 1-byte status via single GPIO
- ✅ **Low cost**: ~$0.80 per chip
- ✅ **Minimal external components**: Just decoupling caps
- ✅ **Single-wire UPDI programming**: Can be programmed via RP2040!
- ✅ **In-field updates**: Reprogram without removing chip

### Components Required
- **U1**: ATtiny412 Microcontroller - $0.80
- **C1**: 100nF Ceramic Capacitor (power decoupling) - $0.05
- **R1, R2**: 100kΩ Resistors (input protection) - $0.10
- **C2, C3**: 10nF Ceramic Capacitors (input filtering) - $0.10
- **R3**: 4.7kΩ Resistor (UPDI programming interface) - $0.05
- **C4**: 10nF Ceramic Capacitor (UPDI filtering) - $0.05

**Total Cost**: ~$1.15

---

## Circuit Schematic

```
Sensor 1 Input (0V / 1.68V / 3.47V)
    |
    +---[R1: 100k]---+---[C2: 10nF]---GND
                     |
                     +---> PA6 (AIN6, Pin 3)
                     
Sensor 2 Input (0V / 1.68V / 3.47V)
    |
    +---[R2: 100k]---+---[C3: 10nF]---GND
                     |
                     +---> PA7 (AIN7, Pin 4)

                    ATtiny412
                   +--------+
    VCC    -------|1  U  8|------- VCC (3.3V)
    GND    -------|2     7|------- PA0/UPDI
    PA6    -------|3     6|------- PA1 (Serial TX)
    PA7    -------|4     5|------- PA2 (unused)
                   +--------+

    VCC---[C1: 100nF]---GND  (Power decoupling)
    
    Serial Output:
    PA1 (Pin 6) ---------> RP2040 UART RX GPIO
    
    UPDI Programming Interface:
    RP2040 GPIO ---[R3: 4.7k]---+--- PA0/UPDI (Pin 7)
                                |
                             [C4: 10nF]
                                |
                               GND
```

---

## Pin Connections

### ATtiny412 Pinout
- **Pin 1**: VCC (3.3V power supply)
- **Pin 2**: GND (ground)
- **Pin 3**: PA6/AIN6 (Sensor 1 ADC input)
- **Pin 4**: PA7/AIN7 (Sensor 2 ADC input)
- **Pin 5**: PA2 (unused, available for status LED)
- **Pin 6**: PA1 (Serial TX output to RP2040 UART RX)
- **Pin 7**: PA0/UPDI (programming interface, can be GPIO after programming)
- **Pin 8**: VCC (3.3V power supply)

### RP2040 Connections
- **UART RX GPIO** (e.g., GP16): Connect to ATtiny412 PA1 (Pin 6)
- **Programming GPIO** (e.g., GP15): Connect to ATtiny412 PA0/UPDI (Pin 7) via 4.7kΩ resistor
- **3.3V**: Connect to ATtiny412 VCC (Pins 1 & 8)
- **GND**: Connect to ATtiny412 GND (Pin 2)

---

## Data Protocol

### Output Format (1 byte via serial)
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

### Example Values
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

---

## ATtiny412 Firmware

### Complete Firmware (Arduino/C++)

```cpp
// ATtiny412 Voltage Detector Firmware
// Compile with megaTinyCore: https://github.com/SpenceKonde/megaTinyCore

#include <SoftwareSerial.h>

// Pin definitions for ATtiny412
#define SENSOR1_PIN PIN_PA6  // AIN6, Pin 3
#define SENSOR2_PIN PIN_PA7  // AIN7, Pin 4
#define TX_PIN PIN_PA1       // Serial TX, Pin 6

// Voltage thresholds (ADC values for 3.3V reference)
// ATtiny412 has 10-bit ADC: ADC = (Vin / Vref) * 1023
#define THRESHOLD_LOW_INACTIVE  310   // ~1.0V threshold (1.0/3.3 * 1023)
#define THRESHOLD_INACTIVE_HIGH 768   // ~2.5V threshold (2.5/3.3 * 1023)

// State definitions
#define STATE_LOW      0b00
#define STATE_INACTIVE 0b01
#define STATE_HIGH     0b10
#define STATE_ERROR    0b11

SoftwareSerial serial(255, TX_PIN); // RX unused, TX on PA1

void setup() {
  // Configure ADC reference to VCC (3.3V)
  analogReference(VDD);
  
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

---

## RP2040 Receiver Code

```cpp
// RP2040 Receiver Code
#include "hardware/uart.h"
#include "hardware/gpio.h"

#define RX_PIN 16  // Connect to ATtiny412 TX pin (PA1)

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
void main() {
  setup_serial_receiver();
  
  SensorState sensor1, sensor2;
  
  while (1) {
    read_sensor_states(&sensor1, &sensor2);
    
    // Process states
    if (sensor1 == STATE_HIGH && sensor2 == STATE_HIGH) {
      // Both sensors high
    } else if (sensor1 == STATE_LOW && sensor2 == STATE_LOW) {
      // Both sensors low
    }
    // ... handle other states
  }
}
```

---

## Programming the ATtiny412 via RP2040

### Hardware Setup

1. **Connect UPDI programming interface**:
   ```
   RP2040 GPIO (e.g., GP15) ---[4.7kΩ]---+--- ATtiny412 PA0/UPDI (Pin 7)
                                         |
                                      [10nF]
                                         |
                                        GND
   ```

2. **Power connections**:
   - RP2040 3.3V → ATtiny412 VCC (Pins 1 & 8)
   - RP2040 GND → ATtiny412 GND (Pin 2)

### Software Setup

#### Option A: Using Arduino IDE with SerialUPDI

**Step 1: Install megaTinyCore**
```
1. Open Arduino IDE
2. File → Preferences
3. Additional Boards Manager URLs: 
   http://drazzy.com/package_drazzy.com_index.json
4. Tools → Board → Boards Manager
5. Search "megaTinyCore" and install
```

**Step 2: Configure Board Settings**
```
Tools → Board → megaTinyCore → ATtiny412/402/212/202
Tools → Chip → ATtiny412
Tools → Clock → 10MHz internal
Tools → millis()/micros() → Enabled
Tools → Programmer → SerialUPDI
Tools → Port → (Select RP2040 serial port)
```

**Step 3: Configure SerialUPDI**
Create a file `programmers.txt` in Arduino hardware folder:
```
serialupdi.name=SerialUPDI
serialupdi.communication=serial
serialupdi.protocol=serialupdi
serialupdi.program.protocol=serialupdi
serialupdi.program.tool=serialupdi
serialupdi.program.extra_params=-P{serial.port}
```

**Step 4: Upload Firmware**
```
Sketch → Upload Using Programmer
```

#### Option B: Using pymcuprog (Command Line)

**Installation**:
```bash
pip install pymcuprog
```

**Programming**:
```bash
# Compile firmware to hex file first in Arduino IDE:
# Sketch → Export Compiled Binary

# Then program via RP2040:
pymcuprog write -t uart -u /dev/ttyACM0 -d attiny412 -f firmware.hex

# Verify programming:
pymcuprog read -t uart -u /dev/ttyACM0 -d attiny412 -m flash

# Read device info:
pymcuprog ping -t uart -u /dev/ttyACM0 -d attiny412
```

**Windows**:
```cmd
pymcuprog write -t uart -u COM3 -d attiny412 -f firmware.hex
```

#### Option C: Using RP2040 as Dedicated UPDI Programmer

Flash the RP2040 with UPDI programmer firmware to make it a permanent programmer:

**Step 1: Flash RP2040 with UPDI Programmer Firmware**
```bash
# Clone jtag2updi repository
git clone https://github.com/ElTangas/jtag2updi

# Flash to RP2040 (requires modification for RP2040)
# Or use pre-built UPDI programmer firmware
```

**Step 2: Use avrdude**
```bash
avrdude -c jtag2updi -P /dev/ttyACM0 -p attiny412 -U flash:w:firmware.hex
```

---

## In-Field Programming Workflow

### Initial Programming (Factory)
1. Connect ATtiny412 UPDI pin to RP2040 programming GPIO
2. Flash firmware via UPDI
3. Test functionality
4. Deploy

### Field Updates
1. Connect to RP2040 via USB/Serial
2. Put RP2040 into "programming mode"
3. RP2040 programs ATtiny412 via UPDI
4. Verify and restart

### Example: RP2040 Field Update Code

```cpp
// RP2040 code to program ATtiny412 in the field
#include "hardware/uart.h"
#include "hardware/gpio.h"

#define UPDI_PIN 15  // GPIO connected to ATtiny412 UPDI

void enter_programming_mode() {
  // Configure UPDI pin for programming
  gpio_init(UPDI_PIN);
  gpio_set_dir(UPDI_PIN, GPIO_OUT);
  
  // Send UPDI break signal
  gpio_put(UPDI_PIN, 0);
  sleep_ms(100);
  gpio_put(UPDI_PIN, 1);
  
  // Initialize UPDI UART
  uart_init(uart1, 225000); // UPDI baud rate
  gpio_set_function(UPDI_PIN, GPIO_FUNC_UART);
}

void program_attiny412(const uint8_t *firmware, size_t size) {
  enter_programming_mode();
  
  // Send UPDI commands to program flash
  // (Implementation requires UPDI protocol)
  
  // Verify programming
  // Exit programming mode
}
```

---

## Advantages Over ATtiny85

| Feature | ATtiny85 | ATtiny412 |
|---------|----------|-----------|
| **Programming** | 6-pin ISP | ✅ Single-wire UPDI |
| **ADC Resolution** | 8-bit | ✅ 10-bit (better precision) |
| **In-field Updates** | ❌ Difficult | ✅ Easy via RP2040 |
| **Cost** | $0.80 | $0.80 (same) |
| **Flash Memory** | 8KB | 4KB |
| **Architecture** | Classic AVR | ✅ Modern AVR |
| **Programming Hardware** | Requires ISP programmer | ✅ RP2040 can program it |

---

## Troubleshooting

### UPDI Programming Issues

**Problem**: Cannot connect to ATtiny412
- Check 4.7kΩ resistor is present
- Verify 3.3V power supply
- Ensure GND is connected
- Try lower baud rate (115200 instead of 225000)

**Problem**: Programming fails midway
- Add 10nF capacitor on UPDI line to GND
- Check power supply stability
- Verify firmware hex file is valid

**Problem**: Device not responding after programming
- Check if UPDI pin is configured as GPIO (fuse setting)
- Verify firmware uploaded correctly
- Try erasing chip and reprogramming

### Serial Communication Issues

**Problem**: No data received on RP2040
- Verify baud rate matches (9600)
- Check TX pin connection (PA1 to RP2040 RX)
- Test with oscilloscope/logic analyzer
- Verify ATtiny412 is running (check power LED)

---

## PCB Layout Recommendations

1. **Keep UPDI trace short** and away from noisy signals
2. **Place decoupling caps close** to VCC pins
3. **Add test points** for UPDI, TX, and sensor inputs
4. **Ground plane** for stable ADC readings
5. **Separate analog and digital grounds** if possible
6. **Add programming header** for easy access to UPDI pin

### Suggested PCB Layout
```
    [Sensor 1 Input]     [Sensor 2 Input]
           |                    |
        [R1,C2]              [R2,C3]
           |                    |
    +------+--------------------+------+
    |                                  |
    |         ATtiny412                |
    |                                  |
    +--+--+--+--+--+--+--+--+--+--+---+
       |  |  |  |  |  |  |  |  |  |
      VCC GND     PA1 PA0        VCC
                   |   |
                   |   +--[R3,C4]--[UPDI Header]
                   |
                   +--[TX to RP2040]
```

---

## Bill of Materials (BOM)

| Qty | Part Number | Description | Unit Price | Total |
|-----|-------------|-------------|------------|-------|
| 1   | ATtiny412   | Microcontroller | $0.80 | $0.80 |
| 2   | 100kΩ 1/4W  | Resistor (input protection) | $0.05 | $0.10 |
| 1   | 4.7kΩ 1/4W  | Resistor (UPDI) | $0.05 | $0.05 |
| 1   | 100nF       | Ceramic Cap (power) | $0.05 | $0.05 |
| 3   | 10nF        | Ceramic Cap (filtering) | $0.05 | $0.15 |
| **TOTAL** | | | | **$1.15** |

---

## Summary

The ATtiny412 provides a complete solution for dual-sensor voltage detection with these key benefits:

✅ **Single GPIO output** to RP2040 (serial communication)  
✅ **Single-wire UPDI programming** via RP2040  
✅ **In-field firmware updates** without chip removal  
✅ **Low cost** (~$1.15 total)  
✅ **High precision** (10-bit ADC)  
✅ **Minimal components** (just one IC + passives)  
✅ **Programmable thresholds** (no manual adjustment)  

This design reduces GPIO usage from 4 pins (2 per sensor) to just 1 pin for both sensors, while adding the ability to update firmware in the field via the RP2040's programming interface.
