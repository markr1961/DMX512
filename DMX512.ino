/*
## DMX512 Project

### 4 digit display
Display toggles between being used to select the operating mode,
and showing 1 digit mode and 3 digits 0-255 value.

####  mode + value
'mode' is 1st digit: 'A' 'r' 'g' 'b' 'P':
'P' - program address: sets DMX start address, allowed: 1-510
'r' - red level
'g' - green level
'b' - blue level
'I' - intensity mode: increments existing RGB values.
'A' - all: sets all 512 channels to value selected
'X' - color wheel

#### button function
selects between mode setting and level setting.

Requires Matthias Hertel's libraries for
rotary encoder: https://github.com/mathertel/RotaryEncoder
one button: https://github.com/mathertel/OneButton
state of the rotary encoder is checked in the loop() function.
A double tap on the switch toggles the built-in LED.

*/

#include <Arduino.h>
#include <RotaryEncoder.h>
#include <OneButton.h>

// Hardware setup:
// Attach rotary encoder output pins to:
// * D2 and D3 for encoder 
// * D4 for switch 
#define EN_PIN_IN1 2
#define EN_PIN_IN2 3
#define SWITCH_IN  4

// Setup a RotaryEncoder with 4 steps per latch for the 2 signal input pins:
// RotaryEncoder encoder(EN_PIN_IN1, EN_PIN_IN2, RotaryEncoder::LatchMode::FOUR3);
// Setup a RotaryEncoder with 2 steps per latch for the 2 signal input pins:
RotaryEncoder encoder(EN_PIN_IN1, EN_PIN_IN2, RotaryEncoder::LatchMode::TWO03);

OneButton button(SWITCH_IN); // Setup a new OneButton on pin A1.  


inline void toggle(int pin)
{
   digitalWrite(pin, digitalRead(pin) ? false : true);
}


/* trash to fix */
#define I2CAddress  0x47
uint8_t SevenSegmentASCII[] = {0};
void I2CSend(int, uint8_t *, int);
/* end trash to fix */

typedef enum
{
  DMX_PROG,   //  "Prg"
  DMX_RED,    //  "rEd"
  DMX_GREEN,  //  "Grn"
  DMX_BLUE,   //  "blu"
  DMX_INT,    //  "Int"
  DMX_ALL,    //  "All"
  DMX_WHEEL,  //  "XXX" runs color wheel
  DMX_MAX_MODE
} dmx_mode_en;

// characters are chosen based on their uniqueness when mapped to a 7-segment display
// character to segment map https://www.partsnotincluded.com/segmented-led-display-ascii-library/
// library: https://github.com/dmadison/LED-Segment-ASCII
char displayStrings[][4] {{"Prg"}, {"rEd"}, {"Grn"}, {"blu"}, {"Int"}, {"All"}, {"XXX"}};

// maps a single segment per index so it 'rolls' around the outside segments.
uint8_t roller[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40}; // segments a-g

char displayData[4];  // buffer for display data.

dmx_mode_en dmxMode, dmxSelect;
int dmxAddress, dmxRed, dmxGreen, dmxBlue;
bool selectMode, buttonPress, encoderUp, encoderDown;

uint8_t dmxData[512] = {0};     // does not include DMX512 'code' byte!
uint8_t dmaBuffer[513] = {0};

void sendDigits(char *data);
void setDisplayData(dmx_mode_en mode);

// https://stackoverflow.com/a/20765875/30494907
dmx_mode_en operator ++(dmx_mode_en &id, int)
{
   dmx_mode_en currentID = id;

   if ( DMX_MAX_MODE < id + 1 ) id = DMX_PROG;
   else id = static_cast<dmx_mode_en>( id + 1 );

   return ( currentID );
}
dmx_mode_en operator --(dmx_mode_en &id, int )
{
   dmx_mode_en currentID = id;

   if ( DMX_PROG < id - 1 ) id = DMX_MAX_MODE;
   else id = static_cast<dmx_mode_en>( id - 1 );

   return ( currentID );
}

///
// @brief copy the rolling segment data to the display buffer
//
void displayRoller(void)
{
  static int segment = 0;
  static unsigned long lastMillis = 0;

  // update the roller at 5 Hz
  if (lastMillis + 200 < millis())
  {
    lastMillis = millis();

    memset(displayData, roller[segment], sizeof(displayData));

    // update for next time.
    if (++segment >= sizeof(roller))
      segment = 0;
  }
}

