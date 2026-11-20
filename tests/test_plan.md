## Test Plan

## Purpose
This test plan checks whether the Smart Classroom Comfort Monitor correctly evaluates temperatures, calculates averages, saves readings, and controls the warning LED.

## Test Cases
Test 1: Normal Temperature
Input: 72°F
Expected Result: NORMAL, LED OFF.

Test 2: Lower Boundary
Input: 68°F
Expected Result: NORMAL, LED OFF.

Test 3: Upper Boundary
Input: 76°F
Expected Result: NORMAL, LED OFF.

Test 4: Low Temperature Warning
Input: 65°F
Expected Result: WARNING, LED ON.

Test 5: High Temperature Warning
Input: 80°F
Expected Result: WARNING, LED ON.

Test 6: Invalid Input
Input: Text instead of a number.
Expected Result: Input rejected.

Test 7: Custom Temperature Range
Change range to 65–75°F and enter 70°F.
Expected Result: NORMAL.

Test 8: Saved Readings
Use the program to view previous readings.
Expected Result: Previously saved readings are displayed.

## Test Results
Results will be recorded after running each test on the Raspberry Pi.
