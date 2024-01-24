import sys
import serial
import time
from kbhit import KBHit
import os

port = None

def init_port():
    global port
    port = serial.Serial(
        "/dev/ttyUSB0",
        baudrate=115200,
        rtscts=True,
        timeout=0)

def deinit_port():
    global port
    if port:
        port.close()
    port = None

def do_flash():
    return os.system("make flash")

def do_erase():
    return os.system("make erase")

def do_reset():
    port.rts = True
    time.sleep(0.2)
    port.rts = False

def monitor():
    init_port()
    
    if port.isOpen():
        print('monitor opened')
    else:
        print('open failed')
    
    kb = KBHit()
    mode = False
    buf = bytearray()
    
    try:
        do_reset()
        while True:
            data = port.read_all()
            print(data.decode(errors='ignore'), end='', flush=True)
            if kb.kbhit():
                c = kb.getch()
                d = ord(c)
                if mode:
                    mode = False
                    if d == 6:
                        print('do flash')
                        deinit_port()
                        r = do_flash()
                        init_port()
                        if r == 0:
                            do_reset()
                        else:
                            print("flash failed")
                    if d == 18:
                        print('do reset')
                        do_reset()
                    if d == 22:
                        print('do erase')
                        do_erase()
                    if d == 23:
                        print('do quit')
                        break
                    continue
                # Ctrl + T
                if d == 20:
                    mode = True
                    print('command mode')
                    print('\tCtrl + W\tquit')
                    print('\tCtrl + F\tflash')
                    print('\tCtrl + R\treset')
                    print('\tCtrl + V\terase')
                    continue
                if d == 127:
                    print('\b ', end="", flush=True) 
                    sys.stdout.write('\010')
                    port.write(c.encode())
                    continue
                print(c, end='', flush=True)
                buf += c.encode()
                if c == '\n':
                    port.write(buf)
                    buf = bytearray()
            time.sleep(0.001)
    except KeyboardInterrupt:
        print('exit')
    except OSError:
        print('serial disconnected, waitting re-connect')
        while True:
            time.sleep(1)
            print('.', end='', flush=True)
            try:
                init_port()
            except serial.serialutil.SerialException:
                continue
            finally:
                deinit_port()
            break
        monitor()
    finally:
        kb.set_normal_term()

if __name__ == '__main__':
    monitor()