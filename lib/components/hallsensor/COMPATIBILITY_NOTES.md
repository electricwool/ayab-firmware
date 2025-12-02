# Hardware Compatibility Notes

## Platform Support Status

Both `dualhallsensor.h` and `hallsensor.cpp` are designed to be hardware agnostic and work across all target platforms.

### Supported Platforms
- ✅ **ATmega328P** (Arduino Uno) - `atmelavr` platform
- ✅ **ATmega2560** (Arduino Mega) - `atmelavr` platform  
- ✅ **Renesas RA4M1** (Arduino UNO R4 WiFi) - ARM Cortex-M4
- ✅ **ESP32-S3** (DevKitC-1, DevKitM-1) - Xtensa LX7
- ✅ **RP2040** (Raspberry Pi Pico W) - ARM Cortex-M0+

## Design Principles for Hardware Agnosticism

### 1. Hardware Abstraction Layer (HAL)
All hardware access goes through `hardwareAbstraction::HalInterface`:
```cpp
_hal->pinMode(pin, mode);
_hal->digitalWrite(pin, state);
_hal->digitalRead(pin);
_hal->analogRead(pin);
_hal->delayMicroseconds(us);
_hal->attachInterrupt(pin, handler, mode);
```

### 2. Hardware-Agnostic Atomic Operations
When implementing ISR-based code (like `dualhallsensor.cpp`), use the common atomic header:

```cpp
// Hardware-agnostic atomic operations
#include "atomic.h"

// Use in your code:
void schedule() {
  if (_isr_doorbell) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      _isr_doorbell = false;
      _position = _isr_position;
    }
  }
}
```

The `lib/platform/common/atomic.h` header automatically redirects to the correct
platform-specific implementation (AVR util/atomic.h, Renesas custom, ESP32 FreeRTOS,
or RP2040 Arduino-Pico).

### 3. Standard Integer Types
Use fixed-width integer types from `<stdint.h>`:
- `uint8_t`, `int16_t`, `uint16_t`, `uint32_t`
- Never use `int`, `long`, `unsigned int` (size varies by platform)

### 4. Volatile Variables for ISR Communication
Always mark variables shared between ISR and main loop as `volatile`:
```cpp
volatile bool _isrDoorbell;
volatile Pole _leftPole;
volatile Pole _rightPole;
```

### 5. Timing Considerations
Different platforms have different timing characteristics:
- **AVR (16 MHz)**: Simple instructions, predictable timing
- **Renesas RA4M1 (48 MHz)**: ARM Cortex-M4, faster execution
- **ESP32-S3 (240 MHz)**: Dual core, much faster, FreeRTOS overhead
- **RP2040 (133 MHz)**: Dual core ARM Cortex-M0+

**Use HAL abstraction for delays:**
```cpp
_hal->delayMicroseconds(100); // Hardware agnostic
// NOT: delayMicroseconds(100); // Bypasses HAL
```

## Implementation Checklist for dualhallsensor.cpp

When implementing `dualhallsensor.cpp`, ensure:

- [ ] Include common atomic header: `#include "atomic.h"`
- [ ] Use `ATOMIC_BLOCK(ATOMIC_RESTORESTATE)` for ISR variable access
- [ ] All hardware access through `_hal->` methods
- [ ] Use `volatile` for all ISR-shared variables
- [ ] Use fixed-width integer types
- [ ] Test on all target platforms
- [ ] Verify interrupt attachment works on each platform
- [ ] Check timing-sensitive code on slow (AVR) and fast (ESP32) platforms

## Platform-Specific Notes

### AVR (ATmega328P/2560)
- Limited RAM (2KB/8KB) - keep ISR code minimal
- Single core - no multicore concerns
- `util/atomic.h` available natively

### Renesas RA4M1 (UNO R4)
- ARM Cortex-M4 - different instruction set
- Custom `atomic.h` in `lib/platform/arch/renesas-ra/util/`
- More RAM (32KB) - less constrained

### ESP32-S3
- Dual core Xtensa architecture
- FreeRTOS-based - use FreeRTOS atomic primitives
- Much more RAM (512KB) - can afford larger buffers
- ISRs run on different core - proper synchronization critical

### RP2040 (Pico W)
- Dual core ARM Cortex-M0+
- Arduino-Pico core provides `util/atomic.h`
- More RAM (264KB) than AVR
- Fast execution - may need longer debounce times

## Known Compatibility Issues

### None Currently
Both header files follow best practices for hardware abstraction. When the implementation (`.cpp`) files are created, they must follow the patterns established in `encoder.cpp` for cross-platform compatibility.

## Testing Matrix

| Platform | hallsensor.cpp | dualhallsensor.h | kh970_sensors.h |
|----------|---------------|------------------|-----------------|
| ATmega328P (Uno) | ✅ Working | 📝 Header only | 📝 Header only |
| ATmega2560 (Mega) | ✅ Working | 📝 Header only | 📝 Header only |
| Renesas RA4M1 (R4) | ✅ Working | 📝 Header only | 📝 Header only |
| ESP32-S3 (DevKitC) | ✅ Working | 📝 Header only | 📝 Header only |
| ESP32-S3 (Mini) | ✅ Working | 📝 Header only | 📝 Header only |
| RP2040 (Pico W) | ⚠️ Untested | 📝 Header only | 📝 Header only |

Legend:
- ✅ Working - Tested and confirmed
- 📝 Header only - Implementation not yet created
- ⚠️ Untested - Should work but needs testing
- ❌ Broken - Known issues
