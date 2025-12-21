#ifndef DEBUG_H
#define DEBUG_H

#include <Arduino.h>

// Debug output on Serial1 (UART TX=GP0, RX=GP1) for RP2040
#ifdef ARDUINO_ARCH_RP2040
  #define DEBUG_SERIAL Serial1
  #define DEBUG_ENABLED 1
  
  // Basic debug macros
  #define DEBUG_INIT() Serial1.begin(115200)
  #define DEBUG_PRINT(x) Serial1.print(x)
  #define DEBUG_PRINTLN(x) Serial1.println(x)
  #define DEBUG_PRINT_HEX(x) Serial1.print(x, HEX)
  #define DEBUG_PRINTLN_HEX(x) Serial1.println(x, HEX)
  
  // Helper functions for formatted output
  inline void DEBUG_PRINT_INT(const char* label, int value) {
    Serial1.print(label);
    Serial1.println(value);
  }
  
  inline void DEBUG_PRINT_UINT(const char* label, unsigned int value) {
    Serial1.print(label);
    Serial1.println(value);
  }
  
  inline void DEBUG_PRINT_HEX_BYTE(const char* label, uint8_t value) {
    Serial1.print(label);
    Serial1.print("0x");
    if (value < 0x10) Serial1.print("0");
    Serial1.println(value, HEX);
  }
  
  inline void DEBUG_PRINT_BUFFER(const char* label, const uint8_t* buffer, size_t size) {
    Serial1.print(label);
    Serial1.print(" (");
    Serial1.print(size);
    Serial1.print(" bytes): ");
    for (size_t i = 0; i < size; i++) {
      if (buffer[i] < 0x10) Serial1.print("0");
      Serial1.print(buffer[i], HEX);
      if (i < size - 1) Serial1.print(" ");
    }
    Serial1.println();
  }
  
  inline void DEBUG_PRINT_BOOL(const char* label, bool value) {
    Serial1.print(label);
    Serial1.println(value ? "true" : "false");
  }
  
#else
  #define DEBUG_ENABLED 0
  #define DEBUG_INIT()
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINT_HEX(x)
  #define DEBUG_PRINTLN_HEX(x)
  #define DEBUG_PRINT_INT(label, value)
  #define DEBUG_PRINT_UINT(label, value)
  #define DEBUG_PRINT_HEX_BYTE(label, value)
  #define DEBUG_PRINT_BUFFER(label, buffer, size)
  #define DEBUG_PRINT_BOOL(label, value)
#endif

#endif // DEBUG_H
