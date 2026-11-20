## Project Design

## System Overview
The Smart Classroom Comfort Monitor is a C program that runs on a Raspberry Pi. Users enter classroom temperatures manually, and the program determines whether each reading is within the acceptable range.

## Program Structure

## Temperature Input
The program accepts temperature readings from the user and validates the input.

## Temperature Evaluation
Each reading is compared against the configured lower and upper temperature limits.

## Status Display
NORMAL: Temperature is within the acceptable range.
WARNING: Temperature is outside the acceptable range.

## Data Management
The program stores readings in memory, calculates the average temperature, and saves readings to a CSV file.

## Temperature Settings
Users can change the lower and upper temperature limits through the program menu.

## GPIO LED Indicator
The Raspberry Pi uses GPIO pin 17 to control an LED. The LED turns on when the temperature is outside the acceptable range and turns off when the temperature is normal.

## Hardware Design
An LED is connected to GPIO pin 17 through a 220-ohm resistor. The LED is connected to ground to complete the circuit.

## Software Design
The program is written in C and compiled using GCC. The pigpio library is used to control the Raspberry Pi GPIO pin.
