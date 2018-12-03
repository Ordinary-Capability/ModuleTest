#/usr/bin/env python



import os
import sys
import time

import serial

config_args = [
                [57600, 8, 'N', 1],
                [9600, 8, 'N', 1],
                [19200, 8, 'N', 1],
                [38400, 8, 'N', 1],
                [115200, 8, 'N', 1],
                [115200, 7, 'N', 1],
                [115200, 8, 'E', 1],
                [115200, 8, 'O', 1],
                [115200, 8, 'N', 2],
                ]
                
bytes_pattern = bytearray(1024)
def init_bytes_pattern(data_bits):
    global bytes_pattern
    for i in range(1024):
        if data_bits==8:
            bytes_pattern[i] = (i%256)
        if data_bits==7:
            bytes_pattern[i] = (i%256) & 0x7f
        if data_bits==6:
            bytes_pattern[i] = (i%256) & 0x3f
        if data_bits==5:
            bytes_pattern[i] = (i%256) & 0x1f

if __name__ == "__main__":
    port = sys.argv[1]
    

    for config in config_args:
        print ("Test UART with config {}".format(config))
        init_bytes_pattern(config[1])
        s = serial.serial_for_url(port, *config, timeout=5)
        if not s.isOpen:
            print ("Open %s fail."%port)
            sys.exit(-1)
        s.write(bytes_pattern)
        r = s.read(1024)
        if not r:
            print ("Recieve data from %s fail. \nUART test Fail."%port)
            s.close()
            sys.exit(-1)
        if r != bytes_pattern:
            print (repr(r))
            print (len(r))
            print ("Recieved data does not match to pattern data.\n UART test fail.")
            s.close()
            sys.exit(-1)
        
        time.sleep(3) #wait peer to configure and re-open uart               

        s.close()



