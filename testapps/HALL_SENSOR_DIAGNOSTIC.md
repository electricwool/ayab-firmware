# Hall Sensor Diagnostic Tool

This tool helps diagnose and calibrate hall sensor thresholds for the Raspberry Pi Pico W.

## Problem

The AYAB desktop shows "please start the machine" but doesn't detect when the carriage moves past the hall sensors. This is because the RP2040's ADC characteristics differ from other platforms, requiring different threshold values.

## Solution

Use this diagnostic firmware to measure the actual voltage levels from your hall sensors, then configure appropriate thresholds.

## How to Use

### Step 1: Upload Diagnostic Firmware

1. **Temporarily replace** `src/main.cpp` with `testapps/hall_sensor_diagnostic.cpp`
2. Build and upload to Pico W:
   ```bash
   platformio run -e rpipicow -t upload
   ```
3. Or manually copy `.pio/build/rpipicow/firmware.uf2` to the Pico W

### Step 2: Monitor Sensor Values

1. Open serial monitor at 115200 baud (COM11 or your Pico W port)
2. You'll see continuous output like:
   ```
   R_N, R_S, L_N, L_S  [min: ...] [max: ...]
   2048, 2050, 2045, 2048  [min: 2045,2048,2043,2046] [max: 2050,2052,2048,2050]
   ```

### Step 3: Move Carriage and Record Values

1. **Baseline (no magnet):** Note the values when carriage is far from sensors
2. **Move carriage slowly** across the left turn mark
3. **Watch for changes** in the values as the magnet passes
4. **Record min/max values** for each sensor

### Step 4: Analyze Results

**Expected behavior:**
- **No magnet:** Values around mid-scale (2048 for 12-bit ADC)
- **North pole (K carriage):** Values increase significantly
- **South pole (L carriage):** Values decrease significantly

**Example readings:**
```
No magnet:    2048 (baseline)
North pole:   3500 (high)
South pole:   600  (low)
```

### Step 5: Calculate Thresholds

Based on your readings, calculate thresholds:

**For 10-bit ADC (Arduino Uno/Mega - 0-1023):**
- `thresholdLow`: baseline - (baseline - min) * 0.5
- `thresholdHigh`: baseline + (max - baseline) * 0.5

**For 12-bit ADC (RP2040 - 0-4095):**
- Multiply 10-bit thresholds by 4
- Or calculate directly from your readings

**Example:**
If your readings are:
- Baseline: 2048
- Min (South): 600
- Max (North): 3500

Then:
- `thresholdLow` = 2048 - (2048 - 600) * 0.5 = 1324
- `thresholdHigh` = 2048 + (3500 - 2048) * 0.5 = 2774

### Step 6: Update Configuration

The thresholds are set in the machine configuration. You'll need to modify the values in the firmware or add a configuration option.

**Current default thresholds (for 10-bit ADC):**
- `thresholdLow`: ~300
- `thresholdHigh`: ~700

**For RP2040 (12-bit ADC), multiply by 4:**
- `thresholdLow`: ~1200
- `thresholdHigh`: ~2800

## Troubleshooting

### No value changes when moving carriage
- Check hall sensor wiring
- Verify pins in pin_definitions.json match your hardware
- Check if sensors are powered

### Values are always 0 or 4095
- ADC pin might not be configured correctly
- Check if pin supports analog input on RP2040

### Values change but detection still doesn't work
- Thresholds might need more adjustment
- Check encoder is working (carriage position tracking)
- Verify belt phase detection

## RP2040 ADC Notes

- **Resolution:** 12-bit (0-4095)
- **Reference voltage:** 3.3V
- **ADC pins:** GP26-GP29 (ADC0-ADC3)
- **Your sensor pins:** GP12-GP15 (check if these support ADC!)

**IMPORTANT:** GP12-GP15 are **NOT** ADC pins on RP2040! 
- ADC pins are only GP26, GP27, GP28, GP29
- You may need to rewire sensors to ADC-capable pins
- Or use digital mode if sensors have digital outputs

## Next Steps

After determining correct values, update the machine configuration in the firmware to use RP2040-specific thresholds.
