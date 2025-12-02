#pragma once

#include <stdint.h>

// Enable only when building for ESP32 and explicitly requested via build flag
#if defined(ARDUINO_ESP32) && defined(USE_WS2812_STATUS_LED)

// Forward-declared API to control a single WS2812 status LED that represents
// the three logical indicator LEDs (A=R, B=G, C=B).
//
// Usage:
// - Call ws2812_status_ensure_init() before first use (idempotent).
// - Call ws2812_status_set_channel(index, on) where index: 0=R(A),1=G(B),2=B(C).
// - The module composes the color and updates the pixel.

void ws2812_status_ensure_init();
void ws2812_status_set_channel(uint8_t channelIndex, bool on);

#endif // ARDUINO_ESP32 && USE_WS2812_STATUS_LED
