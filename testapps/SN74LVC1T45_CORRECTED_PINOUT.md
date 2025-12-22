# SN74LVC1T45DBVR Corrected Pinout and Circuit

## ⚠️ CRITICAL CORRECTION

**Previous documentation contained INCORRECT pinout information for the SN74LVC1T45DBVR.**

---

## CORRECT Pinout (SOT-23-6 Package)

```
     ┌─────┐
VCCA │1   6│ VCCB
 GND │2   5│ DIR
   A │3   4│ B
     └─────┘
```

### Pin Assignments:
- **Pin 1:** VCCA (5V supply side)
- **Pin 2:** GND (Ground)
- **Pin 3:** A (Port A - 5V side input/output)
- **Pin 4:** B (Port B - 3.3V side input/output) ✓
- **Pin 5:** DIR (Direction control) ✓
- **Pin 6:** VCCB (3.3V supply side) ✓

---

## Direction Control

### DIR Pin Function:
- **DIR = HIGH**: Data flows from A to B (5V → 3.3V)
- **DIR = LOW**: Data flows from B to A (3.3V → 5V)

**For 5V to 3.3V level shifting: DIR must be HIGH**

### DIR Pin Logic Level Reference:

**Confirmed:** DIR pin logic threshold is referenced to **VCCA** (the A-side supply).

From TI datasheet layout example (Figure 8-5), the DIR pin should be connected through a **pull-up or pull-down resistor**.

**Recommended Implementation for A→B (5V→3.3V) Direction:**

```
Pin 5 (DIR) ──[10kΩ]──> VCCA (5V)

This pulls DIR HIGH for A→B direction
```

**Resistor Value:** **10kΩ** (standard value for DIR pin pull-up/pull-down)

**Alternative - Direct Connection (if DIR is static):**
```
Pin 5 (DIR) → VCCA (5V) directly  ✓ For permanent A→B direction
Pin 5 (DIR) → GND directly        ✓ For permanent B→A direction
```

**Why use a resistor:**
- Allows dynamic direction control if needed
- Provides weak pull-up/pull-down
- Standard practice per TI layout example
- 10kΩ is typical for control pins

**For fixed unidirectional 5V→3.3V:** Either direct connection to VCCA or 10kΩ pull-up to VCCA works.

---

## CORRECT Circuit for 5V → 3.3V Level Shifting

```
                    SN74LVC1T45DBVR
                    ┌─────────────┐
5V Supply ──[0.1µF]─┤1   VCCA     │
                    │             │
GND ────────────────┤2   GND      │
                    │             │
5V Input ───────────┤3   A        │
                    │             │
                    │       B    4│──────> 3.3V Output
                    │             │
3.3V (HIGH) ────────┤5   DIR      │  ← Must be HIGH for A→B
                    │             │
3.3V Supply ─[0.1µF]┤6   VCCB     │
                    └─────────────┘
```

---

## Complete Connection Details

### For Unidirectional 5V → 3.3V:

```
Pin 1 (VCCA)  → 5V supply + 0.1µF ceramic cap to GND
Pin 2 (GND)   → Ground plane
Pin 3 (A)     → 5V input signal (from encoder, sensor, etc.)
Pin 4 (B)     → 3.3V output signal (to MCU GPIO)
Pin 5 (DIR)   → 3.3V (tie to VCCB for A→B direction)
Pin 6 (VCCB)  → 3.3V supply + 0.1µF ceramic cap to GND
```

### Decoupling Capacitors:
- **C1:** 0.1µF ceramic, 50V, X7R (at VCCA, pin 1)
- **C2:** 0.1µF ceramic, 50V, X7R (at VCCB, pin 6)
- Place capacitors as close as possible to IC pins (<5mm)

---

## Example: 3-Channel Encoder Level Shifter

```
Encoder A (5V) ──[100Ω]──> 74LVC1T45 #1 Pin 3 (A)
                           Pin 4 (B) ──> MCU GPIO (3.3V)
                           Pin 5 (DIR) ──> 3.3V

Encoder B (5V) ──[100Ω]──> 74LVC1T45 #2 Pin 3 (A)
                           Pin 4 (B) ──> MCU GPIO (3.3V)
                           Pin 5 (DIR) ──> 3.3V

Index (5V) ────[100Ω]──> 74LVC1T45 #3 Pin 3 (A)
                         Pin 4 (B) ──> MCU GPIO (3.3V)
                         Pin 5 (DIR) ──> 3.3V

All ICs:
- Pin 1 (VCCA) → 5V + 0.1µF to GND
- Pin 2 (GND) → GND
- Pin 6 (VCCB) → 3.3V + 0.1µF to GND
```

---

## Common Mistakes to Avoid

### ❌ WRONG - DIR connected to GND:
```
Pin 5 (DIR) → GND  ✗ This enables B→A (3.3V→5V), NOT A→B!
```

### ❌ WRONG - DIR connected to VCCA (5V):
```
Pin 5 (DIR) → 5V  ✗ DIR pin is referenced to VCCB, not VCCA!
```

### ❌ WRONG - Swapped B and DIR pins:
```
Pin 4 → DIR  ✗ Pin 4 is B (output)
Pin 5 → Output  ✗ Pin 5 is DIR (control)
```

### ✅ CORRECT - DIR connected to VCCB (3.3V):
```
Pin 5 (DIR) → 3.3V (VCCB)  ✓ Enables A→B (5V→3.3V)
```

---

## Verification Checklist

Before powering up your circuit, verify:

- [ ] Pin 1 (VCCA) connected to 5V supply
- [ ] Pin 2 (GND) connected to ground
- [ ] Pin 3 (A) connected to 5V input signal
- [ ] Pin 4 (B) connected to 3.3V output (to MCU)
- [ ] Pin 5 (DIR) connected to 3.3V (HIGH) for A→B direction
- [ ] Pin 6 (VCCB) connected to 3.3V supply
- [ ] 0.1µF decoupling caps on both VCCA and VCCB
- [ ] All connections match the corrected pinout above

---

## Datasheet Reference

**Part Number:** SN74LVC1T45DBVR  
**Manufacturer:** Texas Instruments  
**Package:** SOT-23-6 (DBV)  
**Datasheet:** [TI SN74LVC1T45](https://www.ti.com/lit/ds/symlink/sn74lvc1t45.pdf)

**Key Specifications:**
- VCCA Range: 1.65V to 5.5V
- VCCB Range: 1.65V to 5.5V
- Propagation Delay: 2.5ns typical
- Output Drive: ±24mA
- DIR Input: Referenced to VCCB

---

## Summary

**Critical Points:**
1. **Pin 4 is B (output)**, NOT DIR
2. **Pin 5 is DIR (control)**, NOT B
3. **Pin 6 is VCCB (3.3V supply)**, NOT DIR
4. **DIR must be HIGH (3.3V)** for A→B (5V→3.3V) direction
5. **DIR is referenced to VCCB**, not VCCA

This corrected pinout must be used in all circuit designs to ensure proper operation of the SN74LVC1T45DBVR level shifter.
