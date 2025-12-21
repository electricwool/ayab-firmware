// Simple Serial Test for RP2040
// Upload this to test if basic Serial communication works
// This bypasses PacketSerial to isolate the issue

#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(1000); // Wait for USB to initialize
}

void loop() {
  // Echo back any received data
  if (Serial.available()) {
    int data = Serial.read();
    Serial.write(data); // Echo it back
    Serial.write('!');  // Add exclamation mark
  }
  
  // Send a heartbeat every second
  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat > 1000) {
    Serial.println("HEARTBEAT");
    lastHeartbeat = millis();
  }
}
