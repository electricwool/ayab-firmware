/**
 * @file i2cdma.h
 * @brief I2C DMA Cache with Circular Buffer and Write FIFO
 * 
 * This header provides a DMA-accelerated I2C interface that extends
 * ayabfirmware's I2cInterface with:
 * - Circular buffer DMA for cached reads (ESP32 ULP RTC / RP2040 ring buffer)
 * - FIFO queue for asynchronous writes with priority support
 * - Transparent integration - same API as standard I2cInterface
 * 
 * Architecture Support:
 * - ESP32: ULP coprocessor with RTC Slow Memory circular buffers
 * - RP2040: Hardware DMA channels with ring buffer mode  
 * - AVR: Software fallback (no hardware DMA)
 * - Renesas: Software fallback (no hardware DMA)
 * 
 * Usage in ayabfirmware:
 * Replace `i2c` with `i2cDma` in platform-specific implementations.
 * No changes needed to application code - same I2cInterface API.
 * 
 * @version 2.0.0
 * @license MIT
 */

#ifndef I2CDMA_H
#define I2CDMA_H

#include <stdint.h>
#include <stddef.h>

// ============================================================================
// Configuration Constants
// ============================================================================

namespace I2cDmaConfig {
    // Circular buffer for reads (DMA-managed)
    static constexpr uint8_t CIRCULAR_BUFFER_SIZE = 8;     ///< 8 samples per device
    static constexpr uint8_t MAX_CACHED_DEVICES = 8;       ///< Max devices with read cache
    
    // Write FIFO queue
    static constexpr uint8_t WRITE_FIFO_SIZE = 16;         ///< Max queued writes
    
    // Refresh timing
    static constexpr uint32_t DEFAULT_REFRESH_MS = 10;     ///< 10ms refresh interval
    
    // Priority levels for writes
    enum WritePriority {
        PRIORITY_LOW = 0,
        PRIORITY_NORMAL = 1,
        PRIORITY_HIGH = 2
    };
}

// ============================================================================
// Data Structures
// ============================================================================

/**
 * @brief Circular buffer entry for cached reads
 */
struct CircularBufferEntry {
    uint8_t deviceAddr;      ///< I2C device address
    uint8_t regAddr;         ///< Register address to cache
    uint16_t samples[I2cDmaConfig::CIRCULAR_BUFFER_SIZE]; ///< Ring buffer
    uint8_t writePos;        ///< Current DMA write position
    uint8_t sampleCount;     ///< Number of samples to average (1-8)
    bool is16Bit;            ///< 16-bit register
    bool active;             ///< Entry is active
};

/**
 * @brief Write FIFO entry for queued writes
 */
struct WriteFifoEntry {
    uint8_t deviceAddr;      ///< I2C device address
    uint8_t regAddr;         ///< Register address
    uint16_t value;          ///< Value to write
    bool is16Bit;            ///< 16-bit write
    uint8_t priority;        ///< Write priority level
    bool active;             ///< Entry is active
};

// ============================================================================
// Common Functions (implemented per-architecture)
// ============================================================================

/**
 * @brief Initialize I2C DMA system
 * @param sdaPin SDA pin number
 * @param sclPin SCL pin number
 * @param frequency I2C clock frequency (Hz)
 * @return true on success
 */
bool i2cDmaInit(uint8_t sdaPin, uint8_t sclPin, uint32_t frequency);

/**
 * @brief Register a device register for circular buffer caching
 * @param deviceAddr I2C device address
 * @param regAddr Register address
 * @param is16Bit True for 16-bit register
 * @param sampleCount Number of samples to average (1-8)
 * @return true on success
 */
bool i2cDmaRegisterCache(uint8_t deviceAddr, uint8_t regAddr, bool is16Bit, uint8_t sampleCount);

/**
 * @brief Read from circular buffer cache (non-blocking)
 * @param deviceAddr I2C device address
 * @param regAddr Register address
 * @param is16Bit True for 16-bit read
 * @return Cached value (averaged if multisampled)
 */
uint16_t i2cDmaReadCached(uint8_t deviceAddr, uint8_t regAddr, bool is16Bit);

/**
 * @brief Queue a write to FIFO (non-blocking)
 * @param deviceAddr I2C device address
 * @param regAddr Register address (0 for device-only writes)
 * @param value Value to write
 * @param is16Bit True for 16-bit write
 * @param priority Write priority level
 * @return true if queued successfully
 */
