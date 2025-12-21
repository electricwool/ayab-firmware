import serial
import time

s = serial.Serial('COM10', 115200, timeout=2)
print('Waiting for data...')
time.sleep(3)
data = s.read(1000)
print(f'Received {len(data)} bytes')
if data:
    print('Data:', data)
s.close()
