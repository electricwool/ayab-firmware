# Bipolar Voltage Measurement (-5V to +5V)

## Question: Can ATtiny412 Read -5V to +5V?

**Short Answer: NO** - The ATtiny412 (and most microcontrollers) can only read **0V to VCC** (typically 0-3.3V or 0-5V).

**Negative voltages will damage the ADC input pins.**

---

## Solutions for Bipolar Voltage Measurement

### Option 1: Voltage Level Shifter (Recommended for ATtiny412)

Add a simple resistor divider circuit to shift -5V to +5V into the 0-3.3V range:

```
Input Signal (-5V to +5V)
    |
    +---[R1: 10kΩ]---+---[C1: 10nF]---GND
                     |
                     +---> To ATtiny412 ADC
                     |
                  [R2: 10kΩ]
                     |
                    +3.3V (Vref/2)
```

**How it works:**
- Creates a voltage divider with offset
- -5V input → 0V at ADC
- 0V input → 1.65V at ADC
- +5V input → 3.3V at ADC

**Formula:**
```
Vout = (Vin + 5V) × (3.3V / 10V)
Vout = (Vin × 0.33) + 1.65V
```

**Advantages:**
- ✅ Simple (2 resistors + 1 cap)
- ✅ Low cost (~$0.15)
- ✅ Works with ATtiny412
- ✅ No additional ICs

**Disadvantages:**
- ❌ Reduces resolution (10V range compressed to 3.3V)
- ❌ Requires calibration in software
- ❌ Limited accuracy

---

### Option 2: Differential ADC with Instrumentation Amplifier

Use an instrumentation amplifier (INA826, AD8226) to condition the signal:

```
Input Signal (-5V to +5V)
    |
    +---> [INA826 Instrumentation Amp]
                |
                +---> 0-3.3V output ---> ATtiny412 ADC
                |
              Gain & Offset Control
```

**Components:**
- **INA826**: Instrumentation amplifier - $2.50
- **Resistors**: Gain setting - $0.10
- **Capacitors**: Filtering - $0.10

**Total Cost**: ~$2.70

**Advantages:**
- ✅ High accuracy
- ✅ Adjustable gain and offset
- ✅ Good noise rejection
- ✅ Works with ATtiny412

**Disadvantages:**
- ❌ Higher cost
- ❌ More complex circuit
- ❌ Requires dual power supply or rail-to-rail op-amp

---

### Option 3: Microcontroller with Built-in Differential ADC

Some microcontrollers have **differential ADC inputs** that can measure bipolar voltages:

#### A) STM32 Series (e.g., STM32F103)

**Features:**
- ✅ 12-bit differential ADC
- ✅ Can measure negative voltages (with external circuitry)
- ✅ Multiple channels
- ✅ Low cost (~$2.00)

**Limitations:**
- ⚠️ Still requires level shifting for -5V to +5V
- ⚠️ Differential mode measures difference between two pins, not true bipolar

#### B) ESP32

**Features:**
- ✅ 12-bit ADC
- ✅ Multiple channels
- ✅ Built-in WiFi/Bluetooth
- ✅ Low cost (~$3.00)

**Limitations:**
- ❌ ADC still 0-3.3V only
- ❌ Requires level shifting for bipolar voltages

#### C) Arduino Due (SAM3X8E)

**Features:**
- ✅ 12-bit ADC
- ✅ Differential mode available
- ✅ Multiple channels

**Limitations:**
- ❌ Expensive (~$40)
- ❌ Still requires level shifting

---

### Option 4: External ADC with Bipolar Input

Use a dedicated ADC chip with true bipolar input capability:

#### A) ADS1115 (16-bit I2C ADC)

**Features:**
- ✅ 16-bit resolution
- ✅ Differential inputs
- ✅ Programmable gain amplifier (PGA)
- ✅ Can measure ±6.144V with PGA
- ✅ I2C interface

**Cost**: ~$5.00

**Circuit:**
```
Input Signal (-5V to +5V)
    |
    +---> AIN0 (ADS1115)
    |
   GND ---> AIN1 (ADS1115)
    
    ADS1115 ---[I2C]---> ATtiny412 or RP2040
```

**Advantages:**
- ✅ True bipolar measurement
- ✅ High resolution (16-bit)
- ✅ Programmable gain
- ✅ No level shifting needed (with proper PGA setting)

**Disadvantages:**
- ❌ Expensive ($5)
- ❌ Requires I2C (2 pins)
- ❌ Slower sampling rate

#### B) MCP3421 (18-bit I2C ADC)

**Features:**
- ✅ 18-bit resolution
- ✅ Differential input
- ✅ Programmable gain (1x, 2x, 4x, 8x)
- ✅ I2C interface
- ✅ Can measure ±2.048V (with external reference)

**Cost**: ~$3.50

**Limitations:**
- ⚠️ ±2.048V range (would need voltage divider for ±5V)

---

### Option 5: Voltage-to-Frequency Converter

Use a V/F converter (LM331) to convert voltage to frequency:

```
Input Signal (-5V to +5V)
    |
    +---> [LM331 V/F Converter]
                |
                +---> Frequency output ---> ATtiny412 GPIO
```

