import serial
import time
import subprocess
ret=subprocess.run("ls /dev/ttyUSB*",shell=True,stdout=subprocess.PIPE)
ret = ret.stdout.decode("ascii").split('\n')[:-1]
if len(ret)<1:
    print("Error: Could not find usb serial port\n")
else:
    print("Resetting: Using"+ret[0]+"\n")
    PORT = ret[0]
    BAUD = 115200
    port = serial.serial_for_url(PORT)
    port.baudrate=115200
    port.dtr=False
    time.sleep(0.1)
    port.dtr=True

