##Smart Classroom Comfort Monitor

## Project Description
The Smart Classroom Comfort Monitor is a Raspberry Pi-based program that allows users to enter classroom temperatures and determine whether the room is within an acceptable comfort range.

## Features
Manual temperature entry in Fahrenheit
Default acceptable range of 68°F to 76°F
NORMAL and WARNING status indicators
Temperature average calculation
Temperature reading history
Configurable temperature limits
CSV data logging
Raspberry Pi GPIO LED warning indicator

## Hardware
Raspberry Pi 4 Model B
Breadboard
LED
220-ohm resistor
Jumper wires

## Compilation
Run from the project directory:
gcc -std=c11 -Wall -Wextra -pedantic -DUSE_RPI_GPIO src/main.c -o comfort_monitor -lpigpio -lrt -lpthread

## Execution
sudo ./comfort_monitor
