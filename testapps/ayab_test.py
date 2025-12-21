#!/usr/bin/env python3
"""
AYAB Firmware Test Application
Tests communication with AYAB firmware using the same protocol as ayab-desktop
Outputs detailed logs to a file for debugging
"""

import serial
import serial.tools.list_ports
import sliplib
import time
import sys
from datetime import datetime

# AYAB API Token definitions (from communication.py)
class Token:
    reqInfo = 0x03
    cnfInfo = 0xC3
    reqInit = 0x05
    cnfInit = 0xC5
    reqStart = 0x01
    cnfStart = 0xC1
    indState = 0x84

# CRC8 calculation (from communication.py)
def add_crc(crc, data):
    for i in range(len(data)):
        n = data[i]
        for _j in range(8):
            f = (crc ^ n) & 1
            crc >>= 1
            if f:
                crc ^= 0x8C
            n >>= 1
    return crc & 0xFF

class AYABTester:
    def __init__(self, port_name, log_file="ayab_test_log.txt"):
        self.port_name = port_name
        self.log_file = log_file
        self.ser = None
        self.driver = sliplib.Driver()
        self.rx_msg_list = []
        
        # Open log file
        self.log = open(log_file, 'w')
        self.log_message(f"AYAB Firmware Test - Started at {datetime.now()}")
        self.log_message(f"Port: {port_name}")
        self.log_message("="*70)
        
    def log_message(self, message):
        """Log message to both console and file"""
        print(message)
        self.log.write(message + "\n")
        self.log.flush()
        
    def log_hex(self, label, data):
        """Log data in hex format"""
        hex_str = ' '.join(f'{b:02X}' for b in data)
        self.log_message(f"{label}: [{len(data)} bytes] {hex_str}")
        
    def open_serial(self):
        """Open serial port"""
        try:
            self.log_message(f"\nOpening serial port {self.port_name}...")
            self.ser = serial.Serial(self.port_name, 115200, timeout=0.1, exclusive=True)
            self.log_message("Serial port opened successfully")
            time.sleep(0.5)  # Give device time to initialize
            return True
        except Exception as e:
            self.log_message(f"ERROR: Could not open serial port: {e}")
            return False
            
    def close_serial(self):
        """Close serial port"""
        if self.ser and self.ser.is_open:
            self.ser.close()
            self.log_message("\nSerial port closed")
            
    def send_message(self, data):
        """Send SLIP-encoded message"""
        encoded = self.driver.send(bytes(data))
        self.log_hex("TX", data)
        self.log_hex("TX (SLIP encoded)", encoded)
        self.ser.write(encoded)
        
    def read_message(self, timeout=1.0):
        """Read SLIP-decoded message with timeout"""
        start_time = time.time()
        
        while time.time() - start_time < timeout:
            # Check if we have buffered messages
            if len(self.rx_msg_list) > 0:
                msg = self.rx_msg_list.pop(0)
                self.log_hex("RX", msg)
                return msg
                
            # Try to read from serial
            if self.ser.in_waiting > 0:
                data = self.ser.read(self.ser.in_waiting)
                self.log_hex("RX (SLIP encoded)", data)
                # Decode SLIP and add to buffer
                messages = self.driver.receive(data)
                self.rx_msg_list.extend(messages)
            else:
                time.sleep(0.01)  # Small delay to avoid busy waiting
                
        return None
        
    def test_req_info(self):
        """Test requestInfo command"""
        self.log_message("\n" + "="*70)
        self.log_message("TEST: requestInfo")
        self.log_message("="*70)
        
        # Send requestInfo (just token, no CRC)
        self.send_message([Token.reqInfo])
        
        # Wait for confirmInfo response
        msg = self.read_message(timeout=2.0)
        
        if msg is None:
            self.log_message("ERROR: No response received (TIMEOUT)")
            return False
            
        if len(msg) < 2:
            self.log_message(f"ERROR: Response too short ({len(msg)} bytes)")
            return False
            
        if msg[0] != Token.cnfInfo:
            self.log_message(f"ERROR: Wrong response token: 0x{msg[0]:02X} (expected 0x{Token.cnfInfo:02X})")
            return False
            
        # Parse confirmInfo response
        api_version = msg[1]
        fw_major = msg[2] if len(msg) > 2 else 0
        fw_minor = msg[3] if len(msg) > 3 else 0
        fw_patch = msg[4] if len(msg) > 4 else 0
        fw_suffix = msg[5:21].decode('ascii', errors='ignore').rstrip('\x00') if len(msg) > 5 else ""
        
        self.log_message(f"SUCCESS: Received confirmInfo")
        self.log_message(f"  API Version: {api_version}")
        self.log_message(f"  Firmware Version: {fw_major}.{fw_minor}.{fw_patch}{fw_suffix}")
        
        return True
        
    def test_req_init(self, machine_type=0):
        """Test requestInit command"""
        self.log_message("\n" + "="*70)
        self.log_message(f"TEST: requestInit (machine type {machine_type})")
        self.log_message("="*70)
        
        # Build requestInit message
        data = bytearray([Token.reqInit, machine_type])
        crc = add_crc(0, data)
        data.append(crc)
        
        self.send_message(data)
        
        # Wait for confirmInit response
        msg = self.read_message(timeout=2.0)
        
        if msg is None:
            self.log_message("ERROR: No response received (TIMEOUT)")
            return False
            
        if len(msg) < 2:
            self.log_message(f"ERROR: Response too short ({len(msg)} bytes)")
            return False
            
        if msg[0] != Token.cnfInit:
            self.log_message(f"ERROR: Wrong response token: 0x{msg[0]:02X} (expected 0x{Token.cnfInit:02X})")
            return False
            
        error_code = msg[1]
        
        if error_code == 0:
            self.log_message(f"SUCCESS: Received confirmInit (no error)")
            return True
        else:
            self.log_message(f"ERROR: confirmInit returned error code: {error_code}")
            return False
            
    def run_tests(self):
        """Run all tests"""
        self.log_message("\n" + "="*70)
        self.log_message("STARTING AYAB FIRMWARE TESTS")
        self.log_message("="*70)
        
        if not self.open_serial():
            return False
            
        try:
            # Test 1: requestInfo
            if not self.test_req_info():
                self.log_message("\nTest FAILED: requestInfo")
                return False
                
            # Test 2: requestInit
            if not self.test_req_init(machine_type=0):
                self.log_message("\nTest FAILED: requestInit")
                return False
                
            self.log_message("\n" + "="*70)
            self.log_message("ALL TESTS PASSED!")
            self.log_message("="*70)
            return True
            
        except Exception as e:
            self.log_message(f"\nEXCEPTION: {e}")
            import traceback
            self.log_message(traceback.format_exc())
            return False
        finally:
            self.close_serial()
            self.log.close()

def list_serial_ports():
    """List available serial ports"""
    ports = serial.tools.list_ports.comports()
    print("\nAvailable serial ports:")
    for i, port in enumerate(ports):
        print(f"  {i+1}. {port.device} - {port.description}")
    return ports

def main():
    print("AYAB Firmware Test Application")
    print("="*70)
    
    # List available ports
    ports = list_serial_ports()
    
    if not ports:
        print("\nNo serial ports found!")
        return
        
    # Get port selection
    if len(sys.argv) > 1:
        port_name = sys.argv[1]
    else:
        try:
            selection = int(input("\nSelect port number (or press Enter for port 1): ") or "1")
            port_name = ports[selection - 1].device
        except (ValueError, IndexError):
            print("Invalid selection")
            return
            
    # Run tests
    tester = AYABTester(port_name)
    success = tester.run_tests()
    
    print(f"\nTest log saved to: {tester.log_file}")
    
    if success:
        print("\n✓ All tests passed!")
        sys.exit(0)
    else:
        print("\n✗ Tests failed - check log file for details")
        sys.exit(1)

if __name__ == "__main__":
    main()