**How it works:**
- Converts voltage to frequency (e.g., 0Hz at -5V, 10kHz at +5V)
- Microcontroller measures frequency using timer/counter
- No ADC needed!

**Advantages:**
- ✅ No ADC required
- ✅ Good noise immunity
- ✅ Long cable runs possible

**Disadvantages:**
- ❌ Slower response time
- ❌ More complex circuit
- ❌ Requires calibration

---

## Comparison Table

| Solution | Cost | Complexity | Accuracy | Bipolar Support | Recommended |
|----------|------|------------|----------|-----------------|-------------|
| **Voltage Divider** | $0.15 | Low | Medium | ⚠️ With offset | ⭐ Budget |
| **Instrumentation Amp** | $2.70 | Medium | High | ✅ Yes | ⭐ Precision |
| **ADS1115 ADC** | $5.00 | Low | Very High | ✅ Yes | ⭐ Best |
| **MCP3421 ADC** | $3.50 | Low | Very High | ⚠️ ±2V only | Maybe |
| **STM32 MCU** | $2.00 | Medium | High | ⚠️ With circuit | Alternative |
| **V/F Converter** | $2.00 | High | Medium | ✅ Yes | Niche |

---

## Recommended Solution: ADS1115 with ATtiny412

For true bipolar voltage measurement (-5V to +5V), use the **ADS1115** external ADC:

### Circuit Schematic

```
Sensor 1 Input (-5V to +5V)
    |
    +---[100kΩ]---+--- AIN0 (ADS1115)
                  |
               [10nF]
                  |
                 GND
                 
Sensor 2 Input (-5V to +5V)
    |
    +---[100kΩ]---+--- AIN2 (ADS1115)
                  |
               [10nF]
                  |
                 GND

                ADS1115
               +--------+
    VDD -------|1     10|------- SDA (to ATtiny412 PA1)
    GND -------|2      9|------- SCL (to ATtiny412 PA2)
    AIN0 ------|3      8|------- ADDR (to GND)
    AIN1 ------|4      7|------- ALERT (unused)
    AIN2 ------|5      6|------- AIN3 (GND reference)
               +--------+

    VDD = 3.3V
    Pull-ups on SDA/SCL: 4.7kΩ to VDD
```

### ADS1115 Configuration

```cpp
// Configure ADS1115 for ±6.144V range
// PGA = 1x (FSR = ±6.144V)
// This allows measurement of -5V to +5V

#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;

void setup() {
  ads.begin();
  
  // Set gain to 1x for ±6.144V range
  ads.setGain(GAIN_TWOTHIRDS);  // ±6.144V
  
  // Set data rate
  ads.setDataRate(RATE_ADS1115_860SPS);
}

int16_t readBipolarVoltage(uint8_t channel) {
  // Read differential voltage (channel vs GND)
  int16_t adc = ads.readADC_SingleEnded(channel);
  
  // Convert to voltage (-6.144V to +6.144V)
  float voltage = ads.computeVolts(adc);
  
  return (int16_t)(voltage * 1000);  // Return millivolts
}
```

### ATtiny412 with ADS1115 (I2C)

**Note:** ATtiny412 has limited I2C support. Consider using **ATtiny1614** or **RP2040** for better I2C performance.

---

## Alternative: Use RP2040 ADC with Level Shifter

The **RP2040** has a 12-bit ADC (0-3.3V). For bipolar measurement, use a level shifter:

### Simple Level Shifter Circuit

```
Input Signal (-5V to +5V)
    |
    +---[R1: 20kΩ]---+---[C1: 100nF]---GND
                     |
                     +---> RP2040 ADC (GP26-GP29)
                     |
                  [R2: 10kΩ]
                     |
                   +3.3V
                   
    +---[R3: 10kΩ]---+
    |                |
   GND            [D1: 3.3V Zener]
                     |
                    GND
```

**Voltage Mapping:**
- -5V input → 0V at ADC
- 0V input → 1.1V at ADC
- +5V input → 2.2V at ADC

**Software Conversion:**
```cpp
float readBipolarVoltage(uint8_t adc_pin) {
  uint16_t adc = analogRead(adc_pin);
  
  // Convert ADC (0-4095) to voltage (0-3.3V)
  float v_adc = (adc / 4095.0) * 3.3;
  
  // Convert to input voltage (-5V to +5V)
  float v_input = (v_adc - 1.1) * (10.0 / 2.2);
  
  return v_input;
}
```

---

## Summary

### For Your Application (0V, 1.68V, 3.47V):
- ✅ **No bipolar support needed** - all voltages are positive
- ✅ **ATtiny412 works perfectly** with current design
- ✅ **No level shifting required**

### If You Need True Bipolar (-5V to +5V):
1. **Best**: ADS1115 external ADC ($5) - true ±6.144V range
2. **Budget**: Voltage divider + offset ($0.15) - reduced accuracy
3. **Precision**: Instrumentation amplifier ($2.70) - high accuracy

### Microcontrollers with Better Bipolar Support:
- **None have true bipolar ADC inputs**
- All require external circuitry (level shifter or external ADC)
- ADS1115 is the most practical solution for true bipolar measurement

For your current application with 0V, 1.68V, and 3.47V signals, the ATtiny412 design is optimal and requires no modifications.
