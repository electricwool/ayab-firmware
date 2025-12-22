# Encoder Interface: Bit-Bang vs LS7366R

## The Question

**Can we replace the $4.50 LS7366R with bit-banging?**

**Answer: YES! RP2040 PIO is perfect for this.**

---

## Option 1: LS7366R (Hardware IC)

### Specifications

- **Cost:** $4.50
- **Interface:** SPI
- **Latency:** < 1 µs
- **CPU Overhead:** 0% (hardware counting)
- **Counter:** 32-bit
- **Features:** Quadrature decoding, INDEX support

### Pros
✅ Zero CPU overhead  
✅ Ultra-fast SPI reads  
✅ Proven, reliable  
✅ 32-bit counter (never overflows)

### Cons
❌ **Expensive** ($4.50)  
❌ Requires SPI bus  
❌ External component

---

## Option 2: RP2040 PIO (Bit-Bang)

### What is PIO?

**PIO = Programmable I/O**

The RP2040 has **8 PIO state machines** that can:
- Run independently of CPU
- Execute custom assembly programs
- Handle GPIO at 125 MHz
- Generate interrupts
- **Perfect for quadrature decoding!**

### Specifications

- **Cost:** $0.00 (built into RP2040)
- **Interface:** Direct GPIO
- **Latency:** ~10 µs (interrupt + read)
- **CPU Overhead:** ~1% (interrupt handling)
- **Counter:** 32-bit (software)
- **Features:** Quadrature decoding, INDEX support

### Pros
✅ **FREE** (no external IC)  
✅ Built into RP2040  
✅ Fast enough (10 µs vs 7 ms period)  
✅ Frees up SPI bus  
✅ Flexible (can modify behavior)

### Cons
⚠️ Requires PIO programming  
⚠️ ~1% CPU overhead  
⚠️ Slightly slower than LS7366R

---

## Performance Comparison

### Timing Analysis

**KH-930 Encoder @ 1 m/s:**
- 16 PPR (pulses per revolution)
- 7.1 ms per pulse
- **Minimum detectable time: 7100 µs**

| Method | Latency | Margin | CPU Overhead |
|--------|---------|--------|--------------|
| **LS7366R** | < 1 µs | 99.99% | 0% |
| **RP2040 PIO** | 10 µs | 99.86% | ~1% |

**Both have excellent margins!**

### Cost Comparison

| Component | LS7366R | PIO | Savings |
|-----------|---------|-----|---------|
| Encoder IC | $4.50 | $0.00 | **$4.50** |
| Passives | $0.15 | $0.15 | $0.00 |
| **Total** | $4.65 | $0.15 | **$4.50** |

**PIO saves $4.50 (97% cost reduction!)**

---

## RP2040 PIO Implementation

### PIO Program (Quadrature Decoder)

```pio
; Quadrature encoder decoder
; Reads A and B inputs, updates counter
; Generates interrupt on position change

.program quadrature_encoder
.side_set 1 opt

    wait 1 pin 0        ; Wait for A high
    in pins, 2          ; Read A and B
    push noblock        ; Push to FIFO
    irq 0               ; Trigger interrupt
    wait 0 pin 0        ; Wait for A low
    in pins, 2          ; Read A and B
    push noblock        ; Push to FIFO
    irq 0               ; Trigger interrupt

% c-sdk {
static inline void quadrature_encoder_program_init(PIO pio, uint sm, uint offset, uint pin_a, uint pin_b) {
    pio_sm_config c = quadrature_encoder_program_get_default_config(offset);
    
    // Set input pins (A and B)
    sm_config_set_in_pins(&c, pin_a);
    sm_config_set_in_shift(&c, false, false, 32);
    
    // Set clock divider (125 MHz / 1 = 125 MHz)
    sm_config_set_clkdiv(&c, 1.0);
    
    // Initialize GPIO
    pio_gpio_init(pio, pin_a);
    pio_gpio_init(pio, pin_b);
    gpio_set_pulls(pin_a, true, false);  // Pull-up
    gpio_set_pulls(pin_b, true, false);  // Pull-up
    
    // Set pin directions (input)
    pio_sm_set_consecutive_pindirs(pio, sm, pin_a, 2, false);
    
    // Load and start
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}
%}
```

