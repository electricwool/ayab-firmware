#pragma once

// This patch normalizes macro collisions ONLY on ESP32.
// For other architectures (AVR, Renesas, etc.) it is inert to avoid side-effects.

#ifdef ARDUINO_ESP32
	// Load Arduino so framework headers see their expected macros first.
	#include <Arduino.h>

	// Remove framework macro versions so HAL can provide canonical values.
	#ifdef INPUT
		#undef INPUT
	#endif
	#ifdef OUTPUT
		#undef OUTPUT
	#endif
	#ifdef INPUT_PULLUP
		#undef INPUT_PULLUP
	#endif
	#ifdef CHANGE
		#undef CHANGE
	#endif

	// Re-apply HAL definitions.
#endif 
	// Non-ESP32: leave macros untouched (HAL already handles its own overrides).
	// Optionally include HAL for consistency if the file is force-included elsewhere.
	#include "hal.h"
