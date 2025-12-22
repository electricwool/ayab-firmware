/**
 * @file encoder_level_shifter_test.cpp
 * @brief Test firmware for TXS0104E level shifter with KH-930 encoder
 * 
 * Hardware Setup:
 * - RP2040 Pico W
 * - TXS0104E 4-channel level shifter
 * - KH-930 encoder (5V logic)
 * 
 * Connections:
 * - GP10: Encoder A (via TXS0104E)
 * - GP11: Encoder B (via TXS0104E)
 * - GP12: Belt Phase/ENC_C (via TXS0104E)
 * 
 * Features:
 * - Quadrature encoder decoding
 * - Direction detection
 * - Speed measurement
 * - Belt phase monitoring
 * - Real-time statistics
 * - Serial output for monitoring
 * 
 * Compile with: platformio run -e pico
 */

#include <Arduino.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

// ============================================================================
// Configuration
// ============================================================================

// GPIO Pin Definitions
#define ENCODER_A_PIN     10    // GP10 - Encoder A (quadrature)
#define ENCODER_B_PIN     11    // GP11 - Encoder B (quadrature)
#define BELT_PHASE_PIN    12    // GP12 - Belt Phase (ENC_C)

// LED Pin (built-in on Pico W)
#define LED_PIN           25    // Built-in LED

// Timing
#define REPORT_INTERVAL_MS  1000  // Status report every 1 second
#define LED_BLINK_MS        100   // LED blink duration

// ============================================================================
// Global Variables
// ============================================================================

// Encoder state
volatile int32_t g_encoder_position = 0;
volatile int8_t g_encoder_direction = 0;  // -1 = left, 0 = stopped, +1 = right
volatile uint8_t g_encoder_state = 0;     // Current state [B][A]
volatile uint32_t g_encoder_changes = 0;  // Total state changes

// Belt phase state
volatile bool g_belt_phase = false;
volatile uint32_t g_belt_phase_changes = 0;

// Speed measurement
volatile uint32_t g_last_pulse_time = 0;
volatile uint32_t g_pulse_period_us = 0;  // Time between pulses (microseconds)

// Statistics
volatile uint32_t g_total_interrupts = 0;
volatile uint32_t g_invalid_transitions = 0;

// Quadrature lookup table
// Index: [previous_state][current_state] = direction
// States: 00=0, 01=1, 10=2, 11=3
const int8_t QUADRATURE_TABLE[4][4] = {
  // From 00:  to 00, 01, 10, 11
  {  0, -1,  1,  0 },
  // From 01:  to 00, 01, 10, 11
  {  1,  0,  0, -1 },
  // From 10:  to 00, 01, 10, 11
  { -1,  0,  0,  1 },
  // From 11:  to 00, 01, 10, 11
  {  0,  1, -1,  0 }
};

// ============================================================================
// Interrupt Handlers
// ============================================================================

/**
 * @brief Encoder interrupt handler (called on any edge of A or B)
 */
void encoder_irq_handler(uint gpio, uint32_t events) {
  g_total_interrupts++;
  
  // Read current state of both encoder pins
  uint8_t prev_state = g_encoder_state & 0x03;
  uint8_t a = gpio_get(ENCODER_A_PIN);
  uint8_t b = gpio_get(ENCODER_B_PIN);
  uint8_t curr_state = (b << 1) | a;
  
  // Update encoder state
  g_encoder_state = (prev_state << 2) | curr_state;
  
  // Lookup direction from table
  int8_t dir = QUADRATURE_TABLE[prev_state][curr_state];
  
  if (dir != 0) {
    // Valid transition
    g_encoder_position += dir;
    g_encoder_direction = dir;
    g_encoder_changes++;
    
    // Measure pulse period for speed calculation
    uint32_t now = time_us_32();
    if (g_last_pulse_time != 0) {
      g_pulse_period_us = now - g_last_pulse_time;
    }
    g_last_pulse_time = now;
  } else if (prev_state != curr_state) {
    // Invalid transition (should not happen with good encoder)
    g_invalid_transitions++;
  }
}

/**
 * @brief Belt phase interrupt handler (called on any edge)
 */
void belt_phase_irq_handler(uint gpio, uint32_t events) {
  g_total_interrupts++;
  g_belt_phase = gpio_get(BELT_PHASE_PIN);
  g_belt_phase_changes++;
}

// ============================================================================
// Initialization Functions
// ============================================================================