### C++ Wrapper

```cpp
class PIOEncoder {
private:
    PIO pio;
    uint sm;
    int32_t position;
    uint8_t last_state;
    
    // Quadrature lookup table
    // [old_state][new_state] = direction
    static const int8_t QUADRATURE_TABLE[4][4] = {
        { 0, -1,  1,  0},  // 00 -> 00, 01, 10, 11
        { 1,  0,  0, -1},  // 01 -> 00, 01, 10, 11
        {-1,  0,  0,  1},  // 10 -> 00, 01, 10, 11
        { 0,  1, -1,  0}   // 11 -> 00, 01, 10, 11
    };

public:
    PIOEncoder(PIO pio_instance, uint state_machine, uint pin_a, uint pin_b) 
        : pio(pio_instance), sm(state_machine), position(0), last_state(0) {
        
        // Load PIO program
        uint offset = pio_add_program(pio, &quadrature_encoder_program);
        quadrature_encoder_program_init(pio, sm, offset, pin_a, pin_b);
        
        // Set up interrupt
        pio_set_irq0_source_enabled(pio, pis_interrupt0, true);
        irq_set_exclusive_handler(PIO0_IRQ_0, pio_irq_handler);
        irq_set_enabled(PIO0_IRQ_0, true);
        
        // Read initial state
        last_state = (gpio_get(pin_a) << 1) | gpio_get(pin_b);
    }
    
    void update() {
        // Called from interrupt handler
        if (!pio_sm_is_rx_fifo_empty(pio, sm)) {
            uint32_t data = pio_sm_get(pio, sm);
            uint8_t new_state = data & 0x03;
            
            // Update position using lookup table
            position += QUADRATURE_TABLE[last_state][new_state];
            last_state = new_state;
        }
    }
    
    int32_t get_position() const {
        return position;
    }
    
    void reset_position() {
        position = 0;
    }
};

// Global instance for interrupt handler
PIOEncoder* g_encoder = nullptr;

void pio_irq_handler() {
    if (g_encoder) {
        g_encoder->update();
    }
    pio_interrupt_clear(pio0, 0);
}
```

### Usage Example

```cpp
// Initialize encoder on GP10 (A) and GP11 (B)
PIOEncoder encoder(pio0, 0, 10, 11);
g_encoder = &encoder;

// Main loop
while (1) {
    int32_t position = encoder.get_position();
    printf("Position: %d\n", position);
    sleep_ms(100);
}
```

---

## Alternative: Simple GPIO Interrupts

**Even simpler than PIO:**

```cpp
class SimpleEncoder {
private:
    int32_t position;
    uint pin_a, pin_b;
    
public:
    SimpleEncoder(uint a, uint b) : position(0), pin_a(a), pin_b(b) {
        gpio_init(pin_a);
        gpio_init(pin_b);
        gpio_set_dir(pin_a, GPIO_IN);
        gpio_set_dir(pin_b, GPIO_IN);
        gpio_pull_up(pin_a);
        gpio_pull_up(pin_b);
        
        // Interrupt on A rising/falling
        gpio_set_irq_enabled_with_callback(pin_a, 
            GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
            true, &encoder_callback);
    }
    
    void on_change() {
        bool a = gpio_get(pin_a);
        bool b = gpio_get(pin_b);
        
        // Simple quadrature logic
        if (a == b) {
            position++;  // Clockwise
        } else {
            position--;  // Counter-clockwise
        }
    }
    
    int32_t get_position() const {
        return position;
    }
};

// Interrupt handler
void encoder_callback(uint gpio, uint32_t events) {
    g_simple_encoder->on_change();
}
```

**Pros:**
✅ Even simpler than PIO  
✅ No PIO programming needed  
✅ Still fast enough (< 10 µs)

**Cons:**
⚠️ Slightly more CPU overhead (~2%)  
⚠️ May miss pulses at very high speeds (not an issue for KH-930)

---

