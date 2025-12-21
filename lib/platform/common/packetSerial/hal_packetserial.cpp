#include "hal_packetserial.h"
#include "debug.h"

namespace hardwareAbstraction {

    PacketSerial::PacketSerial() {
        DEBUG_INIT();
        DEBUG_PRINTLN("PacketSerial: Initializing...");
        
        ::Serial.begin(115200);
        DEBUG_PRINTLN("PacketSerial: Serial.begin(115200) called");
        
        myPacketSerial.setStream(&::Serial);
        DEBUG_PRINTLN("PacketSerial: setStream() called");
    }

    void PacketSerial::send(const uint8_t *buffer, size_t size) {
        DEBUG_PRINT("PacketSerial: Sending ");
        DEBUG_PRINT(size);
        DEBUG_PRINT(" bytes, first byte: 0x");
        DEBUG_PRINTLN_HEX(buffer[0]);
        myPacketSerial.send(buffer, size);
    }

    void PacketSerial::setPacketHandler(void (*callback)(const uint8_t* buffer, size_t size)) {
        myPacketSerial.setPacketHandler(callback);
        DEBUG_PRINTLN("PacketSerial: Packet handler set");
    }

    void PacketSerial::schedule() {
        myPacketSerial.update();
        
        #ifdef ARDUINO_ARCH_RP2040
        static unsigned long lastDebug = 0;
        static int scheduleCount = 0;
        
        scheduleCount++;
        
        // Debug output every 5 seconds
        if (millis() - lastDebug > 5000) {
            DEBUG_PRINT_INT("PacketSerial: schedule() called times: ", scheduleCount);
            scheduleCount = 0;
            lastDebug = millis();
        }
        #endif
    }
}