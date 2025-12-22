import os
import subprocess
import re
import json

Import("env")
print("Pre build script")

# Reads the current git tag of the repo and returns the version number 
# elements
# In case there are changes since the last tag, the dirty flag is set
# In case the git tag does not match the x.y.z format, 0.0.0 is used as fallback
def git_version():
    def _minimal_ext_cmd(cmd):
        # construct minimal environment
        env = {}
        for k in ['SYSTEMROOT', 'PATH']:
            v = os.environ.get(k)
            if v is not None:
                env[k] = v
        # LANGUAGE is used on win32
        env['LANGUAGE'] = 'C'
        env['LANG'] = 'C'
        env['LC_ALL'] = 'C'
        out = subprocess.Popen(cmd, stdout = subprocess.PIPE, env=env, shell=True).communicate()[0]
        return out

    fw_maj = "0"
    fw_min = "0"
    fw_patch = "0"
    fw_suffix = ""

    try:
        out = _minimal_ext_cmd("git describe --tags")
        version_string = out.strip().decode('ascii')
        print(version_string)

        # Check if string matches the version format 0.0.0-...
        regex = r'v\d{,3}[.]\d{1,3}[.]\d{1,3}'

        pattern = re.compile(regex)
        match = pattern.match(version_string)

        if match:
            fw_maj, fw_min, tail = version_string[1:].split('.')
            tail = tail.split('-', 1)
            fw_patch = tail[0]
            if len(tail) > 1:
                # Maximum length of suffix: 16 characters
                fw_suffix = tail[1][:16]
    except OSError:
        pass

    return fw_maj, fw_min, fw_patch, fw_suffix

fw_maj, fw_min, fw_patch, fw_suffix = git_version()
print("GIT Version: " + fw_maj + "." + fw_min + "." + fw_patch + "-" + fw_suffix)

with open("include/version.h", "w") as text_file:
    text_file.write("constexpr uint8_t FW_VERSION_MAJ = {0}U;\n".format(fw_maj))
    text_file.write("constexpr uint8_t FW_VERSION_MIN = {0}U;\n".format(fw_min))
    text_file.write("constexpr uint8_t FW_VERSION_PATCH = {0}U;\n".format(fw_patch))
    text_file.write("constexpr char  FW_VERSION_SUFFIX[] = \"{0}\";\n".format(fw_suffix))

# Convert voltage to ADS1015 12-bit value
def voltage_to_ads1015(voltage_str, gain_range=6.144):
    """
    Convert human-readable voltage string to ADS1015 12-bit value.
    
    Args:
        voltage_str: Voltage as string (e.g., "2.50", "-1.25")
        gain_range: Full-scale range in volts (default 6.144V for 2/3x gain)
    
    Returns:
        12-bit signed integer value for ADS1015 (left-aligned in 16-bit)
    """
    try:
        voltage = float(voltage_str)
        # ADS1015 is 12-bit: range is -2048 to +2047
        # But values are left-aligned in 16-bit registers (shifted left by 4)
        # Calculate the 12-bit value
        value_12bit = int((voltage / gain_range) * 2048.0)
        # Clamp to 12-bit signed range
        value_12bit = max(-2048, min(2047, value_12bit))
        # Left-align: shift left by 4 bits for 16-bit register
        value_16bit = value_12bit << 4
        return value_16bit
    except (ValueError, TypeError):
        print(f"Warning: Could not convert voltage '{voltage_str}' to ADS1015 value")
        return 0

