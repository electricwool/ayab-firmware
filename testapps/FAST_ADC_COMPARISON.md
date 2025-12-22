# Fast ADC Options for Carriage Detection

## Overview
Comparison of ADC options for carriage position detection, trading resolution for speed. For bipolar hall sensor detection (3 states: South/Inactive/North), high resolution is not critical.

---

## ADC Comparison Table

| ADC Model | Resolution | Sample Rate | Latency | I2C Speed | Price | Best For |
|-----------|------------|-------------|---------|-----------|-------|----------|
| **ADS1015** | 12-bit | 3300 SPS | 1.6 ms | 400 kHz | $3.50 | **Current choice** |
| **ADS7924** | 12-bit | 1000 SPS | 2.0 ms | 400 kHz | $2.50 | Lower cost |
| **MCP3021** | 10-bit | Continuous | 0.8 ms | 400 kHz | $1.20 | **Faster, cheaper** |
| **MCP3221** | 12-bit | Continuous | 1.0 ms | 400 kHz | $1.50 | Good balance |
| **TLA2024** | 12-bit | 3300 SPS | 1.5 ms | 400 kHz | $1.80 | Cheaper ADS1015 |
| **ADS1013** | 12-bit | 3300 SPS | 1.6 ms | 400 kHz | $2.80 | Single channel |
| **MCP3425** | 16-bit | 240 SPS | 5.0 ms | 400 kHz | $2.00 | High precision |
| **Comparator** | 1-bit | Instant | **10 µs** | N/A | $0.30 | **Fastest!** |

---

## Detailed Analysis

### Option 1: Comparator-Based Detection (Fastest)

**Component:** LM393 Dual Comparator

**Specifications:**
- Resolution: 1-bit (HIGH/LOW only)
- Response time: ~1 µs
- Total latency: **~10 µs** (160× faster than ADS1015!)
- Cost: $0.30 per IC

**Circuit per Sensor:**
```
Hall Sensor → Comparator → GPIO Interrupt

Sensor voltage:
- South pole: 0V → Comparator OUT = LOW
- Inactive: 1.68V → Comparator OUT = MID (need 2 comparators)
- North pole: 3.47V → Comparator OUT = HIGH

Solution: Use 2 comparators per sensor
- Comparator 1: Threshold @ 1.0V (detects South)
- Comparator 2: Threshold @ 2.5V (detects North)
```

**Advantages:**
- ✅ **Ultra-fast:** 10 µs latency (160× faster)
- ✅ **Cheapest:** $0.30 per comparator IC
- ✅ **No I2C:** Direct GPIO connection
- ✅ **Event-driven:** Instant interrupt on change
- ✅ **Simple:** No complex firmware

**Disadvantages:**
- ❌ **No analog value:** Can't measure exact voltage
- ❌ **More GPIO pins:** 2 pins per sensor (4 total)
- ❌ **Fixed thresholds:** Requires resistor changes to adjust
- ❌ **No polarity info:** Can't distinguish N/S directly

**Suitability:** ⚠️ **Not ideal** - loses polarity detection capability needed for carriage identification

---

### Option 2: MCP3021 (10-bit, Fast & Cheap)

**Specifications:**
- Resolution: 10-bit (1024 levels)
- Sample rate: Continuous conversion
- Latency: **~0.8 ms** (2× faster than ADS1015)
- Cost: $1.20 (66% cheaper)

**Performance:**
- Voltage range: 0-3.3V
- Resolution: 3.3V / 1024 = 3.2 mV per count
- Hall sensor swing: 1.79V = 559 counts ✅ Plenty!

**Latency Breakdown:**
| Step | Time (µs) |
|------|-----------|
| Detection to interrupt | 318 |
| I2C read (2 bytes) | 40 |
| Process value | 5 |
| Update state | 10 |
| **Total** | **373 µs** |

**Advantages:**
- ✅ **2× faster:** 0.8 ms vs 1.6 ms
- ✅ **66% cheaper:** $1.20 vs $3.50
- ✅ **Sufficient resolution:** 559 counts for detection
- ✅ **Simple I2C:** Standard interface
- ✅ **Continuous conversion:** No MUX switching

**Disadvantages:**
- ❌ **No comparator:** Must poll or use external comparator
- ❌ **Single channel:** Need 2 ICs for 2 sensors
- ❌ **No ALERT pin:** Can't trigger interrupt automatically

**Suitability:** ⚠️ **Marginal** - lacks ALERT pin for event-driven operation

---

### Option 3: MCP3221 (12-bit, Good Balance)

**Specifications:**
- Resolution: 12-bit (4096 levels)
- Sample rate: Continuous conversion
- Latency: **~1.0 ms** (1.6× faster than ADS1015)
- Cost: $1.50 (57% cheaper)

**Performance:**
- Same resolution as ADS1015 (12-bit)
- Voltage range: 0-3.3V
- Resolution: 0.8 mV per count
- Hall sensor swing: 1.79V = 2237 counts ✅ Excellent!

**Advantages:**
- ✅ **1.6× faster:** 1.0 ms vs 1.6 ms
- ✅ **57% cheaper:** $1.50 vs $3.50
- ✅ **Same resolution:** 12-bit like ADS1015
- ✅ **Simple I2C:** Standard interface

**Disadvantages:**
- ❌ **No comparator:** Must poll
- ❌ **Single channel:** Need 2 ICs
- ❌ **No ALERT pin:** No automatic interrupt

