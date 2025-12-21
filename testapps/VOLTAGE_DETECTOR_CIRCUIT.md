# Voltage Detector Circuit Design

## Overview
Simple voltage detection circuit using a comparator with adjustable threshold for detecting three voltage states on a single wire:
- **LOW**: 0V
- **INACTIVE**: 1.68V
- **HIGH**: 3.47V

The circuit outputs a clean digital signal to an RP2040 GPIO pin (3.3V logic).

## Circuit Design

### Components Required
- **U1**: LM393 Dual Comparator (or LM339 Quad) - $0.30
- **R1**: 10kΩ Potentiometer (threshold adjustment) - $0.50
- **R2**: 10kΩ Resistor (voltage divider for reference) - $0.05
- **R3**: 10kΩ Resistor (pull-up for output) - $0.05
- **R4**: 100kΩ Resistor (input protection) - $0.05
- **C1**: 100nF Ceramic Capacitor (power supply decoupling) - $0.05
- **C2**: 10nF Ceramic Capacitor (input filtering/debounce) - $0.05
- **D1**: 1N4148 Diode (input protection, optional) - $0.05

**Total Cost**: ~$1.10

### Circuit Schematic

```
Input Signal (0V / 1.68V / 3.47V)
    |
    +---[R4: 100k]---+---[C2: 10nF]---+
                     |                |
                    [D1]             GND
                     |
                     +-------------(+) IN+ (Pin 3)
                                      |
                     LM393            |
                                      |
    3.3V                             OUT (Pin 1)---[R3: 10k]---3.3V----> To RP2040 GPIO
     |                                |
     |                                |
     +---[R2: 10k]---+               GND
                     |
                  [POT: 10k]
                     |
                    GND
                     |
                     +------------(-) IN- (Pin 2)

    3.3V---[C1: 100nF]---GND  (Power decoupling)
     |                    |
    VCC (Pin 8)         GND (Pin 4)
```

### Pin Connections (LM393)
- **Pin 1**: Output (to RP2040 GPIO via R3 pull-up)
- **Pin 2**: IN- (Inverting input - Reference voltage from potentiometer)
- **Pin 3**: IN+ (Non-inverting input - Signal to measure)
- **Pin 4**: GND
- **Pin 5-7**: Comparator 2 (unused, can be left floating or tied to GND)
- **Pin 8**: VCC (3.3V)

## How It Works

### Comparator Operation
The LM393 compares the input signal voltage (IN+) against the adjustable reference voltage (IN-):
- When **Input > Reference**: Output goes HIGH (pulled to 3.3V via R3)
- When **Input < Reference**: Output goes LOW (pulled to GND internally)

### Threshold Adjustment
The potentiometer creates an adjustable reference voltage between 0V and 3.3V:
- **Adjust threshold** to detect transitions between states
- Typical setting: ~2.0V to 2.5V to distinguish between INACTIVE (1.68V) and HIGH (3.47V)
- For detecting LOW (0V) vs INACTIVE (1.68V), set threshold to ~1.0V

### Input Protection
- **R4 (100kΩ)**: Limits current if input voltage exceeds safe levels
- **C2 (10nF)**: Filters high-frequency noise and provides basic debouncing
- **D1 (optional)**: Clamps negative voltages to protect comparator input

### Output
- **R3 (10kΩ)**: Pull-up resistor ensures clean HIGH state
- Output is compatible with RP2040 digital input thresholds:
  - HIGH: > 2.0V (3.3V when pulled up)
  - LOW: < 0.8V (near 0V when comparator pulls low)

## Calibration Procedure

1. **Connect circuit** to 3.3V power and RP2040 GPIO
2. **Apply test voltages** to input:
   - 0V (LOW state)
   - 1.68V (INACTIVE state)
   - 3.47V (HIGH state)
3. **Adjust potentiometer** to set desired threshold:
   - For HIGH/LOW detection: Set threshold between 1.68V and 3.47V (~2.5V)
   - For INACTIVE/LOW detection: Set threshold between 0V and 1.68V (~1.0V)
4. **Verify transitions** are clean and reliable

## Alternative: Dual Threshold Detection

For detecting all three states independently, use both comparators in the LM393:

### Comparator 1: Detects HIGH (3.47V)
- Threshold: ~2.5V (between 1.68V and 3.47V)
- Output HIGH when input > 2.5V

### Comparator 2: Detects ACTIVE (not LOW)
- Threshold: ~0.8V (between 0V and 1.68V)
- Output HIGH when input > 0.8V

### State Decoding (requires 2 GPIO pins)
| Input Voltage | Comp1 Output | Comp2 Output | State |
|---------------|--------------|--------------|-------|
| 0V            | LOW          | LOW          | LOW   |
| 1.68V         | LOW          | HIGH         | INACTIVE |
| 3.47V         | HIGH         | HIGH         | HIGH  |

## PCB Layout Considerations

1. **Keep comparator inputs short** to minimize noise pickup
2. **Place C1 close to VCC pin** for effective decoupling
3. **Ground plane** recommended for stable reference voltage
4. **Separate analog and digital grounds** if possible, connect at single point

## Software Integration

### RP2040 GPIO Configuration
```cpp
// Configure GPIO as input with internal pull-up disabled
gpio_init(GPIO_PIN);
gpio_set_dir(GPIO_PIN, GPIO_IN);
gpio_disable_pulls(GPIO_PIN); // External pull-up R3 provides pull-up

// Read state
bool state = gpio_get(GPIO_PIN);
```

### Interrupt-Based Detection
```cpp
// Enable interrupt on both edges
gpio_set_irq_enabled_with_callback(GPIO_PIN, 
    GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 
    true, 
    &gpio_callback);

void gpio_callback(uint gpio, uint32_t events) {
    if (events & GPIO_IRQ_EDGE_RISE) {
        // Voltage crossed threshold (rising)
    }
    if (events & GPIO_IRQ_EDGE_FALL) {
        // Voltage crossed threshold (falling)
    }
}
```

## Advantages

1. **Low cost**: ~$1.10 in components
2. **Simple**: Only one IC and a few passive components
3. **Adjustable**: Potentiometer allows field calibration
4. **Clean output**: Comparator provides crisp digital transitions
5. **Protected**: Input protection prevents damage from voltage spikes
6. **Low power**: LM393 draws ~1mA typical

## Limitations

1. **Single threshold**: Basic design only detects one threshold at a time
2. **Manual adjustment**: Potentiometer requires manual calibration
3. **No isolation**: Input is not isolated from MCU (use optocoupler if needed)

## Enhancements

- **Add hysteresis**: Connect 1MΩ resistor from output to IN+ for noise immunity
- **Software threshold**: Use ADC instead for fully programmable thresholds (but requires ADC pin)
- **Dual comparator**: Use both comparators for three-state detection (requires 2 GPIO pins)

## Bill of Materials (BOM)

| Qty | Part Number | Description | Unit Price | Total |
|-----|-------------|-------------|------------|-------|
| 1   | LM393       | Dual Comparator | $0.30 | $0.30 |
| 1   | 3296W-103   | 10kΩ Trim Pot | $0.50 | $0.50 |
| 2   | 10kΩ 1/4W   | Resistor | $0.05 | $0.10 |
| 1   | 100kΩ 1/4W  | Resistor | $0.05 | $0.05 |
| 1   | 100nF       | Ceramic Cap | $0.05 | $0.05 |
| 1   | 10nF        | Ceramic Cap | $0.05 | $0.05 |
| 1   | 1N4148      | Diode (optional) | $0.05 | $0.05 |
| **TOTAL** | | | | **$1.10** |

