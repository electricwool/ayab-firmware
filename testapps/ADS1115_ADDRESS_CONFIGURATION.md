# ADS1115 I2C Address Configuration - Hardware Setup

## Overview
The ADS1115 has a single **ADDR pin** that determines its I2C address. By connecting this pin to different voltages, you can set 4 different addresses, allowing up to 4 ADS1115 modules on the same I2C bus.

---

## ADDR Pin Configuration

### Physical Connection Table

| ADDR Pin Connected To | I2C Address (7-bit) | Hex Address | Binary |
|----------------------|---------------------|-------------|--------|
| **GND** | 72 | **0x48** | 1001000 |
| **VDD** | 73 | **0x49** | 1001001 |
| **SDA** | 74 | **0x4A** | 1001010 |
| **SCL** | 75 | **0x4B** | 1001011 |

---

## Hardware Wiring Examples

### Single ADS1115 (Address 0x48)

```
ADS1115 Module
+----------------+
|VDD         SDA |----> RP2040 SDA
|GND         SCL |----> RP2040 SCL
|A0        ALERT |----> RP2040 GPIO
|A1          ADD |----> GND  ← Connect to GND for 0x48
|A2             |
|A3             |
+----------------+

Result: I2C Address = 0x48
```

### Two ADS1115 Modules (Addresses 0x48 and 0x49)

```
ADS1115 #1                    ADS1115 #2
+----------------+            +----------------+
|VDD         SDA |---+    +---|VDD         SDA |
|GND         SCL |---+    +---|GND         SCL |
|A0        ALERT |   |    |   |A0        ALERT |
|A1          ADD |---+    |   |A1          ADD |---+
|A2             |   |    |   |A2             |   |
|A3             |   |    |   |A3             |   |
+----------------+   |    |   +----------------+   |
        |            |    |           |            |
       GND           |    |          VDD           |
                     |    |                        |
                     v    v                        v
              RP2040 I2C Bus                    3.3V
              (SDA/SCL)

ADS1115 #1: ADDR → GND = 0x48
ADS1115 #2: ADDR → VDD = 0x49
```

### Three ADS1115 Modules (Addresses 0x48, 0x49, 0x4A)

```
Module #1: ADDR → GND = 0x48
Module #2: ADDR → VDD = 0x49
Module #3: ADDR → SDA = 0x4A

                    I2C Bus
                    SDA  SCL
                     |    |
    +----------------+----+----+
    |                |    |    |
ADS1115 #1      ADS1115 #2    ADS1115 #3
ADDR → GND      ADDR → VDD    ADDR → SDA
(0x48)          (0x49)        (0x4A)
```

### Four ADS1115 Modules (Maximum - All Addresses)

```
Module #1: ADDR → GND = 0x48
Module #2: ADDR → VDD = 0x49
Module #3: ADDR → SDA = 0x4A
Module #4: ADDR → SCL = 0x4B

All connected to same I2C bus (SDA/SCL)
Total: 16 ADC channels (4 modules × 4 channels)
```

---

## Detailed Wiring Diagrams

### Configuration 1: ADDR → GND (Address 0x48)

```
ADS1115
   |
  ADD pin
   |
   +---[wire]--- GND

Simple: Just connect ADDR pin directly to GND
```

### Configuration 2: ADDR → VDD (Address 0x49)

```
ADS1115
   |
  ADD pin
   |
   +---[wire]--- VDD (3.3V or 5V)

Simple: Just connect ADDR pin directly to VDD
```

### Configuration 3: ADDR → SDA (Address 0x4A)

```
ADS1115
   |
  ADD pin
   |
   +---[wire]---+--- SDA line (I2C data)
                |
             [4.7kΩ pull-up to VDD]

Connect ADDR pin to the SDA line
(The existing I2C pull-up resistor provides the voltage)
```

### Configuration 4: ADDR → SCL (Address 0x4B)

```
ADS1115
   |
  ADD pin
   |
   +---[wire]---+--- SCL line (I2C clock)
                |
             [4.7kΩ pull-up to VDD]

Connect ADDR pin to the SCL line
(The existing I2C pull-up resistor provides the voltage)
```

---

## Practical PCB Layout Example

### Two ADS1115 Modules on PCB

```
                    RP2040
                      |
                   I2C Bus
                   SDA  SCL
                    |    |
        +-----------+----+-----------+
        |           |    |           |
        |       [4.7k] [4.7k]        |
        |         |    |             |
        |        VDD  VDD            |
        |                            |
    +---+---+                    +---+---+
    |ADS1115|                    |ADS1115|
    |  #1   |                    |  #2   |
    +-------+                    +-------+
    |  ADDR |                    |  ADDR |
    +---+---+                    +---+---+
        |                            |
       GND                          VDD
        
Module #1: 0x48                Module #2: 0x49
```