///
// @brief during select, this function displays and sends the mode string
//
void displayMode(dmx_mode_en mode)
{
  char buffer[4];

  buffer[0] = '-';  // set first char to '-'
  memcpy(&buffer[1], &displayStrings[mode][0], 3); // copy first 3 bytes

  sendDigits(buffer);

}

///
// @brief copies DMX values to DMA buffer and starts DMA.
// for now this function outputs the 513 bytes to serial.
//
void startDmx(void)
{
  char buffer[12];

  memcpy(&dmaBuffer[1], dmxData, sizeof(dmxData));

  sprintf(buffer, "code = %3d", dmaBuffer[0]);
  Serial.println(buffer);

  for (int x = 1; x < sizeof(dmaBuffer); x++)
  {
    Serial.print(dmaBuffer[x]);
    if (x < sizeof(dmaBuffer)-1)
      Serial.print(", ");
  }
  Serial.println(".");

}

///
// @brief updates the colors based on a color wheel process.
// @note wheel function from  https://github.com/carl3721/stm32-dmx512
//
void colorWheel(void)
{
  static int wheel= 0;
  static unsigned long lastMillis = 0;

  // update the color wheel at 12.5 Hz
  if (lastMillis + 80 < millis())
  {
    lastMillis = millis();
    // calculate color wheel colors
    if (wheel <= 255) {
        dmxRed = wheel;
        dmxGreen = 0;
        dmxBlue = 255 - wheel;
    } else if (255 < wheel && wheel <= 511) {
        dmxRed = 255 - (wheel - 256);
        dmxGreen = wheel - 256;
        dmxBlue = 0;
    } else if (511 < wheel && wheel <= 767) {
        dmxRed = 0;
        dmxGreen = 255 - (wheel - 512);
        dmxBlue = wheel - 512;
    }
    wheel++;
    wheel %= 768;
  }
}

void setup(void)
{
  Serial.begin(115200);

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  // link the double-click function to be called on a double-click event.
  button.attachDoubleClick(doubleclick);
  button.attachClick(toggleSelect);

  while (!Serial)
    ;  // wait for serial port to connect. Needed for native USB port only

  Serial.println("ready");

}

void loop(void)     //int main(void)
{

  encoder.tick();
  button.tick(); // watch the push button:
  checkForEncChange();

  //while(1)
  {
    if (selectMode)
    {
      if (encoderUp)
      {
        dmxMode++;
        if (dmxMode == DMX_MAX_MODE) // rollover
          dmxMode = DMX_PROG;
        encoderUp = false;
      }
      else if (encoderDown)
      {
        if (dmxMode == DMX_PROG) // bottom
          dmxMode = DMX_WHEEL; // wrap around
        else
          dmxMode--;
        encoderUp = false;
      }

      displayMode(dmxMode);
    }
    else// run mode:
    {
      switch (dmxMode)
      {
      case DMX_PROG:   // program DMA address
        if (encoderUp)
        {
          dmxAddress++;
          encoderUp = false;
        }
        else if (encoderDown)
        {
          dmxAddress--;
          encoderDown = false;
        }
        // bounds check address
        if (dmxAddress > 512)
          dmxAddress = 512;
        else if (dmxAddress < 1)
          dmxAddress = 1;
        break;

      case DMX_RED:
        if (encoderUp)
        {
          dmxRed++;
          encoderUp = false;
        }
        else if (encoderDown)
        {
          dmxRed--;
          encoderDown = false;
        }
        break;

      case DMX_GREEN:
        if (encoderUp)
        {
          dmxGreen++;
          encoderUp = false;
        }
        else if (encoderDown)
        {
          dmxGreen--;
          encoderDown = false;
        }
        break;

      case DMX_BLUE:
        if (encoderUp)
        {
          dmxBlue++;
          encoderUp = false;
        }
        else if (encoderDown)
        {
          dmxBlue--;
          encoderDown = false;
        }
        break;

      case DMX_INT:
      case DMX_ALL:
        if (encoderUp)
        {
          dmxRed++;
          dmxGreen++;
          dmxBlue++;
          encoderUp = false;
        }
        else if (encoderDown)
        {
          dmxRed--;
          dmxGreen--;
          dmxBlue--;
          encoderDown = false;
        }
        break;

      case DMX_WHEEL:
        colorWheel();
        break;
      } // switch (dmxMode)

      // bounds check colors
      if (dmxRed > 255)
        dmxRed = 255;
      else if (dmxRed < 0)
        dmxRed = 0;
      if (dmxGreen > 255)
        dmxGreen = 255;
      else if (dmxGreen < 0)
        dmxGreen = 0;
      if (dmxBlue > 255)
        dmxBlue = 255;
      else if (dmxBlue < 0)
        dmxBlue = 0;

      if (dmxMode != DMX_WHEEL)
      {
        loadDmxData(dmxMode, dmxAddress, dmxRed, dmxGreen, dmxBlue);
      }

      setDisplayData(dmxMode); // sets up displayer buffer and calls sendDigits()

    }
  }

  // send DMX data
  startDmx();

  // wait for 100mS
  delay(100);
}

