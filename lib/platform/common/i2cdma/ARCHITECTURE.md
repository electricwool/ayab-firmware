# I2CDMA Condensed Architecture

## Purpose
Provide a low-level I2C DMA library that ayabfirmware's HAL implementations can use.

## Integration Pattern

```
ayabfirmware/
├── lib/platform/arch/esp32/i2c/
│   ├── i2c.h              (HAL interface - implements I2cInterface)
│   └── i2c.cpp            (Uses i2cdma library functions)
├── lib/platform/arch/rp2040/i2c/
│   ├── i2c.h              (HAL interface - implements I2cInterface)
│   └── i2c.cpp            (Uses i2cdma library functions)
└── ...

I2CDMA/i2cdma/             (THIS LIBRARY - condensed version)
├── i2cdma.h               (Low-level function declarations)
├── arch/
│   ├── esp32_ulp.cpp      (ESP32 ULP circular buffer DMA)
│   ├── rp2040_dma.cpp     (RP2040 ring buffer DMA)
│   ├── avr_fallback.cpp   (AVR software fallback)
│   └── renesas_fallback.cpp (Renesas software fallback)
└── ARCHITECTURE.md        (This file)
```

## Usage Example

In ayabfirmware's ESP32 i2c.cpp:

```cpp
#include "i2c.h"
#include "Wire.h"
#include "i2cdma/i2cdma.h"  // Include low-level DMA library

namespace hardwareAbstraction {

    i2c::i2c() {
        // Initialize i2cdma library (ESP32 ULP circular buffer)
        i2cDmaInit(SDA_PIN
, SCL_PIN
, 400000);
        
        // Register frequently-read devices for circular buffer caching
        // (e.g., ADS1015 sensors that are read in ISRs)
        i2cDmaRegisterCache(ADS1015_ADDR, ADS1015_REG_CONVERSION, true, 4);  // 4x averaging
        
        // Start background DMA refresh
        i2cDmaStartRefresh(10);  // 10ms refresh
    }

    uint16_t i2c::read16(uint8_t device, uint8_t address) {
        // Try cached read first (instant, non-blocking)
        if (i2cDmaHasHardware()) {
            return i2cDmaReadCached(device, address, true);
        }
        
        // Fallback to direct read
        return i2cDmaDirectRead(device, address, true);
    }

    void i2c::write16(uint8_t device, uint8_t address, uint16_t value) {
        // Queue write to FIFO (non-blocking, with priority)
        i2cDmaQueueWrite(device, address, value, true, I2cDmaConfig::PRIORITY_NORMAL);
    }

} // namespace hardwareAbstraction
```

## Key Points

1. **No circular dependency**: i2cdma does NOT include hal.h
2. **Low-level library**: Provides functions, not classes/interfaces
3. **Architecture selection**: Compile-time via platformio.ini flags
4. **Transparent to app**: ayabfirmware code unchanged, only HAL implementations use it
5. **Circular buffer**: ESP32 ULP RTC memory = RP2040 DMA ring buffer
6. **Write FIFO**: Queued writes with priority support

## Architecture-Specific Features

### ESP32 (ULP Circular Buffer DMA)
- Uses RTC Slow Memory for circular buffers
- ULP coprocessor autonomously updates buffers
- FreeRTOS task coordination
- Zero CPU overhead for reads

### RP2040 (Ring Buffer DMA)
- Hardware DMA channels with ring mode
- DMA IRQ handlers
- Repeating timer for refresh
- <1% CPU overhead

### AVR/Renesas (Software Fallback)
- Direct Wire library calls
- No DMA (no hardware support)
- Functions compile but use blocking I2C
