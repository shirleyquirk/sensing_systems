import serial
import time
PORT = "/dev/ttyUSB0"
BAUD = 115200
port = serial.serial_for_url(PORT)
port.baudrate=115200
port.dtr=False
time.sleep(0.1)
port.dtr=True
port.close()

