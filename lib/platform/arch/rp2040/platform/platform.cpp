#ifdef ARDUINO_ARCH_RP2040
#include "platform.h"
#include "../../../common/packetSerial/hal_packetserial.h"
#include "../i2c/i2c.h"

namespace hardwareAbstraction {

    Platform::Platform() {
        packetSerial = new hardwareAbstraction::PacketSerial();
        i2c = new hardwareAbstraction::i2c();
    }

    void Platform::pinMode(uint8_t pin, uint8_t mode) {
        // Map HAL mode values to Arduino mode values
        // HAL: INPUT=0x0, OUTPUT=0x1, INPUT_PULLUP=0x2
        // Arduino Mbed: uses enum PinMode
        PinMode arduinoMode;
        switch(mode) {
            case 0x0: // INPUT
                arduinoMode = INPUT;
                break;
            case 0x1: // OUTPUT
                arduinoMode = OUTPUT;
                break;
            case 0x2: // INPUT_PULLUP
                arduinoMode = INPUT_PULLUP;
                break;
            default:
                arduinoMode = static_cast<PinMode>(mode);
                break;
        }
        ::pinMode(pin, arduinoMode);
    }

    void Platform::digitalWrite(uint8_t pin, uint8_t state) {
        // Map HAL state values to Arduino state values
        // HAL: LOW=0x0, HIGH=0x1
        ::digitalWrite(pin, state ? HIGH : LOW);
    }

    int Platform::digitalRead(uint8_t pin) {
        // Arduino returns HIGH or LOW, convert to HAL values (0x0 or 0x1)
        return (::digitalRead(pin) == HIGH) ? 0x1 : 0x0;
    }

    void Platform::analogWrite(uint8_t pin, uint8_t value) {
        ::analogWrite(pin, value);
    }

    int Platform::analogRead(uint8_t pin) {
        return ::analogRead(pin);
    }

    unsigned long Platform::millis() {
        return ::millis();
    }

    void Platform::delayMicroseconds(unsigned int us) {
        ::delayMicroseconds(us);
    }

    void Platform::attachInterrupt(uint8_t interruptNum, void (*userFunc)(), int mode) {
        // Map HAL interrupt mode to Arduino interrupt mode
        // HAL: CHANGE=1
        // Arduino Mbed: uses enum PinStatus
        PinStatus arduinoMode;
        switch(mode) {
            case 1: // CHANGE
                arduinoMode = CHANGE;
                break;
            default:
                arduinoMode = static_cast<PinStatus>(mode);
                break;
        }
        ::attachInterrupt(digitalPinToInterrupt(interruptNum), userFunc, arduinoMode);
    }
    
} // hardwareAbstraction
#endif // ARDUINO_ARCH_RP2040
