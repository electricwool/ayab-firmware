/*!
 * \file atomic.h
 * \brief Hardware-agnostic atomic operations for AYAB firmware
 * 
 * This header provides ATOMIC_BLOCK and related macros across all supported
 * platforms (AVR, ARM Cortex-M, ESP32, RP2040). It redirects to platform-
 * specific implementations where available or provides compatible definitions.
 * 
 * Usage:
 *   ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
 *     // Critical section - interrupts disabled
 *     // State automatically restored on exit
 *   }
 * 
 * Supported Platforms:
 *   - ATmega328P/2560 (AVR) - uses <util/atomic.h>
 *   - Renesas RA4M1 (ARM Cortex-M4) - uses custom implementation
 *   - ESP32-S3 (Xtensa LX7) - uses FreeRTOS primitives
 *   - RP2040 (ARM Cortex-M0+) - uses Arduino-Pico <util/atomic.h>
 * 
 * \author AYAB Contributors
 * \copyright GPL-3.0 License
 */

#ifndef ATOMIC_H
#define ATOMIC_H

// ESP32 - Use FreeRTOS critical section macros
#ifdef ARDUINO_ESP32
  #include <Arduino.h>
  #include <freertos/FreeRTOS.h>
  
  // ESP32 critical section implementation using portMUX
  // This is compatible with FreeRTOS and ISR-safe
  static portMUX_TYPE __atomic_mux = portMUX_INITIALIZER_UNLOCKED;
  
  #define ATOMIC_BLOCK(type) \
    for (type, __ToDo = (portENTER_CRITICAL(&__atomic_mux), 1); __ToDo; \
         __ToDo = 0, portEXIT_CRITICAL(&__atomic_mux))
  
  #define ATOMIC_RESTORESTATE \
    int __atomic_restore __attribute__((__cleanup__(__atomic_cleanup))) = 0
  
  #define ATOMIC_FORCEON \
    int __atomic_forceon __attribute__((__cleanup__(__atomic_cleanup))) = 0
  
  // Cleanup function for ATOMIC_BLOCK
  static inline void __atomic_cleanup(const int *unused) {
    (void)unused;
    // Cleanup is handled by the for loop exit
  }

// Renesas RA (ARM Cortex-M4) - Use custom ARM implementation
#elif defined(ARDUINO_ARCH_RENESAS)
  #include "../arch/renesas-ra/util/atomic.h"

// RP2040 - Use custom ARM Cortex-M0+ implementation
#elif defined(ARDUINO_ARCH_RP2040)
  // Arduino-Mbed core for RP2040 does not provide util/atomic.h
  // Use our custom implementation for ARM Cortex-M0+
  #include "../arch/rp2040/util/atomic.h"

// AVR (ATmega328P, ATmega2560) - Use standard util/atomic.h
#else
  // Standard AVR atomic operations
  #include <util/atomic.h>
  
  // Ensure ATOMIC_RESTORESTATE is defined
  #ifndef ATOMIC_RESTORESTATE
    #define ATOMIC_RESTORESTATE ATOMIC_RESTORESTATE
  #endif
  
  #ifndef ATOMIC_FORCEON
    #define ATOMIC_FORCEON ATOMIC_FORCEON
  #endif
#endif

// Validate that required macros are defined
#ifndef ATOMIC_BLOCK
  #error "ATOMIC_BLOCK not defined for this platform"
#endif

#ifndef ATOMIC_RESTORESTATE
  #error "ATOMIC_RESTORESTATE not defined for this platform"
#endif

#endif // ATOMIC_H
