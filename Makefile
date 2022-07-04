MCU=atmega4809
AVRDUDEMCU=$(MCU)
TARGET=z80interface
OBJS=twi.o mcp23017.o
DEVICE=pickit4_updi
PORT=usb

F_CPU=16000000UL

include Makefile.all