bool i2cDmaQueueWrite(uint8_t deviceAddr, uint8_t regAddr, uint16_t value, 
                      bool is16Bit, uint8_t priority);

/**
 * @brief Start background DMA refresh task
 * @param intervalMs Refresh interval in milliseconds
 * @return true on success
 */
bool i2cDmaStartRefresh(uint32_t intervalMs);

/**
 * @brief Stop background DMA refresh task
 */
void i2cDmaStopRefresh();

/**
 * @brief Check if hardware DMA is available
 * @return true if ESP32/RP2040 with DMA active
 */
bool i2cDmaHasHardware();

/**
 * @brief Direct I2C read (fallback, blocking)
 * @param deviceAddr I2C device address
 * @param regAddr Register address
 * @param is16Bit True for 16-bit read
 * @return Read value
 */
uint16_t i2cDmaDirectRead(uint8_t deviceAddr, uint8_t regAddr, bool is16Bit);

/**
 * @brief Direct I2C write (fallback, blocking)
 * @param deviceAddr I2C device address
 * @param regAddr Register address (0 for device-only writes)
 * @param value Value to write
 * @param is16Bit True for 16-bit write
 */
void i2cDmaDirectWrite(uint8_t deviceAddr, uint8_t regAddr, uint16_t value, bool is16Bit);

/**
 * @brief Device detection
 * @param deviceAddr I2C device address
 * @return true if device responds
 */
bool i2cDmaDetect(uint8_t deviceAddr);

// ============================================================================
// Preprocessor Wrapper Macros
// ============================================================================

/**
 * @brief Preprocessor macros to wrap I2C operations with DMA acceleration
 *
 * These macros allow transparent integration by wrapping standard I2C calls
 * to use the i2cDma functions. Code can use these macros for automatic
 * DMA acceleration where available (ESP32/RP2040) with fallback on other platforms.
 */

// Initialize I2C with DMA support
#define I2C_INIT(sda, scl, freq) i2cDmaInit(sda, scl, freq)

// Register a device register for DMA caching
#define I2C_REGISTER_CACHE(dev, reg, is16bit, samples) \
    i2cDmaRegisterCache(dev, reg, is16bit, samples)

// Read operations (use cached DMA reads when available)
#define I2C_READ(dev, reg) \
    ((uint8_t)i2cDmaReadCached(dev, reg, false))

#define I2C_READ16(dev, reg) \
    i2cDmaReadCached(dev, reg, true)

// Write operations (queue to DMA FIFO when available)
#define I2C_WRITE(dev, reg, val) \
    i2cDmaQueueWrite(dev, reg, val, false, I2cDmaConfig::PRIORITY_NORMAL)

#define I2C_WRITE16(dev, reg, val) \
    i2cDmaQueueWrite(dev, reg, val, true, I2cDmaConfig::PRIORITY_NORMAL)

#define I2C_WRITE_PRIORITY(dev, reg, val, prio) \
    i2cDmaQueueWrite(dev, reg, val, false, prio)

#define I2C_WRITE16_PRIORITY(dev, reg, val, prio) \
    i2cDmaQueueWrite(dev, reg, val, true, prio)

// Device-only write (no register address)
#define I2C_WRITE_DEVICE(dev, val) \
    i2cDmaQueueWrite(dev, 0, val, false, I2cDmaConfig::PRIORITY_NORMAL)

// Direct (blocking) operations - bypass DMA cache/FIFO
#define I2C_READ_DIRECT(dev, reg) \
    ((uint8_t)i2cDmaDirectRead(dev, reg, false))

#define I2C_READ16_DIRECT(dev, reg) \
    i2cDmaDirectRead(dev, reg, true)

#define I2C_WRITE_DIRECT(dev, reg, val) \
    i2cDmaDirectWrite(dev, reg, val, false)

#define I2C_WRITE16_DIRECT(dev, reg, val) \
    i2cDmaDirectWrite(dev, reg, val, true)

// Device detection
#define I2C_DETECT(dev) i2cDmaDetect(dev)

// DMA control
#define I2C_DMA_START(intervalMs) i2cDmaStartRefresh(intervalMs)
#define I2C_DMA_STOP() i2cDmaStopRefresh()
#define I2C_DMA_HAS_HARDWARE() i2cDmaHasHardware()

#endif // I2CDMA_H
