// ATtiny412 Voltage Detector - Simple Event-Driven Version
// Compile with megaTinyCore: https://github.com/SpenceKonde/megaTinyCore
// Transmits only on state changes (asynchronous serial communication)

#include <SoftwareSerial.h>

// Pin definitions for ATtiny412
#define SENSOR1_PIN PIN_PA6  // AIN6, Pin 3
#define SENSOR2_PIN PIN_PA7  // AIN7, Pin 4
#define TX_PIN PIN_PA1       // Serial TX, Pin 6

// Voltage thresholds (ADC values for 3.3V reference)
// ATtiny412 has 10-bit ADC: ADC = (Vin / Vref) * 1023
#define THRESHOLD_LOW_INACTIVE  310   // ~1.0V threshold (1.0/3.3 * 1023)
#define THRESHOLD_INACTIVE_HIGH 768   // ~2.5V threshold (2.5/3.3 * 1023)

// State definitions
#define STATE_LOW      0b00
#define STATE_INACTIVE 0b01
#define STATE_HIGH     0b10

SoftwareSerial serial(255, TX_PIN); // RX unused, TX on PA1

// Previous states for change detection
uint8_t prevSensor1State = 0xFF;  // Invalid initial state
uint8_t prevSensor2State = 0xFF;

void setup() {
  // Configure ADC reference to VCC (3.3V)
  analogReference(VDD);
  
  // Initialize serial at 9600 baud
  serial.begin(9600);
  
  // Small delay for stability
  delay(100);
  
  // Read and send initial state
  uint8_t s1 = readSensorState(SENSOR1_PIN);
  uint8_t s2 = readSensorState(SENSOR2_PIN);
  
  prevSensor1State = s1;
  prevSensor2State = s2;
  
  sendStateUpdate(s1, s2);
}

uint8_t readSensorState(uint8_t pin) {
  int adcValue = analogRead(pin);
  
  if (adcValue < THRESHOLD_LOW_INACTIVE) {
    return STATE_LOW;
  } else if (adcValue < THRESHOLD_INACTIVE_HIGH) {
    return STATE_INACTIVE;
  } else {
    return STATE_HIGH;
  }
}

void sendStateUpdate(uint8_t sensor1State, uint8_t sensor2State) {
  // Pack into single byte
  uint8_t statusByte = (sensor1State << 6) | (sensor2State << 4);
  
  // Send via serial (asynchronous - only when state changes)
  serial.write(statusByte);
  
  // Send twice for reliability
  delay(5);
  serial.write(statusByte);
}

void loop() {
  // Read current sensor states
  uint8_t sensor1State = readSensorState(SENSOR1_PIN);
  uint8_t sensor2State = readSensorState(SENSOR2_PIN);
  
  // Check if either sensor state changed
  if (sensor1State != prevSensor1State || sensor2State != prevSensor2State) {
    // Debounce: wait and re-read to confirm
    delay(10);
    sensor1State = readSensorState(SENSOR1_PIN);
    sensor2State = readSensorState(SENSOR2_PIN);
    
    // Confirm state change
    if (sensor1State != prevSensor1State || sensor2State != prevSensor2State) {
      // Update stored states
      prevSensor1State = sensor1State;
      prevSensor2State = sensor2State;
      
      // Send update via serial (asynchronous)
      sendStateUpdate(sensor1State, sensor2State);
    }
  }
  
  // Check sensors every 10ms
  delay(10);
}
