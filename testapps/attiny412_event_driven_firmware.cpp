// ATtiny412 Voltage Detector Firmware - Event-Driven
// Compile with megaTinyCore: https://github.com/SpenceKonde/megaTinyCore
// Only transmits when sensor states change (asynchronous communication)

#include <SoftwareSerial.h>

// Pin definitions for ATtiny412
#define SENSOR1_PIN PIN_PA6  // AIN6, Pin 3
#define SENSOR2_PIN PIN_PA7  // AIN7, Pin 4
#define TX_PIN PIN_PA1       // Serial TX, Pin 6

// Voltage thresholds (ADC values for 3.3V reference)
// ATtiny412 has 10-bit ADC: ADC = (Vin / Vref) * 1023
#define THRESHOLD_LOW_INACTIVE  310   // ~1.0V threshold (1.0/3.3 * 1023)
#define THRESHOLD_INACTIVE_HIGH 768   // ~2.5V threshold (2.5/3.3 * 1023)

// Hysteresis to prevent noise-induced state changes
#define HYSTERESIS 20  // ~65mV hysteresis

// State definitions
#define STATE_LOW      0b00
#define STATE_INACTIVE 0b01
#define STATE_HIGH     0b10
#define STATE_ERROR    0b11

// Debounce settings
#define DEBOUNCE_SAMPLES 5    // Number of consistent readings required
#define SAMPLE_INTERVAL_MS 2  // Time between samples

SoftwareSerial serial(255, TX_PIN); // RX unused, TX on PA1

// Previous states for change detection
uint8_t prevSensor1State = STATE_ERROR;
uint8_t prevSensor2State = STATE_ERROR;

// Debounce counters
uint8_t sensor1DebounceCount = 0;
uint8_t sensor2DebounceCount = 0;
uint8_t sensor1PendingState = STATE_ERROR;
uint8_t sensor2PendingState = STATE_ERROR;

void setup() {
  // Configure ADC reference to VCC (3.3V)
  analogReference(VDD);
  
  // Initialize serial at 9600 baud
  serial.begin(9600);
  
  // Small delay for stability
  delay(100);
  
  // Read initial states
  prevSensor1State = readSensorState(SENSOR1_PIN, STATE_ERROR);
  prevSensor2State = readSensorState(SENSOR2_PIN, STATE_ERROR);
  
  // Send initial state
  sendStateUpdate(prevSensor1State, prevSensor2State);
}

uint8_t readSensorState(uint8_t pin, uint8_t currentState) {
  int adcValue = analogRead(pin);
  
  // Apply hysteresis based on current state
  if (currentState == STATE_LOW) {
    // Need to exceed threshold + hysteresis to transition up
    if (adcValue < THRESHOLD_LOW_INACTIVE + HYSTERESIS) {
      return STATE_LOW;
    } else if (adcValue < THRESHOLD_INACTIVE_HIGH) {
      return STATE_INACTIVE;
    } else {
      return STATE_HIGH;
    }
  } else if (currentState == STATE_INACTIVE) {
    // Apply hysteresis on both boundaries
    if (adcValue < THRESHOLD_LOW_INACTIVE - HYSTERESIS) {
      return STATE_LOW;
    } else if (adcValue < THRESHOLD_INACTIVE_HIGH + HYSTERESIS) {
      return STATE_INACTIVE;
    } else {
      return STATE_HIGH;
    }
  } else if (currentState == STATE_HIGH) {
    // Need to drop below threshold - hysteresis to transition down
    if (adcValue < THRESHOLD_LOW_INACTIVE) {
      return STATE_LOW;
    } else if (adcValue < THRESHOLD_INACTIVE_HIGH - HYSTERESIS) {
      return STATE_INACTIVE;
    } else {
      return STATE_HIGH;
    }
  } else {
    // First reading (no hysteresis)
    if (adcValue < THRESHOLD_LOW_INACTIVE) {
      return STATE_LOW;
    } else if (adcValue < THRESHOLD_INACTIVE_HIGH) {
      return STATE_INACTIVE;
    } else {
      return STATE_HIGH;
    }
  }
}

void sendStateUpdate(uint8_t sensor1State, uint8_t sensor2State) {
  // Pack into single byte
  uint8_t statusByte = (sensor1State << 6) | (sensor2State << 4);
  
  // Send via serial (asynchronous - only when state changes)
  serial.write(statusByte);
  
  // Optional: Send twice for reliability
  delay(5);
  serial.write(statusByte);
}

void loop() {
  // Read current sensor states
  uint8_t sensor1State = readSensorState(SENSOR1_PIN, prevSensor1State);
  uint8_t sensor2State = readSensorState(SENSOR2_PIN, prevSensor2State);
  
  // Debounce Sensor 1
  if (sensor1State != prevSensor1State) {
    if (sensor1State == sensor1PendingState) {
      sensor1DebounceCount++;
      if (sensor1DebounceCount >= DEBOUNCE_SAMPLES) {
        // State change confirmed
        prevSensor1State = sensor1State;
        sensor1DebounceCount = 0;
        
        // Send update only if state changed
        sendStateUpdate(prevSensor1State, prevSensor2State);
      }
    } else {
      // New pending state
      sensor1PendingState = sensor1State;
      sensor1DebounceCount = 1;
    }
  } else {
    // State stable, reset debounce
    sensor1DebounceCount = 0;
    sensor1PendingState = sensor1State;
  }
  
  // Debounce Sensor 2
  if (sensor2State != prevSensor2State) {
    if (sensor2State == sensor2PendingState) {
      sensor2DebounceCount++;
      if (sensor2DebounceCount >= DEBOUNCE_SAMPLES) {
        // State change confirmed
        prevSensor2State = sensor2State;
        sensor2DebounceCount = 0;
        
        // Send update only if state changed
        sendStateUpdate(prevSensor1State, prevSensor2State);
      }
    } else {
      // New pending state
      sensor2PendingState = sensor2State;
      sensor2DebounceCount = 1;
    }
  } else {
    // State stable, reset debounce
    sensor2DebounceCount = 0;
    sensor2PendingState = sensor2State;
  }
  
  // Sample at regular intervals
  delay(SAMPLE_INTERVAL_MS);
}
