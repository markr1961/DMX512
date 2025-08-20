## DMX512 test project

2025-08-20
It builds in Arduino IDE as an ino, except for unresolved externs for the unlinked I2C functions.
The ++/-- operators for enums are funky and untested.

initial commit
Right now, this project is just junk.
There is a pseudo-code for setting up DMA & timer on an ST-like CPU with a state machine to run it.
And an Arduino-ish project for state a state machine to display info on a 4 digit 7-segment display.
The Arduino code assume there exists some kind of rotary encode with push-button to control it.
But none of the low level code exists. It's just variables and stubbed out functions.
