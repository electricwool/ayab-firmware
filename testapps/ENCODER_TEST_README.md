# Encoder Level Shifter Test Firmware

## Overview

Test firmware for validating the TXS0104E level shifter circuit with KH-930 encoder on RP2040.

**File:** [`encoder_level_shifter_test.cpp`](encoder_level_shifter_test.cpp)

---

## Hardware Requirements

### Components
- **RP2040 Pico W** (or standard Pico)
- **TXS0104E** 4-channel bidirectional level shifter
- **KH-930 Encoder** (5V logic output)
- **Supporting components** (see circuit diagram)

### Circuit
See [`ENCODER_LEVEL_SHIFTER_CIRCUIT.md`](ENCODER_LEVEL_SHIFTER_CIRCUIT.md) for complete schematic.

### Connections

| KH-930 Encoder | TXS0104E | RP2040 Pico |
|----------------|----------|-------------|
| Pin 1 (VCC) | VCCA (pin 1) | - |
| Pin 2 (ENC_A) | A1 (pin 2) | GP10 (via B1) |
| Pin 3 (ENC_B) | A2 (pin 3) | GP11 (via B2) |
| Pin 4 (ENC_C) | A3 (pin 4) | GP12 (via B3) |
| Pin 5 (GND) | GND (pin 7) | GND |
| - | VCCB (pin 14) | 3.3V |
| - | OE (pin 8) | 3.3V |

---

## Software Requirements

### PlatformIO Configuration

Add to `platformio.ini`:

```ini
[env:pico_encoder_test]
platform = raspberrypi
board = pico
framework = arduino
build_src_filter = 
    +<../testapps/encoder_level_shifter_test.cpp>
monitor_speed = 115200
```

### Dependencies
- **Arduino-Pico** framework (included with platform)
- **Pico SDK** (included with platform)

---

## Building and Uploading

### Using PlatformIO

```bash
# Build
pio run -e pico_encoder_test

# Upload (hold BOOTSEL button, then release)
pio run -e pico_encoder_test -t upload

# Monitor serial output
pio device monitor -b 115200
```

### Using Arduino IDE

1. Install **Arduino-Pico** board support
2. Select **Raspberry Pi Pico** or **Pico W**
3. Open `encoder_level_shifter_test.cpp`
4. Upload to board

---

## Features

### 1. Quadrature Encoder Decoding
- **Hardware interrupts** on both encoder channels
- **Direction detection** (left/right)
- **Position tracking** (32-bit counter)
- **State machine** with lookup table

### 2. Speed Measurement
- **RPM calculation** (based on 16 PPR encoder)
- **Linear speed** (mm/s, assuming 0.1mm per count)
- **Pulse period** measurement (microseconds)

### 3. Belt Phase Monitoring
- **Digital state** (HIGH/LOW)
- **Change detection** with interrupt
- **Transition counting**

### 4. Statistics
- **Total interrupts** (all GPIO events)
- **Invalid transitions** (error detection)
- **Error rate** calculation
- **Changes per second**

### 5. Real-time Reporting
- **1-second intervals** (configurable)
- **Serial output** at 115200 baud
- **LED indicator** (blinks on activity)

---

## Expected Output

### Startup

```
========================================
  KH-930 Encoder Level Shifter Test
========================================
Hardware: RP2040 + TXS0104E
Encoder: 5V → 3.3V level shifted
========================================

Initializing hardware...

Initial State:
  Encoder A (GP10): LOW
  Encoder B (GP11): LOW
  Belt Phase (GP12): LOW
  State: 00 (B=LOW, A=LOW)

Ready! Rotate encoder to test...
```

### During Operation (Rotating Right)

```
----------------------------------------
Time: 5 seconds

Encoder Status:
  Position: 150 (Δ30)
  Direction: RIGHT →
  State: 10 (B=HIGH, A=LOW)
  Changes: 150 (30/sec)

Speed:
  RPM: 28.13
  mm/s: 3.00
  Pulse period: 33333 µs

Belt Phase:
  State: LOW (Regular)
  Changes: 0 (0/sec)

Statistics:
  Total interrupts: 150 (30/sec)
  Invalid transitions: 0 (0/sec)
----------------------------------------
```

### During Operation (Rotating Left)