/**
 * @brief Initialize encoder GPIO pins with interrupts
 */
void init_encoder_pins() {
  // Encoder A
  gpio_init(ENCODER_A_PIN);
  gpio_set_dir(ENCODER_A_PIN, GPIO_IN);
  gpio_pull_up(ENCODER_A_PIN);
  
  // Encoder B
  gpio_init(ENCODER_B_PIN);
  gpio_set_dir(ENCODER_B_PIN, GPIO_IN);
  gpio_pull_up(ENCODER_B_PIN);
  
  // Belt Phase
  gpio_init(BELT_PHASE_PIN);
  gpio_set_dir(BELT_PHASE_PIN, GPIO_IN);
  gpio_pull_up(BELT_PHASE_PIN);
  
  // Read initial states
  uint8_t a = gpio_get(ENCODER_A_PIN);
  uint8_t b = gpio_get(ENCODER_B_PIN);
  g_encoder_state = (b << 1) | a;
  g_belt_phase = gpio_get(BELT_PHASE_PIN);
  
  // Set up interrupts
  gpio_set_irq_enabled_with_callback(ENCODER_A_PIN, 
                                     GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
                                     true, 
                                     &encoder_irq_handler);
  
  gpio_set_irq_enabled(ENCODER_B_PIN, 
                       GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
                       true);
  
  gpio_set_irq_enabled_with_callback(BELT_PHASE_PIN,
                                     GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
                                     true,
                                     &belt_phase_irq_handler);
}

/**
 * @brief Initialize LED pin
 */
void init_led() {
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);
  gpio_put(LED_PIN, 0);
}

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * @brief Calculate encoder speed in RPM
 * @return Speed in RPM (0 if stopped)
 */
float calculate_speed_rpm() {
  if (g_pulse_period_us == 0) {
    return 0.0f;
  }
  
  // KH-930 encoder: 16 PPR (pulses per revolution)
  // With quadrature: 64 edges per revolution (4× multiplier)
  const float EDGES_PER_REV = 64.0f;
  
  // Convert pulse period to frequency
  float frequency_hz = 1000000.0f / g_pulse_period_us;
  
  // Convert to RPM
  float rpm = (frequency_hz * 60.0f) / EDGES_PER_REV;
  
  return rpm;
}

/**
 * @brief Calculate carriage speed in mm/s
 * @return Speed in mm/s (0 if stopped)
 */
float calculate_speed_mm_s() {
  // Assuming 0.1mm per encoder count (adjust based on actual machine)
  const float MM_PER_COUNT = 0.1f;
  
  if (g_pulse_period_us == 0) {
    return 0.0f;
  }
  
  // Counts per second
  float counts_per_sec = 1000000.0f / g_pulse_period_us;
  
  // Convert to mm/s
  return counts_per_sec * MM_PER_COUNT;
}

/**
 * @brief Get direction as string
 */
const char* get_direction_string() {
  if (g_encoder_direction > 0) return "RIGHT →";
  if (g_encoder_direction < 0) return "LEFT  ←";
  return "STOPPED";
}

/**
 * @brief Print encoder state bits
 */
void print_encoder_state() {
  uint8_t state = g_encoder_state & 0x03;
  Serial.print("State: ");
  Serial.print((state & 0x02) ? "1" : "0");
  Serial.print((state & 0x01) ? "1" : "0");
  Serial.print(" (B=");
  Serial.print((state & 0x02) ? "HIGH" : "LOW");
  Serial.print(", A=");
  Serial.print((state & 0x01) ? "HIGH" : "LOW");
  Serial.print(")");
}

/**
 * @brief Blink LED
 */
void blink_led() {
  gpio_put(LED_PIN, 1);
  sleep_ms(LED_BLINK_MS);
  gpio_put(LED_PIN, 0);
}

// ============================================================================
// Main Functions
// ============================================================================

