## DMX Project

### 4 digit display

'mode' is 1st digit: 'A' 'r' 'g' 'b' 'P':
'P' - program address: sets DMX start address, allowed: 1-510
'r' - red level
'g' - green level
'b' - blue level
'I' - intensity mode: increments existing RGB values.
'A' - all: sets all 512 channels to value selected
'X' - color wheel
character to segment map https://www.partsnotincluded.com/segmented-led-display-ascii-library/ library: https://github.com/dmadison/LED-Segment-ASCII

### button function
selects between mode setting and level setting.

typedef enum {
    DMX_PROG;   //  "Prog" 
    DMX_RED;    //  "rEd"
    DMV_GREEN;  //  "Grn"
    DMX_BLUE;   //  "Blu"
    DMX_INT;    //  "Int"
    DMX_ALL;    //  "All"
    DMX_WHEEL;  //  "XXX"runs color wheel
    DMA_MAX_MODES;
} dmx_mode_en;

roller[]={0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80];

dmx_mode_en dmcMode, dmxSelect;

### debounce:
IRQ falling: clear & disable IRQ, set counter = 50mS.
main loop button handler:
    if counter, 
        counter --;
        if counter = 0 && button low
            set button press flag
            enable button IRQ

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
  DisplayMode(dmxMode);

}
else// run mode:
{
  
  switch (dmxMode) {
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
        dmxAddress = 255;
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
      break;
  }

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
  
  if (dmxMode == DMX_WHEEL)
  {
    colorWheel(); // calls displayRoller()
  }
  else
  {
    loadDmx(dmxMode, dmxAddress, dmxRed, dmxGreen, dmxBlue); // callsDisplayData()
  }

}

char display[4];

void DisplayMode(
void DisplayRoller(void)
{
  static int segment = 0;
  segment++;
  if (segment) > sizeof(roller)
    segment = 0;
  memset (display, roller[segment], sizeof(display));
  
  displayDigits(display);
}

void displayData(dmx_mode_en dmxMode, int value)
{
  switch (dmxMode)
    display[0] 

}