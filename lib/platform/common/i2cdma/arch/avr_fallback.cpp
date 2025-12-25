/**
 * @file avr_fallback.cpp
 * @brief AVR Software Fallback (No Hardware DMA)
 * 
 * AVR microcontrollers (Arduino UNO, Mega, etc.) do not have I2C DMA hardware.
 * This implementation provides a software fallback using the Wire library with
 * direct blocking I2C calls.
 * 
 * Key Points:
 * - NO circular buffer DMA (no hardware support)
 * - NO write FIFO (no hardware support)
 * - All reads/writes are blocking
 * - Functions compile for compatibility but provide no DMA benefits
 * - Same API as ESP32/RP2040 for code portability
 * 
 * @version 1.0.0
 * @license MIT
 */

#if defined(ARDUINO_ARCH_AVR)

#include "../i2cdma.h"
#include <Wire.h>

// ============================================================================
// Static State (minimal for AVR)
// ============================================================================

struct AvrState {
    bool initialized;
    uint8_t sdaPin;
    uint8_t sclPin;
    uint32_t frequency;
};

static AvrState g_state = {false, 0, 0, 100000};

// ============================================================================
// Public API Implementation (Software Fallback)
// ============================================================================

bool i2cDmaInit(uint8_t sdaPin, uint8_t sclPin, uint32_t frequency) {
    if (g_state.initialized) return true;
    
    g_state.sdaPin = sdaPin;
    g_state.sclPin = sclPin;
    g_state.frequency = frequency;
    
    // Initialize Wire library
    Wire.begin();
    Wire.setClock(frequency);
    
    g_state.initialized = true;
    return true;
}

bool i2cDmaRegisterCache(uint8_t deviceAddr, uint8_t regAddr, bool is16Bit, uint8_t sampleCount) {
    // No caching on AVR - function exists for compatibility only
    return false;
}

uint16_t i2cDmaReadCached(uint8_t deviceAddr, uint8_t regAddr, bool is16Bit) {
    // No cache on AVR - use direct read
    return i2cDmaDirectRead(deviceAddr, regAddr, is16Bit);
}

bool i2cDmaQueueWrite(uint8_t deviceAddr, uint8_t regAddr, uint16_t value,
                      bool is16Bit, uint8_t priority) {
    // No FIFO on AVR - write immediately
    i2cDmaDirectWrite(deviceAddr, regAddr, value, is16Bit);
    return true;
}

bool i2cDmaStartRefresh(uint32_t intervalMs) {
    // No background refresh on AVR
    return false;
}

void i2cDmaStopRefresh() {
    // No background refresh on AVR
}

bool i2cDmaHasHardware() {
    // AVR has no I2C DMA hardware
    return false;
}

uint16_t i2cDmaDirectRead(uint8_t deviceAddr, uint8_t regAddr, bool is16Bit) {
    Wire.beginTransmission(deviceAddr);
    Wire.write(regAddr);
    Wire.endTransmission(false);
    
    Wire.requestFrom(deviceAddr, (uint8_t)(is16Bit ? 2 : 1));
    
    if (is16Bit) {
        if (Wire.available() >= 2) {
            uint8_t msb = Wire.read();
            uint8_t lsb = Wire.read();
            return (msb << 8) | lsb;
        }
    } else {
        if (Wire.available()) {
            return Wire.read();
        }
    }
    
    return 0;
}

void i2cDmaDirectWrite(uint8_t deviceAddr, uint8_t regAddr, uint16_t value, bool is16Bit) {
    Wire.beginTransmission(deviceAddr);
    
    if (regAddr > 0) {
        Wire.write(regAddr);
    }
    
    if (is16Bit) {
        Wire.write((uint8_t)(value >> 8));
        Wire.write((uint8_t)(value & 0xFF));
    } else {
        Wire.write((uint8_t)(value & 0xFF));
    }
    
    Wire.endTransmission();
}

bool i2cDmaDetect(uint8_t deviceAddr) {
    Wire.beginTransmission(deviceAddr);
    return (Wire.endTransmission() == 0);
}

#endif // ARDUINO_ARCH_AVR
