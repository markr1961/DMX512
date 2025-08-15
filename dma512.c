
/**
  * DMX512 consists of a frame containing a break, a mark before data,
  * followed by a 1 byte start code plus 512 data bytes of 250K baud.
  * The break time is >=88uS, the mark before data time is >-8uS.
  * This results in a ~23mS frame repeating at ~44Hz.
  * Up to 1 second of idle time can be inserted between frames.
  * The break and mark time are semi-flexible as long as the min is met.
  *
  * For DMX512 background information see:
  * https://learn.adafruit.com/intro-to-dmx/dmx512
  * https://learn.sparkfun.com/tutorials/introduction-to-dmx/all
  *
  * HW requirements:
  *   USART TX: 250K baud: 8 bits, no parity, 2 stop.
  *   Timer: 1uS ticks generates break and mark sequences.
  *   DMA TX: sends 512 bytes of data: 1 start code & 512 data bytes.
  *
  * Operation flow
  * 1) set I/O to GPIO mode and drive output low,
  *    start 88uS timer.
  * 2) timer complete: set I/O to USART
  *    start 8uS timer,
  * 3) timer complete: start DMA 513 bytes
  * 4) DMA complete: copy new data, return to #1
  *
  * In theory the entire process can run completely in IRQ space.
  * Using just IRQs, the timer requires two states and DMA requires one.
  * In practice, synchronization is required between a 'working' buffer and the DMA TX.
  * A practical implementation copies a working buffer to the DMA before each TX.
  * Because the break to mark to DMA is the critical path, the process starts from
  * the main loop by setting the I/O and timer for the break. After that, everything
  * progresses in IRQs until the DMA is complete. The main loop function looks for 
  * the DMA complete to repeat the process over again.
  * 
  * main loop:
  * - IDLE: copy buffer, 
            set I/O for break, 
            start 88uS timer, 
            next state: BREAK.
  * Timer IRQ: 
  * - BREAK: set I/O UART, start 8uS timer, next state: MARK.
  * - MARK: start DMA, next state: DMA.
  * DMA complete IRQ:
  * - DMA: next state: IDLE.
  */

typedef enum dmx_states
{
  DMX_STATE_BREAK;  // 88uS break
  DMX_STATE_MARK;   // 8uS mark state
  DMX_STATE_DMA;    // start DMA.
  DMX_STATE_IDLE;   // waits for DMA TX complete.
} dmx_states_en;

dmx_states_en dmxState;
uint8_t dmaBuffer[513]; // double buffer for DMA

uint8_t dataBuffer[512];  // populated by dimmer handler based on offset and values to set.


///
// @brief init's HW and data buffers.
//
void InitDmx512(void)
{
  // set up RS-485 control pin for write/TX mode
  
  // set up the timer for 1uS ticks. 
  // do not start timer or enable IRQs.
  
  // set up USART for TX
  // do not start TX or enable IRQ
  
  // set up DMA, DMA address to beginning of dmaBuffer
  // do not start or enable DMA.
  // count and enable will be set in TimerIRQ

  // set working vars
  dmxState = DMA_STATE_IDLE;
  memset(dmaBuffer, 0, sizeof(dmaBuffer);
  memset(dataBuffer, 0, sizeof(dataBuffer);

}


///
// called from main loop. Runs DMX at ~25mS intervals.
//
void RunDmx512(void)
{
  if(dmxState == DMA_STATE_IDLE)
  {
    // set start condition
    dmaBuffer[0] = 0;
    // copy working buffer to DMA buffer
    memcpy(dmaBuffer+1, dataBuffer, sizeof(dmaBuffer-1));
    // set next state to break:
    dmxState = DMX_STATE_BREAK;
    // set GPIO to I/O
    
    // enable timer IRQ & start 88uS timer
    
  }
  else // normal while waiting for timer & DMA
  {
    ; // some kind of deadman...
  }
}

///
// @brief IRQ handler for selected timer
//
void TimerIRQ(void)
{
  if (TimerIrqFlag)
  {
    // clear IRQ
    blah-blah;

    if (TimerIrqEnabled)
    {
      if (dmxState == DMX_STATE_BREAK)
      {
        // drive GPIO high, then set to UART mode
        blah-blah;

        dmxState = DMX_STATE_MARK;
        // start 8uS timer
        blah-blah;

      }
      else if (dmxState == DMX_STATE_MARK)
      {
        // disable timer & timer IRQ
        blah-blah;

        dmxState = DMX_STATE_DMA;
        // start 513 byte DMA
        dma count = sizeof(dmaBuffer);
        blah-blah;
      }
      else  // error
      {
        // disable timer & timer IRQ
        blah-blah;
        dmxState = DMA_STATE_IDLE;
      }
    }
    else // error
      DEBUG_BREAK();
  }
  else // some other Timer or an error
    DEBUG_BREAK();
}

///
// @brief IRQ handler for DMA TX
//
void DmaTxIrq(void)
{
  if (DmaTxIrqFlag)
  {
    // clear DMA TX IRQ
    blah-blah;

    if (DmaTxIrqEnabled)
    {
      if (dmxState == DMX_STATE_DMA)
      {
        // disable DMA
        blah-blah;
        // set for next pass
        dmxState = DMA_STATE_IDLE;
      }
      else // error
      {
        DEBUG_BREAK();
        // disable DMA
        blah-blah;
        dmxState = DMA_STATE_IDLE;
      }
    }
    else // how dafaq did we get here?
    {
      DEBUG_BREAK();
      // disable DMA
      blah-blah;
      dmxState = DMA_STATE_IDLE;
    }
  }
  else // some other DMA channel
  {
    //
  }
}

// eof dma512.c
