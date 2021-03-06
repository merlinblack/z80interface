#!/usr/bin/env python

import serial
import time

def send(ser: serial.Serial, line):
  ret = '';
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
          ret = ret + byteIn.decode('ascii', errors='ignore');

    if xoff:
      while byteIn != b'\x11':
        byteIn = ser.read(1)
        if byteIn != b'\x11':
          ret = ret + byteIn.decode('ascii', errors='ignore');
      xoff = False

    ser.write(byteOut.encode('ascii'))

  return ret

def main():
  ser = serial.Serial('/dev/ttyACM0', 115200, timeout=10, xonxoff=False)
  while (ser.in_waiting):
    print(ser.readline().decode('ascii', errors='ignore'))

  with open('ramimage.hex', 'r') as f:
    print(send(ser, 'Nodisp\r'), end='')
    print(send(ser, 'Load 0000\r'), end='')
    for line in f:
      print(send(ser, line), end='')

  print(send(ser, 'Dump 8000\r'), end='')
  time.sleep(1)
  while ser.in_waiting:
    tail = ser.readline().decode('ascii', errors='ignore')
    print(tail, end='')
    time.sleep(.1)

  print('\nDone')

if __name__ == '__main__':
  main()
