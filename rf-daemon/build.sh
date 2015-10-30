#!/bin/bash

g++ -Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s -Wall -I./RF24/RPi/RF24/ -lrf24-bcm -I/usr/local/include -L/usr/local/lib -lwiringPi -o"rf-daemon" main.cpp