```
----------------------------------------
Time: 10 seconds

Encoder Status:
  Position: 100 (Δ-50)
  Direction: LEFT  ←
  State: 01 (B=LOW, A=HIGH)
  Changes: 200 (50/sec)

Speed:
  RPM: 46.88
  mm/s: 5.00
  Pulse period: 20000 µs

Belt Phase:
  State: HIGH (Shifted)
  Changes: 1 (0/sec)

Statistics:
  Total interrupts: 201 (51/sec)
  Invalid transitions: 0 (0/sec)
----------------------------------------
```

---

## Testing Procedure

### 1. Power-On Test

**Expected:**
- Serial output appears
- Initial state shows all pins LOW or HIGH
- LED blinks 3 times (ready indicator)

**If fails:**
- Check USB connection
- Verify serial baud rate (115200)
- Check power supply (3.3V and 5V)

### 2. Static Signal Test

**Procedure:**
1. Don't rotate encoder
2. Observe initial state
3. Manually apply 5V to encoder pins

**Expected:**
- State changes reflected in serial output
- No position changes (no rotation)
- No invalid transitions

**If fails:**
- Check TXS0104E connections
- Verify VCCA = 5V, VCCB = 3.3V
- Check OE pin = 3.3V (enabled)

### 3. Slow Rotation Test

**Procedure:**
1. Rotate encoder slowly (< 1 rev/sec)
2. Observe position changes
3. Check direction matches rotation

**Expected:**
- Position increments when rotating right
- Position decrements when rotating left
- Direction string matches rotation
- No invalid transitions

**If fails:**
- Check encoder wiring (A and B may be swapped)
- Verify quadrature pattern with oscilloscope
- Check for noise (add filter caps if needed)

### 4. Fast Rotation Test

**Procedure:**
1. Rotate encoder quickly (> 5 rev/sec)
2. Observe speed measurements
3. Check for missed pulses

**Expected:**
- Speed (RPM) increases proportionally
- No invalid transitions
- All pulses captured (changes = 4 × pulses)

**If fails:**
- Check interrupt priority
- Verify TXS0104E speed (should be > 110 Mbps)
- Look for electrical noise

### 5. Belt Phase Test

**Procedure:**
1. Toggle belt phase input (GP12)
2. Observe state changes

**Expected:**
- State changes from LOW to HIGH
- Change counter increments
- No effect on encoder position

**If fails:**
- Check belt phase wiring
- Verify TXS0104E channel 3 (A3/B3)

### 6. Long-Term Stability Test

**Procedure:**
1. Run for 10+ minutes
2. Rotate encoder periodically
3. Monitor error rate

**Expected:**
- Zero invalid transitions
- Consistent speed measurements
- No position drift when stopped

**If fails:**
- Check for loose connections
- Verify power supply stability
- Add decoupling capacitors

---

## Troubleshooting

### No Serial Output

**Symptoms:**
- Blank serial monitor
- No startup message

**Solutions:**
1. Check USB cable connection
2. Verify serial baud rate (115200)
3. Try different serial terminal
4. Press RESET button on Pico
5. Re-upload firmware

### Wrong Direction

**Symptoms:**
- Position increases when rotating left
- Direction string opposite of rotation

**Solutions:**
1. Swap encoder A and B connections
2. Or modify code: swap `ENCODER_A_PIN` and `ENCODER_B_PIN`

### Missed Pulses

**Symptoms:**
- Position doesn't match rotation
- Invalid transitions > 0
- Erratic speed readings

**Solutions:**
1. Check encoder power supply (stable 5V)
2. Add 100nF filter caps on encoder inputs
3. Verify TXS0104E connections
4. Check for electrical noise sources
5. Reduce rotation speed

### No Position Changes

**Symptoms:**
- Position stays at 0
- No changes reported
- Interrupts = 0

**Solutions:**
1. Check TXS0104E power (VCCA = 5V, VCCB = 3.3V)
2. Verify OE pin = 3.3V (enabled)
3. Check encoder power supply
4. Verify GPIO pin assignments
5. Test with multimeter (should see voltage changes)

### High Error Rate

**Symptoms:**
- Invalid transitions > 0
- Error rate > 1%

**Solutions:**
1. Add 100nF filter capacitors on inputs
2. Check for ground loops
3. Verify encoder quality (mechanical bounce)
4. Reduce rotation speed
5. Add Schmitt trigger (74HC14) if needed

### Belt Phase Not Changing

**Symptoms:**
- Belt phase stuck at LOW or HIGH
- Changes = 0