///
//  @brief loads the appropriate data into the dmx buffer
//
void loadDmxData(dmx_mode_en mode, int address, int red, int green, int blue)
{
  int temp;

  switch (mode)
  {
  case DMX_PROG:  // turn off fixture while selecting active address
    memset(dmxData, 0, sizeof(dmxData));
    break;

  case DMX_RED:
    dmxData[address] = red;
    break;

  case DMX_GREEN:
    dmxData[address] = green;
    break;

  case DMX_BLUE:
    dmxData[address] = blue;
    break;

  case DMX_INT: // intensity
    temp = (red + green + blue) / 3;
    dmxData[address] = temp;
    break;

  case DMX_ALL: //
    temp = (red + green + blue) / 3;
    memset(dmxData, temp, sizeof(dmxData));
    break;

  case DMX_WHEEL:
    // colorWheel() loads the dmx array directly
    break;

  default:  // awe-shit!
    memset(dmxData, 0xFF, sizeof(dmxData));
    break;

  }

}

///
// @brief loads the appropriate data in the 4-digit buffer
//
void setDisplayData(dmx_mode_en mode)
{
  static bool toggle = false;
  char buffer[6];
  int temp;

  // display the first char of the mode in the first digit
  displayData[0] = displayStrings[mode][0];

  switch (mode)
  {

  case DMX_PROG:
    sprintf(buffer, "%3d", dmxAddress);
    break;

  case DMX_RED:
    sprintf(buffer, "%3d", dmxRed);
    break;

  case DMX_GREEN:
    sprintf(buffer, "%3d", dmxGreen);
    break;

  case DMX_BLUE:
    sprintf(buffer, "%3d", dmxBlue);
    break;

  case DMX_INT:
  case DMX_ALL:
    temp = (dmxRed + dmxGreen + dmxBlue) / 3;
    sprintf(buffer, "%3d", temp);
    break;

  case DMX_WHEEL:
    // only update the roller every other time.
    if (toggle) {
      displayRoller();  // populates displayData[]
    }
    toggle = !toggle;
    break;

  default:  // ooops
    if (toggle) {
      displayData[0] = '\"';
      memset(buffer, '\"', sizeof(buffer));
    }
    else {
      displayData[0] = 'w';
      memset(buffer, 'w', sizeof(buffer));
    }
    toggle = !toggle;
    break;

  } // switch (dmxMode)

  // for modes other than the color wheel, copy the 3 digit value
  if (dmxMode != DMX_WHEEL)
  {
    memcpy(&displayData[1], buffer, 3);
  }

  // now display the data:
  sendDigits(displayData);
}

///
// sends the data value over I2C to the 4-digit 7-segment display
//
void sendDigits(char *data)
{
  uint8_t tempBuffer[4];
  // look up 7-segment pattern per digit
  for (int x = 0; x < sizeof(data); x++)
  {
    tempBuffer[x] = SevenSegmentASCII[data[x]];
  }

  // send buffer to I2C:
  I2CSend(I2CAddress, tempBuffer, sizeof(tempBuffer));
}

bool checkForEncChange(void)
{
  static int lastPos = 0;
  int newPos = encoder.getPosition();
  if (lastPos != newPos) {
    Serial.print("pos:");
    Serial.print(newPos);
    Serial.print(" dir:");
    bool dir = encoder.getDirection()
    Serial.println((int)dir);
    if (dir)
        encoderUp = true;
    else
        encoderDown = true;
    lastPos = newPos;
    return(true);
  }
  return(false);
}

void toggleSelect(void)
{
  selectMode = ! selectMode;
}

// called when the button was pressed twice in a short time interval.
void doubleclick()
{
  toggle(LED_BUILTIN);
} 