---

## Step-by-Step Hardware Setup

### For 2 ADS1115 Modules:

**Module #1 (Address 0x48):**
1. Connect VDD to 3.3V
2. Connect GND to ground
3. Connect SDA to RP2040 SDA (with 4.7kΩ pull-up)
4. Connect SCL to RP2040 SCL (with 4.7kΩ pull-up)
5. **Connect ADDR to GND** ← This sets address to 0x48

**Module #2 (Address 0x49):**
1. Connect VDD to 3.3V
2. Connect GND to ground
3. Connect SDA to RP2040 SDA (same line as Module #1)
4. Connect SCL to RP2040 SCL (same line as Module #1)
5. **Connect ADDR to VDD** ← This sets address to 0x49

---

## Common Mistakes to Avoid

### ❌ Wrong: Leaving ADDR Floating
```
ADS1115
   |
  ADD pin
   |
  (not connected)  ← DON'T DO THIS!
```
**Problem:** Floating pin causes unpredictable address, may conflict with other devices.

### ❌ Wrong: Using Resistor Divider
```
ADS1115
   |
  ADD pin
   |
   +---[10k]--- VDD
   |
   +---[10k]--- GND
```
**Problem:** ADDR pin needs to be at specific logic levels (GND, VDD, SDA, or SCL), not intermediate voltages.

### ✅ Correct: Direct Connection
```
ADS1115
   |
  ADD pin
   |
   +---[wire]--- GND (or VDD, SDA, SCL)
```
**Solution:** Always connect ADDR directly to one of the four options.

---

## Verification with I2C Scanner

After wiring, verify addresses with I2C scanner:

```cpp
// RP2040 I2C Scanner
#include "hardware/i2c.h"

void i2c_scan() {
  printf("Scanning I2C bus...\n");
  
  for (int addr = 0; addr < 128; addr++) {
    uint8_t rxdata;
    int ret = i2c_read_blocking(i2c0, addr, &rxdata, 1, false);
    
    if (ret >= 0) {
      printf("Found device at 0x%02X\n", addr);
    }
  }
}
```

**Expected Output:**
```
Scanning I2C bus...
Found device at 0x48  ← ADS1115 #1 (ADDR→GND)
Found device at 0x49  ← ADS1115 #2 (ADDR→VDD)
```

---

## Software Configuration

After hardware setup, configure in software:

```cpp
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads1;
Adafruit_ADS1115 ads2;

void setup() {
  // Initialize ADS1115 #1 at address 0x48
  if (!ads1.begin(0x48)) {
    Serial.println("Failed to initialize ADS1115 #1");
  }
  
  // Initialize ADS1115 #2 at address 0x49
  if (!ads2.begin(0x49)) {
    Serial.println("Failed to initialize ADS1115 #2");
  }
  
  Serial.println("Both ADS1115 modules initialized");
}

void loop() {
  // Read from ADS1115 #1
  int16_t adc1 = ads1.readADC_SingleEnded(0);
  
  // Read from ADS1115 #2
  int16_t adc2 = ads2.readADC_SingleEnded(0);
  
  Serial.printf("ADS1: %d, ADS2: %d\n", adc1, adc2);
  delay(100);
}
```

---

## Quick Reference Table

| Number of Modules | ADDR Connections | Addresses | Total Channels |
|-------------------|------------------|-----------|----------------|
| 1 | GND | 0x48 | 4 |
| 2 | GND, VDD | 0x48, 0x49 | 8 |
| 3 | GND, VDD, SDA | 0x48, 0x49, 0x4A | 12 |
| 4 | GND, VDD, SDA, SCL | 0x48, 0x49, 0x4A, 0x4B | 16 |

---

## Summary

### To Configure Non-Conflicting Addresses:

1. **Module #1**: Connect ADDR pin to **GND** → Address 0x48
2. **Module #2**: Connect ADDR pin to **VDD** → Address 0x49
3. **Module #3**: Connect ADDR pin to **SDA** → Address 0x4A
4. **Module #4**: Connect ADDR pin to **SCL** → Address 0x4B

**That's it!** No resistors, no complex circuitry - just a simple wire connection to set the address.

All modules share the same SDA/SCL lines with 4.7kΩ pull-up resistors to VDD.