**Suitability:** ⚠️ **Marginal** - lacks ALERT pin

---

### Option 4: TLA2024 (12-bit, Cheaper ADS1015 Alternative)

**Specifications:**
- Resolution: 12-bit (4096 levels)
- Sample rate: 3300 SPS (same as ADS1015)
- Latency: **~1.5 ms** (similar to ADS1015)
- Cost: $1.80 (49% cheaper)

**Performance:**
- Nearly identical to ADS1015
- Has comparator with ALERT pin ✅
- 4 channels (can monitor 4 sensors)
- I2C interface

**Advantages:**
- ✅ **49% cheaper:** $1.80 vs $3.50
- ✅ **Same features:** Comparator + ALERT
- ✅ **Same speed:** 3300 SPS
- ✅ **Drop-in replacement:** Compatible firmware

**Disadvantages:**
- ❌ **Similar latency:** Only 0.1 ms faster
- ❌ **Less common:** Harder to source

**Suitability:** ✅ **Good alternative** - cheaper with same performance

---

### Option 5: Hybrid Comparator + ADC

**Best of both worlds:**

**Configuration:**
- **Comparators (LM393):** Fast detection, trigger interrupt
- **ADC (MCP3221):** Read exact voltage when needed

**Circuit:**
```
Hall Sensor → Comparator → GPIO Interrupt (fast detection)
            ↓
            → ADC → I2C (precise measurement)

Comparator triggers interrupt immediately (10 µs)
Firmware reads ADC for exact voltage (1.0 ms)
```

**Advantages:**
- ✅ **Ultra-fast detection:** 10 µs interrupt
- ✅ **Precise measurement:** 12-bit ADC when needed
- ✅ **Event-driven:** Comparator triggers interrupt
- ✅ **Flexible:** Can read analog value or just detect change

**Disadvantages:**
- ❌ **More components:** Comparator + ADC
- ❌ **More GPIO pins:** 1 per comparator
- ❌ **Complex:** Two systems to manage

**Cost:**
- 2× LM393 (4 comparators): $0.60
- 2× MCP3221 (2 ADCs): $3.00
- **Total: $3.60** (similar to ADS1015)

**Suitability:** ✅ **Excellent** - combines speed and precision

---

## Recommended Solutions

### For Carriage Detection (Polarity Sensing Required)

**Option A: Keep ADS1015 (Current Design)**
- Cost: $3.50
- Latency: 1.6 ms
- ✅ Best all-in-one solution
- ✅ Proven design
- ✅ Event-driven with ALERT

**Option B: TLA2024 (Cheaper Alternative)**
- Cost: $1.80 (49% savings)
- Latency: 1.5 ms
- ✅ Drop-in replacement
- ✅ Same features
- ⚠️ Less common part

**Option C: Hybrid Comparator + MCP3221**
- Cost: $3.60
- Latency: 10 µs (detection) + 1.0 ms (measurement)
- ✅ Ultra-fast detection
- ✅ Precise measurement
- ❌ More complex

### For Simple Presence Detection (No Polarity Needed)

**Option D: Comparators Only**
- Cost: $0.60 (83% savings!)
- Latency: 10 µs
- ✅ Fastest possible
- ✅ Cheapest
- ❌ No polarity detection

---

## Performance Comparison

### Detection Window Analysis

**Carriage detection window:** 20 ms (magnet over sensor)

| Solution | Latency | Margin | Detections | Cost |
|----------|---------|--------|------------|------|
| ADS1015 | 1.6 ms | 92% | 12× | $3.50 |
| TLA2024 | 1.5 ms | 92.5% | 13× | $1.80 |
| MCP3221 | 1.0 ms | 95% | 20× | $1.50 |
| MCP3021 | 0.8 ms | 96% | 25× | $1.20 |
| Comparator | 0.01 ms | 99.95% | 2000× | $0.60 |
| Hybrid | 0.01 ms + 1.0 ms | 95% | 20× | $3.60 |

---

## Recommendation

**For this application (carriage identification with polarity detection):**

### Best Choice: ADS1015 (Current Design)
**Rationale:**
- ✅ All-in-one solution (ADC + comparator + ALERT)
- ✅ Event-driven operation
- ✅ 92% margin (plenty for 20ms window)
- ✅ Proven, widely available
- ✅ Simple firmware
- ✅ Can detect polarity (North/South)

### Budget Alternative: TLA2024
**Rationale:**
- ✅ 49% cost savings ($1.80 vs $3.50)
- ✅ Same performance
- ✅ Drop-in replacement
- ⚠️ Less common, may be harder to source

### Maximum Performance: Hybrid Comparator + MCP3221
**Rationale:**
- ✅ 160× faster detection (10 µs)
- ✅ Still has precise measurement capability
- ✅ Best of both worlds
- ❌ More complex design
- ❌ Similar cost to ADS1015

---

## Conclusion

**For carriage detection with polarity sensing:**
- **Keep ADS1015** - best balance of features, performance, and simplicity
- **Consider TLA2024** - if cost is critical and part is available
- **Use comparators** - only if polarity detection not needed

**For encoder signals:**
- **Use GPIO interrupts** - 160× faster, no ADC needed (as designed in optimized system)

The current design using **ADS1015 for carriage detection** and **GPIO for encoders** is optimal for this application.
