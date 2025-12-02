# Pin Definitions JSON Reference

## Overview

This directory contains `pin_definitions.json`, which defines hardware pin mappings for different AYAB board configurations. The build system (`scripts/preBuild.py`) reads this file and generates `include/pin_definitions.h` before compilation.

## File Format

The JSON file maps environment names (from `platformio.ini`) to pin configuration objects. Each pin can be:
- A number (generates `constexpr uint8_t PIN_NAME = value;`)
- A string (generates `#define PIN_NAME value`)
- Commented with `//` (JSONC format supported)

## Pin Categories

### EOL Sensor Configuration (Required)
All boards must define hall sensor pins under the `EOL_SENSORS` object with **both** `SIMPLE` and `DUAL` configurations:

**SIMPLE Configuration:**
- `EOL_R_PIN` - Right hall sensor analog/digital input
- `EOL_L_PIN` - Left hall sensor analog/digital input  
- `EOL_R_L_PIN` - Right sensor L-carriage detect line (for hardware fix detection)
- `EOL_R_DETECT_PIN` - Hardware fix detection pin

**DUAL Configuration:**
- `EOL_PIN_R_N` - Right sensor North/K-carriage input
- `EOL_PIN_R_S` - Right sensor South/L-carriage input
- `EOL_PIN_L_N` - Left sensor North/K-carriage input
- `EOL_PIN_L_S` - Left sensor South/L-carriage input

Both configurations are flattened to the top level during build. The DUAL config automatically generates legacy `EOL_R_PIN` and `EOL_L_PIN` aliases, so all pin names are available in code regardless of which section defines them.

### Other Required Pins
- `ENC_A_PIN`, `ENC_B_PIN`, `ENC_C_PIN` - Rotary encoder inputs

### Optional Pins
Mark these with `// optional` comment and omit if not available:
- `LED_A_PIN`, `LED_B_PIN`, `LED_C_PIN` - Status LEDs
- `PIEZO_PIN` - Beeper output
- `WS2812`, `WS2812_DATA_PIN` - NeoPixel/addressable LED support
- `I2C_PIN_SDA`, `I2C_PIN_SCL` - Custom I2C pins
- `SPI_PIN_*`, `UART_PIN_*`, `USER_PIN_*` - Auxiliary pins

### AVR-Specific Pins
Arduino UNO/Mega need additional I2C configuration:
- `I2C_HARDWARE` - Set to 1 to use hardware I2C, 0 for software
- `SDA_PORT`, `SDA_PIN` - Port and bit for software I2C data
- `SCL_PORT`, `SCL_PIN` - Port and bit for software I2C clock

### ESP32-Specific Pins
**Additional ESP32 pins:**
- `MCP23017_ADDR_0` - I2C address for GPIO expander
- `MCP_SDA_PIN`, `MCP_SCL_PIN` - Dedicated MCP I2C pins

## Special Flags

### WS2812
Set `"WS2812": 1` to enable NeoPixel status LED support. This generates the `USE_WS2812_STATUS_LED` macro. Set to `0` to disable.

## EOL Sensor Pin Flattening

The build system automatically flattens the nested `EOL_SENSORS` structure:

1. All pins from `EOL_SENSORS.SIMPLE` are extracted to the top level
2. All pins from `EOL_SENSORS.DUAL` are extracted to the top level
3. If DUAL config exists and legacy names are not already defined:
   - `EOL_R_PIN` is aliased to `EOL_PIN_R_N`
   - `EOL_L_PIN` is aliased to `EOL_PIN_L_N`

This means:
- Code can use either naming convention (`EOL_R_PIN` or `EOL_PIN_R_N`)
- All pin names from both SIMPLE and DUAL sections are available
- The nested structure only exists in the JSON for organization

## Pin Usage Details

### Encoder Pins (`ENC_A_PIN`, `ENC_B_PIN`, `ENC_C_PIN`)
- `ENC_A_PIN`, `ENC_B_PIN` - Quadrature encoder phase A and B (interrupts)
- `ENC_C_PIN` - Encoder button/switch input

### Hall Sensor Pins

All boards define sensors using the nested `EOL_SENSORS` object with two sections:

**SIMPLE Section (basic hall sensor configuration):**
- `EOL_R_PIN` - Right hall sensor analog/digital input
- `EOL_L_PIN` - Left hall sensor analog/digital input
- `EOL_R_L_PIN` - Right L-carriage signal from machine (drives LOW when L-carriage detected)
- `EOL_R_DETECT_PIN` - Used to detect if hardware fix is present

**DUAL Section (advanced dual-input configuration):**
- `EOL_PIN_R_N` - Right sensor North/K-carriage input
- `EOL_PIN_R_S` - Right sensor South/L-carriage input
- `EOL_PIN_L_N` - Left sensor North/K-carriage input
- `EOL_PIN_L_S` - Left sensor South/L-carriage input

Both sections must be defined in the JSON. The build script flattens all pins to the top level, making them all available in code.

### Hardware Fix Detection (Platform Agnostic)

The `EOL_R_L_PIN` and `EOL_R_DETECT_PIN` pins enable automatic detection of the hardware fix for Lace carriage support. This feature is **platform agnostic** and works on any board where these pins are defined.

