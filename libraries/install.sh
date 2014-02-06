#!/bin/bash

#bash script to compile and install shared library

echo
echo "compiling object files"
gcc -c SimpleGPIO.c
gcc -c robotics_cape.c

echo "generating shared lib"
gcc -shared -o librobotics_cape.so SimpleGPIO.o robotics_cape.o

echo "copying to /usr/lib"
cp SimpleGPIO.h /usr/include
cp robotics_cape.h /usr/include
cp librobotics_cape.so /usr/lib

echo "cleaning up files"
rm -r *.o
rm -r *.so

echo
echo "robotics cape library installed"
echo