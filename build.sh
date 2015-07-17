#!/bin/bash

g++ -I/usr/local/include -L/usr/local/lib -lwiringPi -o"bwatch-daemon" main.cpp