**Detection Process:**
1. At startup, `EOL_R_DETECT_PIN` is set as OUTPUT LOW
2. If the fix is present, `EOL_R_L_PIN` (wired to machine signal) reads LOW
3. If not present, `EOL_R_L_PIN` (unconnected with pullup) reads HIGH
4. After detection, `EOL_R_DETECT_PIN` is set back to INPUT to avoid interference

**Platform Behavior:**

**When `EOL_R_L_PIN` and `EOL_R_DETECT_PIN` are defined** (AVR boards with hardware fix):
- Creates right hall sensor with all 3 parameters for Lace carriage detection
- Hardware fix detection runs automatically during initialization
- Full support for detecting K, L, and G carriages

**When pins NOT defined** (ESP32, RP2040, or AVR without hardware fix):
- Creates right hall sensor with only the basic pin
- No compilation errors - gracefully degrades to basic functionality
- May have limited Lace carriage detection capability depending on sensor type

The `HallSensor` class is platform agnostic - it uses HAL abstraction for all GPIO operations and handles the hardware fix detection logic internally when the optional pins are provided. Boards without the hardware fix can still function but may have reduced carriage detection capabilities.

## Adding a New Board

1. Add a new entry in `pin_definitions.json` with the environment name from `platformio.ini`
2. Define encoder pins (`ENC_A_PIN`, `ENC_B_PIN`, `ENC_C_PIN`)
3. Define `EOL_SENSORS` with **both** `SIMPLE` and `DUAL` sections:
   - SIMPLE: Map your actual sensor pins to the 4 SIMPLE pin names
   - DUAL: Map your actual sensor pins to the 4 DUAL pin names (can reuse same physical pins)
4. Add optional pins as needed (mark with `// optional` comment)
5. For AVR boards, include I2C configuration pins (`I2C_HARDWARE`, `SDA_PORT`, etc.)
6. For ESP32 boards, include MCP and I2C configuration pins
7. Run build - the pre-build script will flatten EOL_SENSORS and generate `include/pin_definitions.h`

## Example Configurations

### AVR Board (UNO/Mega)
```json
"uno": {
  "I2C_HARDWARE": 1,
  "LED_A_PIN": 5,
  "ENC_A_PIN": 2,
  "ENC_B_PIN": 3,
  "ENC_C_PIN": 4,
  "EOL_SENSORS": {
    "SIMPLE": {
      "EOL_R_PIN": 14,
      "EOL_L_PIN": 15,
      "EOL_R_L_PIN": 7,
      "EOL_R_DETECT_PIN": 8
    },
    "DUAL": {
      "EOL_PIN_R_N": 14,
      "EOL_PIN_R_S": 15,
      "EOL_PIN_L_N": 15,
      "EOL_PIN_L_S": 14
    }
  },
  "SDA_PORT": "PORTC",
  "SDA_PIN": 4,
  "SCL_PORT": "PORTC",
  "SCL_PIN": 5
}
```

### ESP32 Board
```json
"ESP32-S3-DevKitC-1": {
  "LED_A_PIN": 33,
  "ENC_A_PIN": 5,
  "ENC_B_PIN": 6,
  "ENC_C_PIN": 7,
  "EOL_SENSORS": {
    "SIMPLE": {
      "EOL_R_PIN": 3,
      "EOL_L_PIN": 1,
      "EOL_R_L_PIN": 7,
      "EOL_R_DETECT_PIN": 8
    },
    "DUAL": {
      "EOL_PIN_R_N": 3,
      "EOL_PIN_R_S": 4,
      "EOL_PIN_L_N": 1,
      "EOL_PIN_L_S": 2
    }
  },
  "MCP_SDA_PIN": 8,
  "MCP_SCL_PIN": 9,
  "WS2812": 1,
  "WS2812_DATA_PIN": 48
}
```

Note: Both SIMPLE and DUAL sections must be present. The build script flattens both, making all pin names available.

## Build Process

1. `scripts/preBuild.py` runs before compilation
2. Reads `pin_definitions.json` and strips comments
3. Extracts pin definitions for current environment (`PIOENV`)
4. Flattens `EOL_SENSORS` structure:
   - Extracts all pins from `SIMPLE` section to top level
   - Extracts all pins from `DUAL` section to top level
   - Creates `EOL_R_PIN`/`EOL_L_PIN` aliases from DUAL if not already defined
5. Removes the nested `EOL_SENSORS` object (no longer needed)
6. Validates required pins are present
7. Generates `include/pin_definitions.h` with:
   - Header guards
   - `USE_WS2812_STATUS_LED` macro (if enabled)
   - All flattened pin constant definitions
   - Optional pin guards (`#ifdef`)

## Troubleshooting

**Build fails with "Missing required pin values":**
- Check that all required pins are defined in your environment's section
- Verify pin names match exactly (case-sensitive)
- Ensure values are not `null` or empty strings

**Pin not available in code:**
- Optional pins are wrapped in `#ifdef` - check if defined before using
- Verify environment name in `platformio.ini` matches JSON key exactly
- Check generated `include/pin_definitions.h` to see what was created

**Missing EOL sensor pins:**
- Ensure both `SIMPLE` and `DUAL` sections exist under `EOL_SENSORS`
- Each section must define all 4 pins (can reuse same physical pin numbers)
- Check that `EOL_SENSORS` is properly nested in your board's JSON
