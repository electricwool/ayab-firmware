#ifdef ARDUINO_ESP32
#include "i2c.h"
#include "Wire.h"

namespace hardwareAbstraction {

    i2c::i2c() {
        // ESP32 Wire library initialization
        // If I2C pins are provided by configuration, use them. Otherwise, use defaults.
    #if defined(I2C_PIN_SDA) && defined(I2C_PIN_SCL)
        Wire.begin(I2C_PIN_SDA, I2C_PIN_SCL);
    #else
        Wire.begin();
    #endif
    }

    bool i2c::detect(uint8_t device) {
        Wire.beginTransmission(device);
        uint8_t error = Wire.endTransmission();
        return (error == 0);
    }

    uint8_t i2c::read(uint8_t device, uint8_t address) {
        Wire.beginTransmission(device);
        Wire.write(address);
        Wire.endTransmission(false);
        
        Wire.requestFrom(static_cast<uint8_t>(device), static_cast<uint8_t>(1));
        uint8_t value = 0;
        if (Wire.available()) {
            value = Wire.read();
        }
        return value;
    }

    void i2c::write(uint8_t device, uint8_t value) {
        Wire.beginTransmission(device);
        Wire.write(value);
        Wire.endTransmission();
    }

    void i2c::write(uint8_t device, uint8_t address, uint8_t value) {
        Wire.beginTransmission(device);
        Wire.write(address);
        Wire.write(value);
        Wire.endTransmission();
    }

    uint16_t i2c::read16(uint8_t device, uint8_t address) {
        Wire.beginTransmission(device);
        Wire.write(address);
        Wire.endTransmission(false);
        
        Wire.requestFrom(static_cast<uint8_t>(device), static_cast<uint8_t>(2));
        uint16_t value = 0;
        if (Wire.available() >= 2) {
            uint8_t msb = Wire.read();
            uint8_t lsb = Wire.read();
            value = (static_cast<uint16_t>(msb) << 8) | lsb;
        }
        return value;
    }

    void i2c::write16(uint8_t device, uint8_t address, uint16_t value) {
        Wire.beginTransmission(device);
        Wire.write(address);
        Wire.write(static_cast<uint8_t>(value >> 8));  // MSB first
        Wire.write(static_cast<uint8_t>(value & 0xFF)); // LSB second
        Wire.endTransmission();
    }

} // hardwareAbstraction
#endif // ARDUINO_ESP32
