#ifndef SHIELD_H
#define SHIELD_H

#include <stdint.h>
#include "pin_definitions.h"

namespace Shield {

    // LEDs
    struct Leds {
        #ifdef LED_A_PIN
        static constexpr uint8_t LED_A_PIN = ::LED_A_PIN;
        #endif
        #ifdef LED_B_PIN
        static constexpr uint8_t LED_B_PIN = ::LED_B_PIN;
        #endif
        #ifdef LED_C_PIN
        static constexpr uint8_t LED_C_PIN = ::LED_C_PIN;
        #endif
    };

    // Piezo
    struct Piezo {
        static constexpr uint8_t PIEZO_PIN = ::PIEZO_PIN;
    };

    // Encoder
    struct Encoder {
        static constexpr uint8_t ENC_A_PIN = ::ENC_A_PIN;
        static constexpr uint8_t ENC_B_PIN = ::ENC_B_PIN;
        static constexpr uint8_t ENC_C_PIN = ::ENC_C_PIN;
    };

    // Hall detectors
    struct HallDetectors {
        static constexpr uint8_t EOL_R_PIN = ::EOL_R_PIN;
        static constexpr uint8_t EOL_L_PIN = ::EOL_L_PIN;
        
        #ifdef EOL_R_L_PIN
        static constexpr uint8_t EOL_R_L_PIN = ::EOL_R_L_PIN;
        #endif
        
        #ifdef EOL_R_DETECT_PIN
        static constexpr uint8_t EOL_R_DETECT_PIN = ::EOL_R_DETECT_PIN;
        #endif
        
        #ifdef EOL_PIN_R_N
        static constexpr uint8_t EOL_PIN_R_N = ::EOL_PIN_R_N;
        static constexpr uint8_t EOL_PIN_R_S = ::EOL_PIN_R_S;
        static constexpr uint8_t EOL_PIN_L_N = ::EOL_PIN_L_N;
        static constexpr uint8_t EOL_PIN_L_S = ::EOL_PIN_L_S;
        #endif
    };

    // Additional board-specific pin definitions for ESP32
    #if defined(ARDUINO_ESP32)
        #ifdef MCP23017_ADDR_0
        static constexpr uint8_t MCP23017_ADDR_0 = ::MCP23017_ADDR_0;
        #endif
        #ifdef MCP_SDA_PIN
        static constexpr uint8_t MCP_SDA_PIN = ::MCP_SDA_PIN;
        #endif
        #ifdef MCP_SCL_PIN
        static constexpr uint8_t MCP_SCL_PIN = ::MCP_SCL_PIN;
        #endif
        #ifdef I2C_PIN_SDA
        static constexpr uint8_t I2C_PIN_SDA = ::I2C_PIN_SDA;
        #endif
        #ifdef I2C_PIN_SCL
        static constexpr uint8_t I2C_PIN_SCL = ::I2C_PIN_SCL;
        #endif
        #ifdef SPI_PIN_COPI
        static constexpr uint8_t SPI_PIN_COPI = ::SPI_PIN_COPI;
        #endif
        #ifdef SPI_PIN_CIPO
        static constexpr uint8_t SPI_PIN_CIPO = ::SPI_PIN_CIPO;
        #endif
        #ifdef SPI_PIN_SCK
        static constexpr uint8_t SPI_PIN_SCK = ::SPI_PIN_SCK;
        #endif
        #ifdef SPI_PIN_CS
        static constexpr uint8_t SPI_PIN_CS = ::SPI_PIN_CS;
        #endif
        #ifdef UART_PIN_TX
        static constexpr uint8_t UART_PIN_TX = ::UART_PIN_TX;
        #endif
        #ifdef UART_PIN_RX
        static constexpr uint8_t UART_PIN_RX = ::UART_PIN_RX;
        #endif
        #ifdef USER_BUTTON
        static constexpr uint8_t USER_BUTTON = ::USER_BUTTON;
        #endif
        #ifdef USER_PIN_14
        static constexpr uint8_t USER_PIN_14 = ::USER_PIN_14;
        #endif
        #ifdef USER_PIN_17
        static constexpr uint8_t USER_PIN_17 = ::USER_PIN_17;
        #endif
        #ifdef USER_PIN_18
        static constexpr uint8_t USER_PIN_18 = ::USER_PIN_18;
        #endif
        #ifdef USER_PIN_21
        static constexpr uint8_t USER_PIN_21 = ::USER_PIN_21;
        #endif
        #ifdef USER_PIN_39
        static constexpr uint8_t USER_PIN_39 = ::USER_PIN_39;
        #endif
        #ifdef USER_PIN_40
        static constexpr uint8_t USER_PIN_40 = ::USER_PIN_40;
        #endif
        #ifdef USER_PIN_41
        static constexpr uint8_t USER_PIN_41 = ::USER_PIN_41;
        #endif
        #ifdef USER_PIN_42
        static constexpr uint8_t USER_PIN_42 = ::USER_PIN_42;
        #endif
    #endif

    // GPIO expanders
    namespace GpioExpanders {
        // I2C addresses declaration, ends with a sentinel row {0, 0}.
        extern const uint8_t I2C_ADDRESSES[][2];
    };

};
#endif // SHIELD_H
