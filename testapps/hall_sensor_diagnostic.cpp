// Hall Sensor Diagnostic Tool for RP2040
// This firmware reads and outputs hall sensor values in real-time
// Upload this to diagnose hall sensor threshold issues

#include <Arduino.h>
#include "pin_definitions.h"

// Hall sensor pins from pin_definitions.json for rpipicow
// EOL_PIN_R_N = 15 (GP15)
// EOL_PIN_R_S = 14 (GP14)
// EOL_PIN_L_N = 13 (GP13)
// EOL_PIN_L_S = 12 (GP12)

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=== AYAB Hall Sensor Diagnostic ===");
  Serial.println("Platform: RP2040 (Raspberry Pi Pico W)");
  Serial.println("\nHall Sensor Pins:");
  Serial.print("  Right North (EOL_PIN_R_N): GP");
  Serial.println(EOL_PIN_R_N);
  Serial.print("  Right South (EOL_PIN_R_S): GP");
  Serial.println(EOL_PIN_R_S);
  Serial.print("  Left North (EOL_PIN_L_N): GP");
  Serial.println(EOL_PIN_L_N);
  Serial.print("  Left South (EOL_PIN_L_S): GP");
  Serial.println(EOL_PIN_L_S);
  
  // Configure pins as analog inputs
  pinMode(EOL_PIN_R_N, INPUT);
  pinMode(EOL_PIN_R_S, INPUT);
  pinMode(EOL_PIN_L_N, INPUT);
  pinMode(EOL_PIN_L_S, INPUT);
  
  Serial.println("\nADC Resolution: 12-bit (0-4095)");
  Serial.println("RP2040 ADC Reference: 3.3V");
  Serial.println("\nStarting continuous monitoring...");
  Serial.println("Move the carriage slowly across the sensors");
  Serial.println("Format: R_N, R_S, L_N, L_S, [min], [max]");
  Serial.println("----------------------------------------");
  
  delay(1000);
}

void loop() {
  static unsigned long lastPrint = 0;
  static uint16_t minValues[4] = {4095, 4095, 4095, 4095};
  static uint16_t maxValues[4] = {0, 0, 0, 0};
  
  // Read all hall sensors
  uint16_t r_n = analogRead(EOL_PIN_R_N);
  uint16_t r_s = analogRead(EOL_PIN_R_S);
  uint16_t l_n = analogRead(EOL_PIN_L_N);
  uint16_t l_s = analogRead(EOL_PIN_L_S);
  
  // Track min/max values
  if (r_n < minValues[0]) minValues[0] = r_n;
  if (r_n > maxValues[0]) maxValues[0] = r_n;
  if (r_s < minValues[1]) minValues[1] = r_s;
  if (r_s > maxValues[1]) maxValues[1] = r_s;
  if (l_n < minValues[2]) minValues[2] = l_n;
  if (l_n > maxValues[2]) maxValues[2] = l_n;
  if (l_s < minValues[3]) minValues[3] = l_s;
  if (l_s > maxValues[3]) maxValues[3] = l_s;
  
  // Print values every 100ms
  if (millis() - lastPrint > 100) {
    Serial.print(r_n);
    Serial.print(", ");
    Serial.print(r_s);
    Serial.print(", ");
    Serial.print(l_n);
    Serial.print(", ");
    Serial.print(l_s);
    Serial.print("  [min: ");
    Serial.print(minValues[0]);
    Serial.print(",");
    Serial.print(minValues[1]);
    Serial.print(",");
    Serial.print(minValues[2]);
    Serial.print(",");
    Serial.print(minValues[3]);
    Serial.print("] [max: ");
    Serial.print(maxValues[0]);
    Serial.print(",");
    Serial.print(maxValues[1]);
    Serial.print(",");
    Serial.print(maxValues[2]);
    Serial.print(",");
    Serial.print(maxValues[3]);
    Serial.println("]");
    
    lastPrint = millis();
  }
  
  // Reset min/max on button press or every 30 seconds
  static unsigned long lastReset = 0;
  if (millis() - lastReset > 30000) {
    Serial.println("\n--- Resetting min/max values ---\n");
    for (int i = 0; i < 4; i++) {
      minValues[i] = 4095;
      maxValues[i] = 0;
    }
    lastReset = millis();
  }
}
