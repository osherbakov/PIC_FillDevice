# MPLAB IDE generated this makefile for use with GNU make.
# Project: Pic_FillDevice.mcp
# Date: Sun Mar 27 21:48:09 2011

AS = MPASMWIN.exe
CC = mcc18.exe
LD = mplink.exe
AR = mplib.exe
RM = rm

Pic_FillDevice.cof : controls.o FIll_main.o hw_setup.o isr.o delay.o spi_eeprom.o i2c_sw.o rtc.o TxFill.o crcmodel.o hqii.o RxFill.o gps.o serial.o
	$(LD) /p18F43K22 /l"C:\MCC18\lib" "controls.o" "FIll_main.o" "hw_setup.o" "isr.o" "delay.o" "spi_eeprom.o" "i2c_sw.o" "rtc.o" "TxFill.o" "crcmodel.o" "hqii.o" "RxFill.o" "gps.o" "serial.o" /u_CRUNTIME /u_EXTENDEDMODE /u_DEBUG /z__MPLAB_BUILD=1 /z__MPLAB_DEBUG=1 /z__MPLAB_DEBUGGER_PK3=1 /z__ICD2RAM=1 /m"Pic_FillDevice.map" /w /o"Pic_FillDevice.cof"

controls.o : controls.c config.h C:/MCC18/h/p18cxxx.h C:/MCC18/h/p18f43k22.h controls.h delay/delay.h C:/MCC18/h/delays.h
	$(CC) -p=18F43K22 /i".\serial" -I".\gps" -I".\i2c_sw" -I".\ds102_fill" -I".\eeprom" -I".\crc_calc" -I".\rtc" -I".\delay" "controls.c" -fo="controls.o" -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 --extended

FIll_main.o : FIll_main.c config.h C:/MCC18/h/p18cxxx.h C:/MCC18/h/p18f43k22.h delay/delay.h C:/MCC18/h/delays.h controls.h eeprom/spi_eeprom.h rtc/rtc.h gps/gps.h serial/serial.h ds102_fill/Fill.h
	$(CC) -p=18F43K22 /i".\serial" -I".\gps" -I".\i2c_sw" -I".\ds102_fill" -I".\eeprom" -I".\crc_calc" -I".\rtc" -I".\delay" "FIll_main.c" -fo="FIll_main.o" -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 --extended

hw_setup.o : hw_setup.c config.h C:/MCC18/h/p18cxxx.h C:/MCC18/h/p18f43k22.h delay/delay.h C:/MCC18/h/delays.h controls.h rtc/rtc.h C:/MCC18/h/spi.h C:/MCC18/h/pconfig.h i2c_sw/i2c_sw.h serial/serial.h
	$(CC) -p=18F43K22 /i".\serial" -I".\gps" -I".\i2c_sw" -I".\ds102_fill" -I".\eeprom" -I".\crc_calc" -I".\rtc" -I".\delay" "hw_setup.c" -fo="hw_setup.o" -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 --extended

isr.o : isr.c config.h C:/MCC18/h/p18cxxx.h C:/MCC18/h/p18f43k22.h delay/delay.h C:/MCC18/h/delays.h controls.h rtc/rtc.h serial/serial.h
	$(CC) -p=18F43K22 /i".\serial" -I".\gps" -I".\i2c_sw" -I".\ds102_fill" -I".\eeprom" -I".\crc_calc" -I".\rtc" -I".\delay" "isr.c" -fo="isr.o" -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 --extended

delay.o : delay/delay.c config.h C:/MCC18/h/p18cxxx.h C:/MCC18/h/p18f43k22.h delay/delay.h C:/MCC18/h/delays.h
	$(CC) -p=18F43K22 /i".\serial" -I".\gps" -I".\i2c_sw" -I".\ds102_fill" -I".\eeprom" -I".\crc_calc" -I".\rtc" -I".\delay" "delay\delay.c" -fo="delay.o" -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 --extended

spi_eeprom.o : eeprom/spi_eeprom.c config.h C:/MCC18/h/p18cxxx.h C:/MCC18/h/p18f43k22.h delay/delay.h C:/MCC18/h/delays.h controls.h eeprom/spi_eeprom.h C:/MCC18/h/spi.h C:/MCC18/h/pconfig.h
	$(CC) -p=18F43K22 /i".\serial" -I".\gps" -I".\i2c_sw" -I".\ds102_fill" -I".\eeprom" -I".\crc_calc" -I".\rtc" -I".\delay" "eeprom\spi_eeprom.c" -fo="spi_eeprom.o" -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 --extended

