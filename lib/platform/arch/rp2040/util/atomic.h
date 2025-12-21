/*!
 * \file atomic.h
 * \brief RP2040-specific atomic operations for AYAB firmware
 *
 * This header provides ATOMIC_BLOCK implementation for RP2040 (Raspberry Pi Pico)
 * using ARM Cortex-M0+ inline assembly. This avoids conflicts with CMSIS headers.
 *
 * Usage:
 *   ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
 *     // Critical section - interrupts disabled
 *     // State automatically restored on exit
 *   }
 *
 * \author AYAB Contributors
 * \copyright GPL-3.0 License
 */

#ifndef RP2040_ATOMIC_H
#define RP2040_ATOMIC_H

#include <stdint.h>

// ARM Cortex-M0+ PRIMASK register access using inline assembly
// These are defined as statement expressions to avoid conflicts with CMSIS
#define RP2040_GET_PRIMASK() ({ \
  uint32_t __primask_val; \
  __asm__ volatile ("MRS %0, primask" : "=r" (__primask_val)); \
  __primask_val; \
})

#define RP2040_SET_PRIMASK(val) ({ \
  __asm__ volatile ("MSR primask, %0" : : "r" (val) : "memory"); \
})

#define RP2040_DISABLE_IRQ() ({ \
  __asm__ volatile ("cpsid i" : : : "memory"); \
})

#define RP2040_ENABLE_IRQ() ({ \
  __asm__ volatile ("cpsie i" : : : "memory"); \
})

// Cleanup functions for ATOMIC_BLOCK
static inline void __restore_primask_rp2040(const uint32_t *primask) {
  RP2040_SET_PRIMASK(*primask);
}

static inline void __force_enable_rp2040(const uint32_t *unused) {
  (void)unused;
  RP2040_ENABLE_IRQ();
}

// ATOMIC_BLOCK macro - compatible with AVR util/atomic.h
#define ATOMIC_BLOCK(type) \
  for (type, __ToDo = (RP2040_DISABLE_IRQ(), 1); __ToDo; __ToDo = 0)

// ATOMIC_RESTORESTATE - saves and restores interrupt state
#define ATOMIC_RESTORESTATE \
  uint32_t primask_save __attribute__((__cleanup__(__restore_primask_rp2040))) = RP2040_GET_PRIMASK()

// ATOMIC_FORCEON - forces interrupts on when exiting block
#define ATOMIC_FORCEON \
  uint32_t primask_save __attribute__((__cleanup__(__force_enable_rp2040))) = 0

#endif // RP2040_ATOMIC_H
