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
 *******************************************************************************
 */

#include <usbh_midi.h>
#include <usbhub.h>

// ToDo explain this, can we only havew 1 UART out?
#ifdef USBCON
#define _MIDI_SERIAL_PORT Serial1
#else
#define _MIDI_SERIAL_PORT Serial
#endif

// Set to 1 if you want to wait for the Serial MIDI transmission to complete.
// For more information, see https://github.com/felis/USB_Host_Shield_2.0/issues/570
#define ENABLE_MIDI_SERIAL_FLUSH 1

//////////////////////////
// MIDI Pin assign
// 2 : GND
// 4 : +5V(Vcc) with 220ohm
// 5 : TX
//////////////////////////

USB Usb;
USBH_MIDI Midi(&Usb);

void MIDI_poll();

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
void loop() {
  Usb.Task();

  if (Midi) {
    digitalWrite(LED_BUILTIN, LOW);
    MIDI_poll();
  }
  //delay(1ms) if you want
  //delayMicroseconds(1000);
}

uint8_t outBuf[3];
uint8_t size;

// Poll USB MIDI Controler and send to serial MIDI
void MIDI_poll() {
  do {
    if ((size = Midi.RecvData(outBuf)) > 0) {
      digitalWrite(LED_BUILTIN, HIGH);
      
      //MIDI Output
      _MIDI_SERIAL_PORT.write(outBuf, size);
#if ENABLE_MIDI_SERIAL_FLUSH
      _MIDI_SERIAL_PORT.flush();
#endif
    }
  } while (size > 0);
}
