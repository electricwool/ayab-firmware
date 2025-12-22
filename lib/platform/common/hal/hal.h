#ifndef HAL_H
#define HAL_H

#include <stddef.h>
#include <stdint.h>

// Ensure Arduino framework macros are visible first on ESP32,
// then override with HAL's canonical values without redefinition warnings.
#ifdef ARDUINO_ESP32
#include <Arduino.h>
#endif

// For Renesas UNO R4 and RP2040 (Arduino Mbed), use framework enums; avoid macro overrides
// The Arduino Mbed framework uses enums for pin modes and states, which conflict with macros
#if !defined(ARDUINO_ARCH_RENESAS) && !defined(ARDUINO_ARCH_RP2040)
// digital pin levels
#undef LOW
#undef HIGH
#define LOW 0x0
#define HIGH 0x1

// pin mode - undef first so HAL values take precedence over any framework defines
#undef INPUT
#undef OUTPUT
#undef INPUT_PULLUP
#define INPUT 0x0
#define OUTPUT 0x1
#define INPUT_PULLUP 0x2

// interrupt mode
#undef CHANGE
#define CHANGE 1
#endif // !ARDUINO_ARCH_RENESAS && !ARDUINO_ARCH_RP2040

namespace hardwareAbstraction {

    class PacketSerialInterface {
    public:
        virtual ~PacketSerialInterface() = default;

        virtual void send(const uint8_t *buffer, size_t size) = 0;
        virtual void setPacketHandler(void (*callback)(const uint8_t* buffer, size_t size)) = 0;
        virtual void schedule() = 0;
    };

    class I2cInterface {
    public:
        virtual ~I2cInterface() = default;

        virtual bool detect(uint8_t device) = 0;
        virtual uint8_t read(uint8_t device, uint8_t address) = 0;
        virtual void write(uint8_t device, uint8_t value) = 0;
        virtual void write(uint8_t device, uint8_t address, uint8_t value) = 0;
        
        // 16-bit register operations for devices like ADS1015
        virtual uint16_t read16(uint8_t device, uint8_t address) = 0;
        virtual void write16(uint8_t device, uint8_t address, uint16_t value) = 0;
    };

    class HalInterface {
    public:
        virtual ~HalInterface() = default;

        virtual void pinMode(uint8_t pin, uint8_t mode) = 0;
        virtual void digitalWrite(uint8_t pin, uint8_t state) = 0;
        virtual int digitalRead(uint8_t pin) = 0;
        virtual void analogWrite(uint8_t pin, uint8_t value) = 0;
        virtual int analogRead(uint8_t pin) = 0;
        virtual unsigned long millis() = 0;
        virtual void delayMicroseconds(unsigned int us) = 0;
        virtual void attachInterrupt(uint8_t interruptNum, void (*userFunc)(), int mode) = 0;

        I2cInterface *i2c;
        PacketSerialInterface *packetSerial;
    };

} // hardwareAbstraction

#endif // HAL_H
