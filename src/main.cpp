#include <Arduino.h>

#include "knitter.h"
#include "platform.h"
#include "pin_definitions.h"
#include "debug.h"

Knitter *knitter;

/*
 * Setup - do once before going to the main loop.
 */
void setup() {
  DEBUG_INIT();
  DEBUG_PRINTLN("\n\n=== AYAB Firmware Starting ===");
  DEBUG_PRINTLN("Platform: RP2040 (Raspberry Pi Pico W)");
  
  // Print pin configuration
  DEBUG_PRINTLN("\nPin Configuration:");
  DEBUG_PRINT_INT("  PIEZO_PIN: ", PIEZO_PIN);
  DEBUG_PRINT_INT("  ENC_A_PIN: ", ENC_A_PIN);
  DEBUG_PRINT_INT("  ENC_B_PIN: ", ENC_B_PIN);
  DEBUG_PRINT_INT("  ENC_C_PIN: ", ENC_C_PIN);
  DEBUG_PRINT_INT("  MCP_SDA_PIN: ", MCP_SDA_PIN);
  DEBUG_PRINT_INT("  MCP_SCL_PIN: ", MCP_SCL_PIN);
  DEBUG_PRINT_INT("  EOL_PIN_R_N: ", EOL_PIN_R_N);
  DEBUG_PRINT_INT("  EOL_PIN_R_S: ", EOL_PIN_R_S);
  DEBUG_PRINT_INT("  EOL_PIN_L_N: ", EOL_PIN_L_N);
  DEBUG_PRINT_INT("  EOL_PIN_L_S: ", EOL_PIN_L_S);
  
  DEBUG_PRINTLN("\nInitializing HAL...");
  
  // Create Hardware Abstraction Layer (HAL) instance
  hardwareAbstraction::Platform *hal = new hardwareAbstraction::Platform();
  
  DEBUG_PRINTLN("HAL initialized");
  DEBUG_PRINTLN("Creating Knitter instance...");
  
  // Main knitter instance
  knitter = new Knitter(hal);
  
  DEBUG_PRINTLN("Knitter initialized");
  DEBUG_PRINTLN("=== Setup Complete ===\n");
}

/*
 * Main Loop - repeat forever.
 */
void loop() {
  // Schedule knitter event loop
  knitter->schedule();
}
