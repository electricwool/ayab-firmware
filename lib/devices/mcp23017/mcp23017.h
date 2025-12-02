#ifndef MCP23017_H
#define MCP23017_H

#include <Arduino.h>
#include "hal.h"
#include "../gpio_expander/gpio_expander.h"

class Mcp23017 final : public GpioExpander {
 public:
  // MCP23017 register map (IODIR, IOCON, OLAT for A/B)
  static constexpr uint8_t IODIRA = 0x00;
  static constexpr uint8_t IODIRB = 0x01;
  static constexpr uint8_t IOCON  = 0x0A;
  static constexpr uint8_t OLATA  = 0x14;
  static constexpr uint8_t OLATB  = 0x15;

  Mcp23017(hardwareAbstraction::HalInterface *hal, uint8_t i2cAddress)
      : GpioExpander(hal, i2cAddress), _olat_cache_lo(0), _olat_cache_hi(0) {}
  ~Mcp23017() = default;

  // Update low byte output latch (override required by base)
  void update(uint8_t value) override;

  // Update both OLAT registers at once (16-bit value)
  void update16(uint16_t value);

  // Write a single MCP23017 register (8-bit)
  void write(uint8_t reg, uint8_t value);

 private:
  uint8_t _olat_cache_lo;
  uint8_t _olat_cache_hi;
};

#endif
