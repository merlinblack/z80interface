#!/usr/bin/env python

import serial
import time

def send(ser: serial.Serial, line):
  xoff = False
  byteIn = 0
  for byteOut in line:
    while ser.in_waiting:
      byteIn = ser.read(1)
      if byteIn == b'\x13':
        xoff = True
      else:
        if byteIn == b'\x11':
          xoff = False
        else:
          print(byteIn.decode(), end='')

    if xoff:
      while byteIn != b'\x11':
        byteIn = ser.read(1)
        if byteIn != b'\x11':
          print(byteIn.decode(), end='')
      xoff = False

    ser.write(byteOut.encode())

def main():
  ser = serial.Serial('/dev/ttyACM0', 115200, timeout=10, xonxoff=False)
  while (ser.in_waiting):
    print(ser.readline().decode())

  with open('z80interface.c', 'r') as f:
    for line in f:
      send(ser, line)

  time.sleep(.1)
  while ser.in_waiting:
    tail = ser.readline().decode()
    print(tail, end='')
    time.sleep(.1)

  print('\nDone')

if __name__ == '__main__':
  main()
