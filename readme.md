## DMX512 test project

2025-10-02
add datasheet for Conceptinetics DMX512 shield and driver library
add manual for fixture being used for test.
driver is not implemented at this time.

2025-10-01
rotary encoder is working. Requires loop time of 10mS or less for stability

2025-09-19
add rotary encoder and push button. (untested)
required libraries: Matthias Hertel's
rotary encoder: https://github.com/mathertel/RotaryEncoder
one button: https://github.com/mathertel/OneButton

2025-08-20
It builds in Arduino IDE as an ino, except for unresolved externs for the unlinked I2C functions.
The ++/-- operators for enums are funky and untested.

2025-08-14 initial commit
Right now, this project is just junk.
There is pseudo-code for setting up DMA & timer on an ST-like CPU with a state machine to run it.
And an Arduino-ish project for a state machine to display info on a 4 digit 7-segment display.
The Arduino code assume there exists some kind of rotary encoder with push-button to control it.
But none of the low level code exists. It's just variables and stubbed out functions.