void setup() {
  // Initialize serial
  Serial.begin(115200);
  sleep_ms(2000);  // Wait for serial connection
  
  // Print header
  Serial.println("\n\n");
  Serial.println("========================================");
  Serial.println("  KH-930 Encoder Level Shifter Test");
  Serial.println("========================================");
  Serial.println("Hardware: RP2040 + TXS0104E");
  Serial.println("Encoder: 5V → 3.3V level shifted");
  Serial.println("========================================\n");
  
  // Initialize hardware
  Serial.println("Initializing hardware...");
  init_led();
  init_encoder_pins();
  
  // Print initial state
  Serial.println("\nInitial State:");
  Serial.print("  Encoder A (GP10): ");
  Serial.println(gpio_get(ENCODER_A_PIN) ? "HIGH" : "LOW");
  Serial.print("  Encoder B (GP11): ");
  Serial.println(gpio_get(ENCODER_B_PIN) ? "HIGH" : "LOW");
  Serial.print("  Belt Phase (GP12): ");
  Serial.println(g_belt_phase ? "HIGH" : "LOW");
  Serial.print("  ");
  print_encoder_state();
  Serial.println();
  
  Serial.println("\nReady! Rotate encoder to test...\n");
  
  // Blink LED to indicate ready
  for (int i = 0; i < 3; i++) {
    blink_led();
    sleep_ms(100);
  }
}

void loop() {
  static uint32_t last_report_time = 0;
  static int32_t last_position = 0;
  static uint32_t last_changes = 0;
  static uint32_t last_belt_changes = 0;
  static uint32_t last_interrupts = 0;
  static uint32_t last_invalid = 0;
  
  uint32_t now = millis();
  
  // Print status report every second
  if (now - last_report_time >= REPORT_INTERVAL_MS) {
    last_report_time = now;
    
    // Calculate deltas
    int32_t position_delta = g_encoder_position - last_position;
    uint32_t changes_delta = g_encoder_changes - last_changes;
    uint32_t belt_delta = g_belt_phase_changes - last_belt_changes;
    uint32_t interrupts_delta = g_total_interrupts - last_interrupts;
    uint32_t invalid_delta = g_invalid_transitions - last_invalid;
    
    // Update last values
    last_position = g_encoder_position;
    last_changes = g_encoder_changes;
    last_belt_changes = g_belt_phase_changes;
    last_interrupts = g_total_interrupts;
    last_invalid = g_invalid_transitions;
    
    // Calculate speeds
    float rpm = calculate_speed_rpm();
    float mm_s = calculate_speed_mm_s();
    
    // Print report
    Serial.println("----------------------------------------");
    Serial.print("Time: ");
    Serial.print(now / 1000);
    Serial.println(" seconds");
    
    Serial.println("\nEncoder Status:");
    Serial.print("  Position: ");
    Serial.print(g_encoder_position);
    Serial.print(" (Δ");
    Serial.print(position_delta);
    Serial.println(")");
    
    Serial.print("  Direction: ");
    Serial.println(get_direction_string());
    
    Serial.print("  ");
    print_encoder_state();
    Serial.println();
    
    Serial.print("  Changes: ");
    Serial.print(g_encoder_changes);
    Serial.print(" (");
    Serial.print(changes_delta);
    Serial.println("/sec)");
    
    Serial.println("\nSpeed:");
    Serial.print("  RPM: ");
    Serial.println(rpm, 2);
    Serial.print("  mm/s: ");
    Serial.println(mm_s, 2);
    Serial.print("  Pulse period: ");
    Serial.print(g_pulse_period_us);
    Serial.println(" µs");
    
    Serial.println("\nBelt Phase:");
    Serial.print("  State: ");
    Serial.println(g_belt_phase ? "HIGH (Shifted)" : "LOW (Regular)");
    Serial.print("  Changes: ");
    Serial.print(g_belt_phase_changes);
    Serial.print(" (");
    Serial.print(belt_delta);
    Serial.println("/sec)");
    
    Serial.println("\nStatistics:");
    Serial.print("  Total interrupts: ");
    Serial.print(g_total_interrupts);
    Serial.print(" (");
    Serial.print(interrupts_delta);
    Serial.println("/sec)");
    
    Serial.print("  Invalid transitions: ");
    Serial.print(g_invalid_transitions);
    Serial.print(" (");
    Serial.print(invalid_delta);
    Serial.println("/sec)");
    
    if (g_invalid_transitions > 0) {
      float error_rate = (float)g_invalid_transitions / g_encoder_changes * 100.0f;
      Serial.print("  Error rate: ");
      Serial.print(error_rate, 2);
      Serial.println("%");
    }
    
    Serial.println("----------------------------------------\n");
    
    // Blink LED to show activity
    if (changes_delta > 0 || belt_delta > 0) {
      blink_led();
    }
  }
  
  // Small delay to prevent serial buffer overflow
  delay(10);
}

// ============================================================================
// Arduino Framework Entry Points
// ============================================================================

// These are called by the Arduino framework
extern "C" {
  void setup();
  void loop();
}