i2c_sw.o : i2c_sw/i2c_sw.c config.h C:/MCC18/h/p18cxxx.h C:/MCC18/h/p18f43k22.h delay/delay.h C:/MCC18/h/delays.h i2c_sw/i2c_sw.h
	$(CC) -p=18F43K22 /i".\serial" -I".\gps" -I".\i2c_sw" -I".\ds102_fill" -I".\eeprom" -I".\crc_calc" -I".\rtc" -I".\delay" "i2c_sw\i2c_sw.c" -fo="i2c_sw.o" -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 --extended

rtc.o : rtc/rtc.c config.h C:/MCC18/h/p18cxxx.h C:/MCC18/h/p18f43k22.h rtc/rtc.h i2c_sw/i2c_sw.h
	$(CC) -p=18F43K22 /i".\serial" -I".\gps" -I".\i2c_sw" -I".\ds102_fill" -I".\eeprom" -I".\crc_calc" -I".\rtc" -I".\delay" "rtc\rtc.c" -fo="rtc.o" -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 --extended

TxFill.o : ds102_fill/TxFill.c config.h C:/MCC18/h/p18cxxx.h C:/MCC18/h/p18f43k22.h crc_calc/crcmodel.h delay/delay.h C:/MCC18/h/delays.h rtc/rtc.h eeprom/spi_eeprom.h serial/serial.h ds102_fill/Fill.h
	$(CC) -p=18F43K22 /i".\serial" -I".\gps" -I".\i2c_sw" -I".\ds102_fill" -I".\eeprom" -I".\crc_calc" -I".\rtc" -I".\delay" "ds102_fill\TxFill.c" -fo="TxFill.o" -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 --extended

crcmodel.o : crc_calc/crcmodel.c crc_calc/crcmodel.h
	$(CC) -p=18F43K22 /i".\serial" -I".\gps" -I".\i2c_sw" -I".\ds102_fill" -I".\eeprom" -I".\crc_calc" -I".\rtc" -I".\delay" "crc_calc\crcmodel.c" -fo="crcmodel.o" -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 --extended

hqii.o : havequick/hqii.c config.h C:/MCC18/h/p18cxxx.h C:/MCC18/h/p18f43k22.h delay/delay.h C:/MCC18/h/delays.h controls.h rtc/rtc.h i2c_sw/i2c_sw.h
	$(CC) -p=18F43K22 /i".\serial" -I".\gps" -I".\i2c_sw" -I".\ds102_fill" -I".\eeprom" -I".\crc_calc" -I".\rtc" -I".\delay" "havequick\hqii.c" -fo="hqii.o" -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 --extended

RxFill.o : ds102_fill/RxFill.c config.h C:/MCC18/h/p18cxxx.h C:/MCC18/h/p18f43k22.h crc_calc/crcmodel.h delay/delay.h C:/MCC18/h/delays.h rtc/rtc.h eeprom/spi_eeprom.h serial/serial.h ds102_fill/Fill.h
	$(CC) -p=18F43K22 /i".\serial" -I".\gps" -I".\i2c_sw" -I".\ds102_fill" -I".\eeprom" -I".\crc_calc" -I".\rtc" -I".\delay" "ds102_fill\RxFill.c" -fo="RxFill.o" -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 --extended

gps.o : gps/gps.c config.h C:/MCC18/h/p18cxxx.h C:/MCC18/h/p18f43k22.h gps/gps.h rtc/rtc.h delay/delay.h C:/MCC18/h/delays.h i2c_sw/i2c_sw.h
	$(CC) -p=18F43K22 /i".\serial" -I".\gps" -I".\i2c_sw" -I".\ds102_fill" -I".\eeprom" -I".\crc_calc" -I".\rtc" -I".\delay" "gps\gps.c" -fo="gps.o" -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 --extended

serial.o : serial/serial.c config.h C:/MCC18/h/p18cxxx.h C:/MCC18/h/p18f43k22.h delay/delay.h C:/MCC18/h/delays.h controls.h ds102_fill/Fill.h serial/serial.h gps/gps.h
	$(CC) -p=18F43K22 /i".\serial" -I".\gps" -I".\i2c_sw" -I".\ds102_fill" -I".\eeprom" -I".\crc_calc" -I".\rtc" -I".\delay" "serial\serial.c" -fo="serial.o" -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 --extended

clean : 
	$(RM) "controls.o" "FIll_main.o" "hw_setup.o" "isr.o" "delay.o" "spi_eeprom.o" "i2c_sw.o" "rtc.o" "TxFill.o" "crcmodel.o" "hqii.o" "RxFill.o" "gps.o" "serial.o" "Pic_FillDevice.cof" "Pic_FillDevice.hex" "Pic_FillDevice.map"