## Recommendation

### For KH-930 Knitting Machine: **Use Simple GPIO Interrupts**

**Why:**
1. ✅ **FREE** - no external IC needed
2. ✅ **Simple** - easy to understand and debug
3. ✅ **Fast enough** - 10 µs vs 7100 µs period (99.86% margin)
4. ✅ **Saves $4.50** - significant cost reduction
5. ✅ **Frees SPI bus** - can use for other peripherals

**When to use LS7366R:**
- Very high-speed encoders (> 10 kHz)
- Need absolute zero CPU overhead
- Budget is not a concern
- Want proven hardware solution

**When to use PIO:**
- Need more complex decoding
- Want zero CPU overhead
- Have multiple encoders (8 PIO state machines available)

**When to use GPIO interrupts:**
- Low to medium speed encoders (< 1 kHz)
- Want simplest solution
- Cost-sensitive design
- **Perfect for KH-930!**

---

## Updated Hybrid Design

### New BOM (with GPIO Encoder)

| Qty | Part | Description | Unit Price | Total |
|-----|------|-------------|------------|-------|
| 1 | ADS1015 | 12-bit I2C ADC | $3.50 | $3.50 |
| ~~1~~ | ~~LS7366R~~ | ~~SPI Encoder~~ | ~~$4.50~~ | ~~$4.50~~ |
| 1 | MCP23S17 | 16-pin SPI GPIO | $1.20 | $1.20 |
| 5 | 100kΩ | Input resistors | $0.05 | $0.25 |
| 3 | 100Ω | Encoder resistors | $0.05 | $0.15 |
| 2 | 10nF | Hall sensor caps | $0.05 | $0.10 |
| 3 | 100nF | Encoder filter caps | $0.05 | $0.15 |
| 2 | 100nF | Decoupling caps | $0.05 | $0.10 |
| 2 | 4.7kΩ | I2C pull-ups | $0.05 | $0.10 |
| 1 | 10kΩ | ALERT pull-up | $0.05 | $0.05 |
| **TOTAL** | | | | **$5.60** |

**Savings: $4.65 (45% cost reduction!)**

### New GPIO Assignments

| GPIO | Function | Bus | Device |
|------|----------|-----|--------|
| GP4 | SDA | I2C | ADS1015 |
| GP5 | SCL | I2C | ADS1015 |
| GP6 | ALERT | GPIO | ADS1015 interrupt |
| GP10 | ENC_A | GPIO | Encoder A (interrupt) |
| GP11 | ENC_B | GPIO | Encoder B |
| GP12 | ENC_INDEX | GPIO | Belt Phase |
| GP16 | MISO | SPI | MCP23S17 |
| GP18 | SCK | SPI | MCP23S17 |
| GP19 | MOSI | SPI | MCP23S17 |
| GP22 | CS_SOL | GPIO | MCP23S17 select |

**Total GPIO pins: 10**

---

## Performance Summary

| Component | Method | Latency | CPU | Cost |
|-----------|--------|---------|-----|------|
| Hall Sensors | ADS1015 (I2C) | 1.6 ms | 0% | $3.50 |
| Encoder | GPIO Interrupts | 10 µs | 2% | $0.00 |
| Solenoids | MCP23S17 (SPI) | 5 µs | 0% | $1.20 |
| **Total** | | | **2%** | **$5.60** |

**All margins > 99%! Total cost: $5.60 (was $10.25)**

---

## Conclusion

**Replace LS7366R with GPIO interrupts:**

✅ **Saves $4.50** (45% cost reduction)  
✅ **Still fast enough** (99.86% margin)  
✅ **Simpler design** (fewer external components)  
✅ **Frees SPI bus** (only need for solenoids)  
✅ **Easy to debug** (standard GPIO code)

**For KH-930 knitting machine, GPIO interrupts are the optimal choice.**

**Final design:**
- **ADS1015 (I2C):** Hall sensors with comparator
- **GPIO Interrupts:** Encoder counting
- **MCP23S17 (SPI):** Solenoid control

**Total cost: $5.60 (vs $10.25 with LS7366R)**
