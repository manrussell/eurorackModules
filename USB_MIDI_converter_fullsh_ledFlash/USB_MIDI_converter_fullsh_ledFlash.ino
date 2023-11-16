/*
 *******************************************************************************
 * USB-MIDI to Legacy Serial MIDI converter
 * Copyright (C) 2012-2021 Yuuichi Akagawa
 *
 * Idea from LPK25 USB-MIDI to Serial MIDI converter
 *   by Collin Cunningham - makezine.com, narbotic.com
 *
 * This is sample program. Do not expect perfect behavior.
 *
 * Edit by Me also
 * Harware: Arduino Leonardo and USB host shield 2.0 
 * //////////////////////////
 * // MIDI Pin assign
 * // 2 : GND
 * // 4 : +5V(Vcc) with 220ohm
 * // 5 : TX
 * //////////////////////////
 * Software Overview ...
 *
 * ToDo: 
 * 1) add some timing code i.e. so the logic analyser can display whats going on
 * 2a) Add a SPI dac and midi to CV calc
 * 2b) Add a DAC that uses GPIO's 
 *******************************************************************************
 */

#include <usbh_midi.h>
#include <usbhub.h>

/*  ToDo: explain this, can we only have 1 UART out? is this just making more portable */
#ifdef USBCON
#   define _MIDI_SERIAL_PORT Serial1
#else
#   define _MIDI_SERIAL_PORT Serial
#endif

#define USE_CLASSIC_MIDI_BAUD   1
/* There's no need for my MIDI to go @31250 baud
 * I can go faster like 115200 
 * 1) if i'm going between configurable modules or,
 * 2) If i am connected to 'real' modules like the Yamaha Tx7
 */

#if USE_CLASSIC_MIDI_BAUD
#   define BAUD_RATE (31250)
#else
#   define BAUD_RATE (115200)
#endif 

/* Set to 1 if you want to wait for the Serial MIDI transmission to complete.
 * For more information, see https://github.com/felis/USB_Host_Shield_2.0/issues/570
 */
#define ENABLE_MIDI_SERIAL_FLUSH  1

// #define ENABLE_UHS_DEBUGGING 1

/*TODO comments maybe?*/
USB Usb;
USBH_MIDI Midi(&Usb);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  _MIDI_SERIAL_PORT.begin( BAUD_RATE );

  if (Usb.Init() == -1) 
  {
    while (1)
    {
      /* REVIEWNOTE: Can i try to init again? Or is it games over */
      while(1) /* Debug blink pattern */
      {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500ms)
        digitalWrite(LED_BUILTIN, LOW);
        delay(1000ms)
      }
    }

  }
  delay(200);
}

// ToDo this doesn't look very optimised
void loop() 
{
  static uint8_t outBuf[3]; /* What about other (sysex/clock?) messages? a bigger buffer */
  static uint8_t size;

  digitalWrite(LED_BUILTIN, LOW);
  Usb.Task();

  if (Midi) /* What does this 'if' line actuall do? */
  {
    /* Poll USB MIDI Controller and send to serial MIDI */
    do 
    {
      /* MIDI message is not always 3 bytes */
      if ((size = Midi.RecvData(outBuf)) > 0) 
      {
        digitalWrite(LED_BUILTIN, HIGH);
        
        // MIDI Output
        _MIDI_SERIAL_PORT.write(outBuf, size);
 #if ENABLE_MIDI_SERIAL_FLUSH
        _MIDI_SERIAL_PORT.flush();
 #endif
      }
    } while(size > 0);
  }
  //delay(1ms) if you want
  //delayMicroseconds(1000);
}


/*
 * Other Relavent documentation

Interface modifications

The shield is using SPI for communicating with the MAX3421E USB host controller. It uses the SCK, MISO and MOSI pins via the ICSP on your board.

Note this means that it uses pin 13, 12, 11 on an Arduino Uno, so these pins can not be used for anything else than SPI communication!

Furthermore it uses one pin as SS and one INT pin. These are by default located on pin 10 and 9 respectively. They can easily be reconfigured in case you need to use them for something else by cutting the jumper on the shield and then solder a wire from the pad to the new pin.

After that you need modify the following entry in UsbCore.h:

typedef MAX3421e<P10, P9> MAX3421E;

For instance if you have rerouted SS to pin 7 it should read:

typedef MAX3421e<P7, P9> MAX3421E;

See the "Interface modifications" section in the hardware manual for more information.

*/