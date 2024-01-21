import serial
import time
 
port = serial.Serial(
    "/dev/ttyUSB0",
    baudrate=115200,
    rtscts=True,
    timeout=0)
 
if port.isOpen():
    print('opened')
else:
    print('open failed')
 
packet = bytearray()
packet.append(0x1B)
port.rts = False # This will pull the voltage high
 
while True:
    for i in range(0, 100):
        port.write(packet)
        time.sleep(0.01)
    data = port.read_all()
    print(data)
    if len(data) == 0 or data == b'enter main\r\n':
        print('.')
        continue
    else:
        break
 
print('pass')
 
while True:
    data = port.read_all()
    print(data)
    time.sleep(1)