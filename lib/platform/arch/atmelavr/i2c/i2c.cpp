#ifdef ARDUINO_ARCH_AVR
#include "i2c.h"
#include "pin_definitions.h"

// Use generated pin definitions when available; otherwise fall back to defaults
#if !defined(SDA_PORT) || !defined(SDA_PIN) || !defined(SCL_PORT) || !defined(SCL_PIN)
  #if defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__)
    #define I2C_HARDWARE 1
    #ifndef SDA_PORT
      #define SDA_PORT PORTC
    #endif
    #ifndef SDA_PIN
      #define SDA_PIN 4  // = A4
    #endif
    #ifndef SCL_PORT
      #define SCL_PORT PORTC
    #endif
    #ifndef SCL_PIN
      #define SCL_PIN 5  // = A5
    #endif
  #elif defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
    #ifndef SDA_PORT
      #define SDA_PORT PORTF
    #endif
    #ifndef SDA_PIN
      #define SDA_PIN 4  // = A4
    #endif
    #ifndef SCL_PORT
      #define SCL_PORT PORTF
    #endif
    #ifndef SCL_PIN
      #define SCL_PIN 5  // = A5
    #endif
  #else
    #warning untested board - please check your I2C ports
  #endif
#endif

#include <SoftI2CMaster.h>

namespace hardwareAbstraction {

I2c::I2c() { ::i2c_init(); }

bool I2c::detect(uint8_t device) {
  bool is_detected = ::i2c_start((device << 1) | I2C_WRITE);
  ::i2c_stop();
  return is_detected;
}

uint8_t I2c::read(uint8_t device, uint8_t address) {
  uint8_t value;
  ::i2c_start((device << 1) | I2C_WRITE);
  ::i2c_write(address);
  ::i2c_rep_start((device << 1) | I2C_READ);
  value = ::i2c_read(true); // Block forever unless I2C_TIMEOUT is defined
  ::i2c_stop();
  return value;
}

void I2c::write(uint8_t device, uint8_t value) {
  ::i2c_start((device << 1) | I2C_WRITE);
  ::i2c_write(value);
  ::i2c_stop();
}

void I2c::write(uint8_t device, uint8_t address, uint8_t value) {
  ::i2c_start((device << 1) | I2C_WRITE);
  ::i2c_write(address);
  ::i2c_write(value);
  ::i2c_stop();
}

uint16_t I2c::read16(uint8_t device, uint8_t address) {
  uint16_t value;
  ::i2c_start((device << 1) | I2C_WRITE);
  ::i2c_write(address);
  ::i2c_rep_start((device << 1) | I2C_READ);
  uint8_t msb = ::i2c_read(false);  // Read MSB, send ACK
  uint8_t lsb = ::i2c_read(true);   // Read LSB, send NACK (last byte)
  ::i2c_stop();
  value = (static_cast<uint16_t>(msb) << 8) | lsb;
  return value;
}

void I2c::write16(uint8_t device, uint8_t address, uint16_t value) {
  ::i2c_start((device << 1) | I2C_WRITE);
  ::i2c_write(address);
  ::i2c_write(static_cast<uint8_t>(value >> 8));  // MSB first
  ::i2c_write(static_cast<uint8_t>(value & 0xFF)); // LSB second
  ::i2c_stop();
}
}  // namespace hardwareAbstraction
#endif // ARDUINO_ARCH_AVR