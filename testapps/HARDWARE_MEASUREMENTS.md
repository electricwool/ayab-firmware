# Hardware Measurements - Hall Sensor Voltages

## Knitting Machine Sensor Board Readings

**Date:** 2025-12-21
**Hardware:** Raspberry Pi Pico W with knitting machine hall sensors (via interfacing circuitry)
**Measurement Tool:** Multimeter

### Voltage Readings at Sensor Board (Direct Power, No Interface)

**K-Carriage Sensor (Direct Power):**

| Condition | Voltage | Notes |
|-----------|---------|-------|
| **Magnet detected** | 3.47V | Carriage magnet directly over hall sensor |
| **No magnet (baseline)** | 1.68V | Carriage away from sensor |

**Voltage swing:** 3.47V - 1.68V = **1.79V difference** ✓ Good signal

**Lace Carriage Sensor (Direct Power):**

| Condition | Voltage | Notes |
|-----------|---------|-------|
| **Magnet detected** | 0.05V | Carriage magnet directly over hall sensor |
| **No magnet (baseline)** | 1.68V | Carriage away from sensor |

**Voltage swing:** 1.68V - 0.05V = **1.63V difference** ✓ Good signal

**Note:** These are the raw sensor board voltages when powered directly (disconnected from interface board). Both sensor types work correctly at the sensor board level.

### Voltage Readings at RP2040 Pins (After Interface Circuit)

| Sensor | Condition | Voltage | Notes |
|--------|-----------|---------|-------|
| **K carriage sensors** | No magnet | 3.47V | Baseline reading |
| **K carriage sensors** | Magnet present | 4.2V | **0.73V swing detected!** |
| **Lace carriage sensors** | All conditions | 3.47V | **No change detected!** |
| Lace left sensor | Moving carriage | 3.47V | Static voltage |
| Lace right sensor | Moving carriage | 3.47V | Static voltage |

### Critical Discovery: Interface Circuit Behavior

**Sensor Board Behavior (Direct Power):**

| Sensor Type | No Magnet | Magnet Detected | Swing | Logic |
|-------------|-----------|-----------------|-------|-------|
| K-Carriage | 1.68V (LOW) | 3.47V (HIGH) | 1.79V | Active HIGH |
| Lace Carriage | 1.68V (HIGH) | 0.05V (LOW) | 1.63V | Active LOW |

**Key Finding:** The two sensor types have **opposite polarity**!
- K-carriage: Goes HIGH when magnet detected (1.68V → 3.47V)
- Lace carriage: Goes LOW when magnet detected (1.68V → 0.05V)

**After Interface Circuit (at RP2040):**

| Sensor Type | No Magnet | Magnet Detected | Swing | Status |
|-------------|-----------|-----------------|-------|--------|
| K-Carriage | 3.47V | 4.2V | 0.73V | ✓ Working |
| Lace Carriage | 3.47V | 3.47V | 0V | ✗ Not working |

### Problem Analysis

**Root Cause Identified:**

The interface circuit appears designed for **active-HIGH sensors** (like K-carriage):
- Takes 1.68V input → outputs 3.47V
- Takes 3.47V input → outputs 4.2V
- Level shifts and amplifies the signal

For **active-LOW sensors** (like lace carriage):
- Takes 1.68V input (no magnet) → outputs 3.47V ✓
- Takes 0.05V input (magnet) → outputs ??? (should be different, but reads 3.47V)

**The interface circuit is not properly handling the lace carriage's active-LOW signal!**

**Possible Issues:**

1. **Interface Circuit Design**
   - May have diode protection clamping low voltages
   - May have minimum input threshold that 0.05V doesn't reach
   - May need different circuit configuration for active-LOW sensors

2. **Separate Interface Channels**
   - K-carriage and lace carriage may use different interface circuits
   - Lace carriage interface may be faulty or misconfigured
   - May need to check/replace lace carriage interface components

3. **Wrong Pins Being Measured**
   - Current pins: GP12-GP15 (NOT ADC capable!)
   - May be measuring wrong pins for lace carriage
   - Need to verify actual wiring

## Diagnostic Steps

### Step 1: Verify Pin Connections

Check which RP2040 pins are actually connected to sensors:
- Use continuity test from sensor board to RP2040
- Verify against pin_definitions.json
- Confirm pins are ADC-capable (GP26-29)

### Step 2: Test with Diagnostic Firmware

Upload hall_sensor_diagnostic.cpp and monitor:
- If values are constant at ~4095: Pins reading high (wrong pins or no connection)
- If values are constant at ~0: Pins reading low (grounded or wrong pins)
- If values are constant at ~3400: Reading 3.47V (current situation)
- If values change: Sensors working, just need threshold adjustment

### Step 3: Check for Digital Outputs

Some hall sensor boards have both analog and digital outputs:
- Digital output: HIGH/LOW based on threshold
- Analog output: Variable voltage based on field strength
- Check if sensor board has separate digital pins

### Step 4: Oscilloscope Measurement

Use oscilloscope to see if there are brief voltage changes:
- Set trigger to catch transitions
- Move carriage slowly
- Look for pulses or brief voltage changes

## Current Pin Configuration (from pin_definitions.json)

```json
"rpipicow": {
  "EOL_PIN_R_N": 15,  // GP15 - NOT ADC capable
  "EOL_PIN_R_S": 14,  // GP14 - NOT ADC capable  
  "EOL_PIN_L_N": 13,  // GP13 - NOT ADC capable
  "EOL_PIN_L_S": 12   // GP12 - NOT ADC capable
}
```

**Problem:** These pins cannot do analogRead() on RP2040!

## Recommended Actions

1. **Trace wiring** from sensor board to RP2040
2. **Identify actual pins** sensors are connected to
3. **Check if pins are ADC-capable** (must be GP26-29)
4. **If not ADC pins:** Rewire to GP26-29
5. **Upload diagnostic firmware** to see actual ADC readings
6. **If still no change:** Check for digital output mode or different sensor type

## RP2040 ADC Pin Reference

| GPIO | ADC Channel | Available |
|------|-------------|-----------|
| GP26 | ADC0 | ✓ |
| GP27 | ADC1 | ✓ |
| GP28 | ADC2 | ✓ |
| GP29 | ADC3 | ✓ |
| GP12-15 | None | ✗ NOT ADC |

## Next Steps

- [ ] Verify physical wiring from sensors to RP2040
- [ ] Confirm which GPIO pins are actually used
- [ ] Check if sensors have digital output option
- [ ] Upload diagnostic firmware and capture readings
- [ ] If needed, rewire to ADC-capable pins
- [ ] Test with oscilloscope for fast transitions
