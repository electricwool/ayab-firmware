#ifdef ARDUINO_ESP32
#include "platform.h"
#include "../../../common/packetSerial/hal_packetserial.h"
#include "../i2c/i2c.h"

namespace hardwareAbstraction {

    Platform::Platform() {
        packetSerial = new hardwareAbstraction::PacketSerial();
        i2c = new hardwareAbstraction::i2c();
    }

    void Platform::pinMode(uint8_t pin, uint8_t mode) {
        ::pinMode(pin, mode);
    }

    void Platform::digitalWrite(uint8_t pin, uint8_t state) {
        ::digitalWrite(pin, state);
    }

    int Platform::digitalRead(uint8_t pin) {
        return ::digitalRead(pin);
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
        ::attachInterrupt(interruptNum, userFunc, mode);
    }
    
} // hardwareAbstraction
#endif // ARDUINO_ESP32