**Solutions:**
1. Check belt phase input signal
2. Verify TXS0104E channel 3 (A3/B3)
3. Test with manual 5V input
4. Check GP12 configuration

---

## Performance Metrics

### Expected Performance

| Metric | Value | Notes |
|--------|-------|-------|
| **Max encoder speed** | > 10,000 RPM | Limited by mechanical encoder |
| **Interrupt latency** | < 10 µs | RP2040 GPIO interrupt |
| **Position accuracy** | ±1 count | No missed pulses |
| **Invalid transitions** | 0 | With good encoder |
| **CPU usage** | < 5% | At 1000 RPM |

### KH-930 Typical Values

| Parameter | Value | Notes |
|-----------|-------|-------|
| **Encoder PPR** | 16 | Pulses per revolution |
| **Quadrature edges** | 64 | 4× multiplier |
| **Max carriage speed** | ~1 m/s | Typical knitting speed |
| **Max encoder frequency** | ~1 kHz | At 1 m/s |
| **Belt phase frequency** | ~1 Hz | Once per belt revolution |

---

## Code Structure

### Main Components

```cpp
// Configuration
#define ENCODER_A_PIN     10
#define ENCODER_B_PIN     11
#define BELT_PHASE_PIN    12

// Global state (volatile for ISR access)
volatile int32_t g_encoder_position
volatile int8_t g_encoder_direction
volatile bool g_belt_phase

// Quadrature lookup table
const int8_t QUADRATURE_TABLE[4][4]

// Interrupt handlers
void encoder_irq_handler()
void belt_phase_irq_handler()

// Utility functions
float calculate_speed_rpm()
float calculate_speed_mm_s()
const char* get_direction_string()

// Main functions
void setup()
void loop()
```

### Customization

**Change GPIO pins:**
```cpp
#define ENCODER_A_PIN     10  // Change to your pin
#define ENCODER_B_PIN     11  // Change to your pin
#define BELT_PHASE_PIN    12  // Change to your pin
```

**Change report interval:**
```cpp
#define REPORT_INTERVAL_MS  1000  // Change to desired ms
```

**Change encoder resolution:**
```cpp
// In calculate_speed_rpm()
const float EDGES_PER_REV = 64.0f;  // Change for your encoder
```

**Change position scaling:**
```cpp
// In calculate_speed_mm_s()
const float MM_PER_COUNT = 0.1f;  // Change for your machine
```

---

## Integration with Main Firmware

### Using This Code in Production

1. **Copy interrupt handlers** to your main code
2. **Initialize GPIO** in your setup function
3. **Read position** from `g_encoder_position`
4. **Read direction** from `g_encoder_direction`
5. **Read belt phase** from `g_belt_phase`

### Example Integration

```cpp
// In your main firmware
extern volatile int32_t g_encoder_position;
extern volatile int8_t g_encoder_direction;
extern volatile bool g_belt_phase;

void your_knitting_function() {
  // Get current encoder position
  int32_t position = g_encoder_position;
  
  // Get current direction
  if (g_encoder_direction > 0) {
    // Moving right
  } else if (g_encoder_direction < 0) {
    // Moving left
  }
  
  // Get belt phase
  if (g_belt_phase) {
    // Belt in shifted position
  } else {
    // Belt in regular position
  }
}
```

---

## Next Steps

After successful testing:

1. ✅ **Verify level shifting** - All signals clean and correct voltage
2. ✅ **Validate quadrature decoding** - Direction and position accurate
3. ✅ **Confirm speed measurements** - RPM calculations correct
4. ✅ **Test belt phase** - State changes detected
5. ✅ **Check long-term stability** - No errors over time

Then proceed to:
- Integrate with hall sensor detection
- Add solenoid control
- Implement full knitting firmware

---

## References

- [`ENCODER_LEVEL_SHIFTER_CIRCUIT.md`](ENCODER_LEVEL_SHIFTER_CIRCUIT.md) - Complete circuit schematic
- [`VOLTAGE_SHIFTERS_5V_TO_3V3.md`](VOLTAGE_SHIFTERS_5V_TO_3V3.md) - Level shifter comparison
- [`ENCODER_BITBANG_VS_LS7366R.md`](ENCODER_BITBANG_VS_LS7366R.md) - Encoder interface options
- [`OPTIMIZED_LOW_LATENCY_ENCODER.md`](OPTIMIZED_LOW_LATENCY_ENCODER.md) - Encoder signal details

---

## License

Same as main AYAB firmware project.
