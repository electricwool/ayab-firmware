#include "ws2812_status.h"

#if defined(ARDUINO_ESP32) && defined(USE_WS2812_STATUS_LED)

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "../../platform/common/shield/shield.h"

// WS2812 data pin selection:
// - Prefer user-specified WS2812_DATA_PIN (build flag)
// - Fallback to Shield::Leds::LED_A_PIN
#if defined(WS2812_DATA_PIN)
static constexpr uint8_t WS_PIN = WS2812_DATA_PIN;
#else
static constexpr uint8_t WS_PIN = Shield::Leds::LED_A_PIN;
#endif

static Adafruit_NeoPixel g_strip(1, WS_PIN, NEO_GRB + NEO_KHZ800);
static bool g_inited = false;
static bool g_channel_on[3] = {false, false, false}; // R,G,B channels

static void apply_color() {
  uint8_t r = g_channel_on[0] ? 255 : 0;
  uint8_t g = g_channel_on[1] ? 255 : 0;
  uint8_t b = g_channel_on[2] ? 255 : 0;
  g_strip.setPixelColor(0, g_strip.Color(r, g, b));
  g_strip.show();
}

void ws2812_status_ensure_init() {
  if (g_inited) return;
  g_strip.begin();
  g_strip.clear();
  g_strip.show();
  g_inited = true;
}

void ws2812_status_set_channel(uint8_t channelIndex, bool on) {
  if (channelIndex > 2) return;
  if (!g_inited) ws2812_status_ensure_init();
  g_channel_on[channelIndex] = on;
  apply_color();
}

#endif // ARDUINO_ESP32 && USE_WS2812_STATUS_LED
