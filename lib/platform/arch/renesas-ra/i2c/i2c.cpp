#include <Wire.h>
#include "i2c.h"
#include "pin_definitions.h"
#include "../../../common/i2cdma/i2cdma.h"

namespace hardwareAbstraction {

I2c::I2c() {
  // Initialize i2cdma with DTC (Data Transfer Controller) support
#if defined(I2C_PIN_SDA) && defined(I2C_PIN_SCL)
  i2cDmaInit(I2C_PIN_SDA, I2C_PIN_SCL, 400000);
#else
  i2cDmaInit(SDA, SCL, 400000);  // Default Renesas I2C pins
#endif
  
  // Register frequently-read devices for circular buffer caching
  // Example: i2cDmaRegisterCache(0x48, 0x00, true, 4);  // 4x averaging
  
  // Start background refresh
  i2cDmaStartRefresh(10);  // 10ms interval
}

bool I2c::detect(uint8_t device) {
  return i2cDmaDetect(device);
}

uint8_t I2c::read(uint8_t device, uint8_t address) {
  // Use cached read if available, otherwise direct read
  if (i2cDmaHasHardware()) {
    return static_cast<uint8_t>(i2cDmaReadCached(device, address, false));
  }
  return static_cast<uint8_t>(i2cDmaDirectRead(device, address, false));
}

void I2c::write(uint8_t device, uint8_t value) {
  i2cDmaDirectWrite(device, 0, value, false);
}

void I2c::write(uint8_t device, uint8_t address, uint8_t value) {
  // Queue write to FIFO (non-blocking)
  i2cDmaQueueWrite(device, address, value, false, I2cDmaConfig::PRIORITY_NORMAL);
}

uint16_t I2c::read16(uint8_t device, uint8_t address) {
  // Use cached read if available, otherwise direct read
  if (i2cDmaHasHardware()) {
    return i2cDmaReadCached(device, address, true);
  }
  return i2cDmaDirectRead(device, address, true);
}

void I2c::write16(uint8_t device, uint8_t address, uint16_t value) {
  // Queue write to FIFO (non-blocking)
  i2cDmaQueueWrite(device, address, value, true, I2cDmaConfig::PRIORITY_NORMAL);
}
}  // namespace hardwareAbstraction