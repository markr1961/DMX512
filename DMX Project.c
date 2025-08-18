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

*/

typedef enum
{
  DMX_PROG;   //  "Prg"
  DMX_RED;    //  "rEd"
  DMV_GREEN;  //  "Grn"
  DMX_BLUE;   //  "Blu"
  DMX_INT;    //  "Int"
  DMX_ALL;    //  "All"
  DMX_WHEEL;  //  "XXX" runs color wheel
  DMA_MAX_MODES;
} dmx_mode_en;

// characters were chosen based on their uniqueness when mapped to a 7-segment display
//character to segment map https://www.partsnotincluded.com/segmented-led-display-ascii-library/
// library: https://github.com/dmadison/LED-Segment-ASCII
char displayStrings[][4] {{"Prg"}, {"rEd"}, {"Grn"}, {"blu"}, {"Int"}, {"All"}, {"XXX"}};

// maps a single segment per index so it 'rolls' around the outside segments.
roller[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40]; // segments a-g

char displayData[4];  // buffer for display data.

dmx_mode_en dmxMode, dmxSelect;
int dmxAddress, dmxRed, dmxGreen, dmxBlue;
bool buttonPress, encoderUp, encoderDown;

uint8_t dmxData[512] = {0};     // does not include DMX512 'code' byte!

/*
### debounce:
IRQ falling: clear & disable IRQ, set counter = 50mS.
main loop button handler:
    if counter,
        counter --;
        if counter = 0 && button low
            set button press flag
            enable button IRQ
*/

///
// @brief copy the rolling segment data to the display buffer
//
void displayRoller(void)
{
  static int segment = 0;

  if (segment) >= sizeof(roller)
    segment = 0;

  memset (displayData, roller[segment], sizeof(buffer));

  segment++;  // update for next time.

}

///
// @brief during select, this function displays and sends the mode string
//
void displayMode(mode)
{
  char buffer[4];

  buffer[0] = '-';  // set first char to '-'
  memcpy(&buffer[1], &displayStrings[mode][0], 3); // copy first 3 bytes

  sendDigits(buffer);

}

void startDmx(void)
{
  uint8_t dmaBuffer[513] = {0};
  char buffer[24];

  memcpy(&dmaBuffer[1], dmxData, sizeof(dmxData));

  sprintf(buffer, "code = %d", dmaBuffer[0]);
  Serial.println(buffer);

  for (int x = 1; x < sizeof(dmaBuffer); x++)
  {
    Serial.print(damBuffer[x]);
    if (x < sizeof(dmaBuffer)-1)
      Serial.print(", ");
  }
  Serial.println(".");

}

void colorWheel(void)
{
  // need code here.....
}

void setup(void)
{
  Serial.begin(115200);
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  while (!Serial)
  {
    ;  // wait for serial port to connect. Needed for native USB port only
  }

  digitalWrite(LED_BUILTIN, HIGH); // turn on LED

}

void main(void)
{

  while(1)
  {
    if (buttonPress)
    {
      selectMode = ! selectMode;
      buttonPress = false;
    }

    if (selectMode)
    {
      if (encoder_up)
      {
        dmxMode++;
        if (dmxMode == DMA_MAX_MODES) // rollover
          dmaMode = DMX_PROG;
        encoder_up = false;
      }
      else if (encoder_down)
      {
        if (dmxMode == DMX_PROG) // bottom
          dmxMode = DMX_MAX_MODE - 1; // wrap around
        else
          dmxMode--;
        encoder_up = false;
      }

      displayMode(dmxMode);

    }
    else// run mode:
    {

      switch (dmxMode)
      {
      case DMX_PROG:   // program DMA address
        if (encoder_up)
        {
          dmx_addres++;
          encoder_up = false;
        }
        else if (encoder_down)
        {
          dmx_addres--;
          encoder_down = false;
        }
        // bounds check address
        if (dmxAddress > 512)
          dmxAddress = 512;
        else if (dmxAddress < 1)
          dmxAddress = 1;
        break;

      case DMX_RED:
        if (encoder_up)
        {
          dmx_red++;
          encoder_up = false;
        }
        else if (encoder_down)
        {
          dmx_red--;
          encoder_down = false;
        }
        break;

      case DMV_GREEN:
        if (encoder_up)
        {
          dmx_green++;
          encoder_up = false;
        }
        else if (encoder_down)
        {
          dmx_green--;
          encoder_down = false;
        }
        break;

      case DMX_BLUE:
        if (encoder_up)
        {
          dmx_blue++;
          encoder_up = false;
        }
        else if (encoder_down)
        {
          dmx_blue--;
          encoder_down = false;
        }
        break;

      case DMX_INT:
      case DMX_ALL:
        if (encoder_up)
        {
          dmx_red++;
          dmx_green++;
          dmx_blue++;
          encoder_up = false;
        }
        else if (encoder_down)
        {
          dmx_red--;
          dmx_green--;
          dmx_blue--;
          encoder_down = false;
        }
        break;

      case DMX_WHEEL:
        colorWheel(dmxAddress); // populates dmxData[];
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
    dmaData[address] = red;
    break;

  case DMV_GREEN:
    dmaData[address] = green;
    break;

  case DMX_BLUE:
    dmaData[address] = blue;
    break;

  case DMX_INT: // intensity
    temp = (red + green + blue) / 3;
    dmaData[address] = temp;
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

  // display the first char of the mode in the first digit
  display[0] = displayStrings[mode][0];

  switch (mode)
  {

  case DMX_PROG:
    sprintf(buffer, "%3d", dmxAddress);
    break;

  case DMX_RED:
    sprintf(buffer, "%3d", dmxRed);
    break;

  case DMV_GREEN:
    sprintf(buffer, "%3d", dmxGreen);
    break;

  case DMX_BLUE:
    sprintf(buffer, "%3d", dmxBlue);
    break;

  case DMX_INT:
  case DMX_ALL:
    int temp = (dmxRed + dmxGreen + dmxBlue) / 3;
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
        memset(displayData, '\"', sizeof(displayData));
        memset(buffer, '\"', sizeof(buffer));
    }
    else {
      memset(&displayData, 'w', sizeof(displayData));
      memset(buffer, 'w', sizeof(buffer));
    }
    toggle = !toggle;
    break;

  } // switch (dmxMode)

  // for all modes other than the color wheel, copy the 3 digit value
  if (dmxMode != DMX_WHEEL)
  {
    memcpy(&display[1], buffer, 3);
  }

  // now display the data:
  sendDigits(displayData);
}

///
// sends the data value over I2C to the 4-digit 7-segment display
//
void sendDigits(char *data)
{
  int tempBuffer[4];
  // look up 7-segment pattern per digit
  for (int x = 0; x < sizeof(data); x++)
  {
    tempBuffer[i] = SevenSegmentASCII[data[i]];
  }

  // send buffer to I2C:
  I2CSend(I2CAddress, tempBuffer, sizeof(tempBuffer));
}
