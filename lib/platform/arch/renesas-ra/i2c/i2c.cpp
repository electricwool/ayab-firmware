#include <Wire.h>
#include "i2c.h"
#include "pin_definitions.h"

namespace hardwareAbstraction {

I2c::I2c() {
#if defined(I2C_PIN_SDA) && defined(I2C_PIN_SCL)
  ::Wire.begin(I2C_PIN_SDA, I2C_PIN_SCL);
#else
  ::Wire.begin();
#endif
}

bool I2c::detect(uint8_t device) {
  ::Wire.beginTransmission(device);
  return ::Wire.endTransmission() == 0;
}

uint8_t I2c::read(uint8_t device, uint8_t address) {
  uint8_t value = 0;
  ::Wire.beginTransmission(device);
  ::Wire.write(address);
  if (::Wire.endTransmission(false) == 0) { // false = restart 
    if (::Wire.requestFrom(device, (uint8_t) 1) == 1) {
      value = ::Wire.read();
    }
  } // TODO: signal I2C errors to the desktop app
  return value;
}

void I2c::write(uint8_t device, uint8_t value) {
  ::Wire.beginTransmission(device);
  ::Wire.write(value);
  ::Wire.endTransmission();
}

void I2c::write(uint8_t device, uint8_t address, uint8_t value) {
  ::Wire.beginTransmission(device);
  ::Wire.write(address);
  ::Wire.write(value);
  ::Wire.endTransmission();
}

uint16_t I2c::read16(uint8_t device, uint8_t address) {
  uint16_t value = 0;
  ::Wire.beginTransmission(device);
  ::Wire.write(address);
  if (::Wire.endTransmission(false) == 0) { // false = restart
    if (::Wire.requestFrom(device, (uint8_t) 2) == 2) {
      uint8_t msb = ::Wire.read();
      uint8_t lsb = ::Wire.read();
      value = (static_cast<uint16_t>(msb) << 8) | lsb;
    }
  }
  return value;
}

void I2c::write16(uint8_t device, uint8_t address, uint16_t value) {
  ::Wire.beginTransmission(device);
  ::Wire.write(address);
  ::Wire.write(static_cast<uint8_t>(value >> 8));  // MSB first
  ::Wire.write(static_cast<uint8_t>(value & 0xFF)); // LSB second
  ::Wire.endTransmission();
}
}  // namespace hardwareAbstraction