# Generate pin definitions from JSON
def generate_pin_definitions():
    # Get the current build environment name
    env_name = env.get("PIOENV", "uno")
    print(f"Generating pin definitions for environment: {env_name}")
    
    # Load pin definitions JSON
    json_path = "lib/platform/common/pin_definitions.json"
    try:
        with open(json_path, "r") as f:
            # Read and strip comments (JSONC support)
            content = f.read()
            # Remove single-line comments
            lines = []
            for line in content.split('\n'):
                # Find comment position
                comment_pos = line.find('//')
                if comment_pos != -1:
                    # Keep only the part before the comment
                    line = line[:comment_pos].rstrip()
                lines.append(line)
            clean_content = '\n'.join(lines)
            pin_defs = json.loads(clean_content)
    except FileNotFoundError:
        print(f"Warning: {json_path} not found, skipping pin definitions generation")
        return
    
    # Get pin definitions for current environment
    if env_name not in pin_defs:
        print(f"Warning: No pin definitions found for environment '{env_name}'")
        return
    
    pins = pin_defs[env_name]
    
    # Flatten EOL_SENSORS nested structure
    # Supports either SIMPLE (EOL_R_PIN, EOL_L_PIN, EOL_R_L_PIN, EOL_R_DETECT_PIN)
    # or DUAL (EOL_PIN_R_N, EOL_PIN_R_S, EOL_PIN_L_N, EOL_PIN_L_S)
    # or ADS1015 (ADS1015_I2C_ADDR, ADS1015_ALERT_PIN, channel configs, voltage thresholds)
    # At least one configuration must have all pins defined
    if "EOL_SENSORS" in pins:
        eol_sensors = pins["EOL_SENSORS"]
        
        # Validate that at least one config is present
        if "SIMPLE" not in eol_sensors and "DUAL" not in eol_sensors and "ADS1015" not in eol_sensors:
            print("\n" + "="*70)
            print("ERROR: EOL_SENSORS must contain at least SIMPLE, DUAL, or ADS1015 configuration")
            print("="*70)
            print(f"Environment: {env_name}")
            print("="*70 + "\n")
            raise ValueError("EOL_SENSORS must contain at least SIMPLE, DUAL, or ADS1015 configuration")
        
        # Check which sensor config is complete
        simple_complete = False
        dual_complete = False
        ads1015_complete = False
        
        if "SIMPLE" in eol_sensors:
            simple_pins = eol_sensors["SIMPLE"]
            # SIMPLE is complete if it has all 4 pins with non-null values
            required_simple = ["EOL_R_PIN", "EOL_L_PIN", "EOL_R_L_PIN", "EOL_R_DETECT_PIN"]
            simple_complete = all(
                pin_name in simple_pins and simple_pins[pin_name] is not None and simple_pins[pin_name] != ""
                for pin_name in required_simple
            )
        
        if "DUAL" in eol_sensors:
            dual_pins = eol_sensors["DUAL"]
            # DUAL is complete if it has all 4 pins with non-null values
            required_dual = ["EOL_PIN_R_N", "EOL_PIN_R_S", "EOL_PIN_L_N", "EOL_PIN_L_S"]
            dual_complete = all(
                pin_name in dual_pins and dual_pins[pin_name] is not None and dual_pins[pin_name] != ""
                for pin_name in required_dual
            )
        
        if "ADS1015" in eol_sensors:
            ads1015_pins = eol_sensors["ADS1015"]
            # ADS1015 is complete if it has required fields
            required_ads1015 = [
                "ADS1015_I2C_ADDR", "ADS1015_ALERT_PIN",
                "ADS1015_LEFT_CHANNEL", "ADS1015_RIGHT_CHANNEL",
                "ADS1015_LEFT_THRESHOLD_LOW", "ADS1015_LEFT_THRESHOLD_HIGH",
                "ADS1015_RIGHT_THRESHOLD_LOW", "ADS1015_RIGHT_THRESHOLD_HIGH"
            ]
            ads1015_complete = all(
                pin_name in ads1015_pins and ads1015_pins[pin_name] is not None and ads1015_pins[pin_name] != ""
                for pin_name in required_ads1015
            )
        
        # At least one section must be complete
        if not simple_complete and not dual_complete and not ads1015_complete:
            print("\n" + "="*70)
            print("ERROR: At least one EOL_SENSORS section must define all required fields")
            print("="*70)
            print(f"Environment: {env_name}")
            print("SIMPLE section requires: EOL_R_PIN, EOL_L_PIN, EOL_R_L_PIN, EOL_R_DETECT_PIN")
            print("DUAL section requires: EOL_PIN_R_N, EOL_PIN_R_S, EOL_PIN_L_N, EOL_PIN_L_S")
            print("ADS1015 section requires: ADS1015_I2C_ADDR, ADS1015_ALERT_PIN, channels, thresholds")
            print("="*70 + "\n")
            raise ValueError("At least one EOL_SENSORS section must have all required fields defined")
        
        # Flatten SIMPLE pins to top level (only non-null values)
        if "SIMPLE" in eol_sensors:
            for pin_name, pin_value in eol_sensors["SIMPLE"].items():
                if pin_value is not None and pin_value != "":
                    pins[pin_name] = pin_value
        
        # Flatten DUAL pins to top level (only non-null values)
        if "DUAL" in eol_sensors:
            for pin_name, pin_value in eol_sensors["DUAL"].items():
                if pin_value is not None and pin_value != "":
                    pins[pin_name] = pin_value
            
            # Auto-generate legacy names from DUAL config for backward compatibility
            # Only if SIMPLE section didn't already define them
            if dual_complete:
                if "EOL_R_PIN" not in pins:
                    pins["EOL_R_PIN"] = eol_sensors["DUAL"]["EOL_PIN_R_N"]
                if "EOL_L_PIN" not in pins:
                    pins["EOL_L_PIN"] = eol_sensors["DUAL"]["EOL_PIN_L_N"]
        
        # Process ADS1015 configuration and convert voltage thresholds
        if "ADS1015" in eol_sensors and ads1015_complete:
            ads1015_config = eol_sensors["ADS1015"]
            
            # Copy non-threshold values directly
            for key in ["ADS1015_I2C_ADDR", "ADS1015_ALERT_PIN",
                       "ADS1015_LEFT_CHANNEL", "ADS1015_RIGHT_CHANNEL"]:
                if key in ads1015_config:
                    pins[key] = ads1015_config[key]
            
            # Convert voltage thresholds to ADS1015 binary values
            # Using 2/3x gain: ±6.144V range
            threshold_keys = [
                "ADS1015_LEFT_THRESHOLD_LOW",
                "ADS1015_LEFT_THRESHOLD_HIGH",
                "ADS1015_RIGHT_THRESHOLD_LOW",
                "ADS1015_RIGHT_THRESHOLD_HIGH"
            ]
            
            for key in threshold_keys:
                if key in ads1015_config:
                    voltage_str = ads1015_config[key]
                    binary_value = voltage_to_ads1015(voltage_str, gain_range=6.144)
                    pins[key] = binary_value
                    print(f"  Converted {key}: {voltage_str}V -> {binary_value} (0x{binary_value:04X})")
        
        # Remove the nested structure from pins dict to avoid processing it as a pin
        del pins["EOL_SENSORS"]
    
    # Generate header file
    with open("include/pin_definitions.h", "w") as f:
        f.write("// Auto-generated from lib/platform/common/pin_definitions.json\n")
        f.write("// Do not edit this file directly\n")
        f.write(f"// Environment: {env_name}\n\n")
        f.write("#ifndef PIN_DEFINITIONS_H\n")
        f.write("#define PIN_DEFINITIONS_H\n\n")
        f.write("#include <stdint.h>\n\n")
        
        # Check for WS2812 flag and generate USE_WS2812_STATUS_LED if enabled
        # Default to 0 (disabled) if not present in JSON
        ws2812_enabled = pins.get("WS2812", 0)
        if ws2812_enabled:
            f.write("#define USE_WS2812_STATUS_LED\n\n")
        
        # Generate USE_DUAL_HALL_SENSOR macro if DUAL config is complete
        # Generate USE_ADS1015_HALL_SENSOR macro if ADS1015 config is complete
        # This will be undefined in the outer scope, so the check needs to be done here
        if "EOL_SENSORS" in pin_defs[env_name]:
            eol_sensors = pin_defs[env_name]["EOL_SENSORS"]
            if "DUAL" in eol_sensors:
                dual_pins = eol_sensors["DUAL"]
                required_dual = ["EOL_PIN_R_N", "EOL_PIN_R_S", "EOL_PIN_L_N", "EOL_PIN_L_S"]
                dual_complete = all(
                    pin_name in dual_pins and dual_pins[pin_name] is not None and dual_pins[pin_name] != ""
                    for pin_name in required_dual
                )
                if dual_complete:
                    f.write("#define USE_DUAL_HALL_SENSOR\n\n")
            
            if "ADS1015" in eol_sensors:
                ads1015_pins = eol_sensors["ADS1015"]
                required_ads1015 = [
                    "ADS1015_I2C_ADDR", "ADS1015_ALERT_PIN",
                    "ADS1015_LEFT_CHANNEL", "ADS1015_RIGHT_CHANNEL",
                    "ADS1015_LEFT_THRESHOLD_LOW", "ADS1015_LEFT_THRESHOLD_HIGH",
                    "ADS1015_RIGHT_THRESHOLD_LOW", "ADS1015_RIGHT_THRESHOLD_HIGH"
                ]
                ads1015_complete = all(
                    pin_name in ads1015_pins and ads1015_pins[pin_name] is not None and ads1015_pins[pin_name] != ""
                    for pin_name in required_ads1015
                )
                if ads1015_complete:
                    f.write("#define USE_ADS1015_HALL_SENSOR\n\n")
        
        # List of optional pins that should be wrapped in #ifdef
        optional_pins = {
            "LED_A_PIN", "LED_B_PIN", "LED_C_PIN", "PIEZO_PIN",
            "SPI_PIN_COPI", "SPI_PIN_CIPO", "SPI_PIN_SCK", "SPI_PIN_CS",
            "UART_PIN_TX", "UART_PIN_RX", "USER_BUTTON",
            "USER_PIN_14", "USER_PIN_17", "USER_PIN_18", "USER_PIN_21",
            "USER_PIN_39", "USER_PIN_40", "USER_PIN_41", "USER_PIN_42",
            "WS2812", "WS2812_DATA_PIN", "I2C_PIN_SDA", "I2C_PIN_SCL",
            "SPARE"
        }
        
        # Check for missing required pins
        missing_required_pins = []
        for pin_name, pin_value in pins.items():
            # Skip WS2812 as it's handled specially
            if pin_name == "WS2812":
                continue
            
            # Check if pin is required (not in optional list) and has no value
            if pin_name not in optional_pins and (pin_value is None or pin_value == ""):
                missing_required_pins.append(pin_name)
        
        # Print error if any required pins are missing
        if missing_required_pins:
            print("\n" + "="*70)
            print("ERROR: Required pins are missing values in pin_definitions.json")
            print("="*70)
            print(f"Environment: {env_name}")
            print(f"Missing required pins ({len(missing_required_pins)}):")
            for pin_name in sorted(missing_required_pins):
                print(f"  - {pin_name}")
            print("="*70 + "\n")
            raise ValueError(f"Missing required pin values: {', '.join(sorted(missing_required_pins))}")
        
        # List of ADS1015 threshold pins that should be int16_t
        ads1015_threshold_pins = {
            "ADS1015_LEFT_THRESHOLD_LOW", "ADS1015_LEFT_THRESHOLD_HIGH",
            "ADS1015_RIGHT_THRESHOLD_LOW", "ADS1015_RIGHT_THRESHOLD_HIGH"
        }
        
        for pin_name, pin_value in pins.items():
            # Skip WS2812 as it's handled specially above
            if pin_name == "WS2812":
                continue
            
            # Skip empty/null values for optional pins
            if pin_value is None or pin_value == "":
                continue
            
            # Generate the definition
            if isinstance(pin_value, str):
                f.write(f"#define {pin_name} {pin_value}\n")
            elif pin_name in ads1015_threshold_pins:
                # ADS1015 thresholds are 16-bit signed values
                f.write(f"constexpr int16_t {pin_name} = {pin_value};\n")
            else:
                f.write(f"constexpr uint8_t {pin_name} = {pin_value}U;\n")
        
        f.write("\n#endif // PIN_DEFINITIONS_H\n")
    
    print(f"Generated include/pin_definitions.h with {len(pins)} pin definitions")

generate_pin_definitions()
