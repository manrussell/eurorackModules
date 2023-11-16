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
 *
 *
 * ToDo: add some timing code i.e. so the logic analyser 
 * can see thats going on
 *******************************************************************************
 */

#include <usbh_midi.h>
#include <usbhub.h>

// ToDo: explain this, can we only have 1 UART out? is this just making more portable
#ifdef USBCON
#   define _MIDI_SERIAL_PORT Serial1
#else
#   define _MIDI_SERIAL_PORT Serial
#endif

#define USE_CLASSIC_MIDI_BAUD 1
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

// Set to 1 if you want to wait for the Serial MIDI transmission to complete.
// For more information, see https://github.com/felis/USB_Host_Shield_2.0/issues/570
#define ENABLE_MIDI_SERIAL_FLUSH 1

USB Usb;
USBH_MIDI Midi(&Usb);

void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  _MIDI_SERIAL_PORT.begin(31250);

  if (Usb.Init() == -1) {
    while (1)
      ;  //halt
  }
  delay(200);
}

// ToDo this doesn't look very optimised
void loop() 
{
  static uint8_t outBuf[3];
  static uint8_t size;

  Usb.Task();

  if (Midi) 
  {
    digitalWrite(LED_BUILTIN, LOW);
    // Poll USB MIDI Controler and send to serial MIDI
    do 
    {
      if ((size = Midi.RecvData(outBuf)) > 0) 
      {
        digitalWrite(LED_BUILTIN, HIGH);
        
        // MIDI Output
        _MIDI_SERIAL_PORT.write(outBuf, size);
 #if ENABLE_MIDI_SERIAL_FLUSH
        _MIDI_SERIAL_PORT.flush();
 #endif
      }
    } while (size > 0);
  }
  //delay(1ms) if you want
  //delayMicroseconds(1000);
}
