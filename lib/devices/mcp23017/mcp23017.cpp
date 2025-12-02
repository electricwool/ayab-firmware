#include "mcp23017.h"

void Mcp23017::update(uint8_t value) {
  // Update low byte (OLATA)
  write(OLATA, value);
}

void Mcp23017::update16(uint16_t value) {
  uint8_t lo = value & 0xFF;
  uint8_t hi = (value >> 8) & 0xFF;

  // If both bytes are unchanged and cache is valid, skip write
  if (!_cache_invalid && (lo == _olat_cache_lo) && (hi == _olat_cache_hi)) {
    return;
  }

  _olat_cache_lo = lo;
  _olat_cache_hi = hi;
  _cache_invalid = false; // TODO: set true on write failure when supported

  // Write OLATA then OLATB
  _hal->i2c->write(_i2cAddress, OLATA, lo);
  _hal->i2c->write(_i2cAddress, OLATB, hi);
}

void Mcp23017::write(uint8_t reg, uint8_t value) {
  // Keep caches in sync when writing OLAT registers directly
  if (reg == OLATA) {
    if ((value == _olat_cache_lo) && (!_cache_invalid)) {
      return;
    }
    _olat_cache_lo = value;
    _cache_invalid = false;
  } else if (reg == OLATB) {
    if ((value == _olat_cache_hi) && (!_cache_invalid)) {
      return;
    }
    _olat_cache_hi = value;
    _cache_invalid = false;
  }

  _hal->i2c->write(_i2cAddress, reg, value);
}
