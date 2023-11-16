/* based upon 2 examples from USB-MIDI lib
 * "MIDi_DIN2USB.ino" and "allevents.ino"
 * TODO 
 * add code for CV/gate ie a SPI DAC and a gpio
 * more callbacks, check flush, 
 * prob remove "onSerialMessage" unless i want this to appear as a midi device as well? -good for dev/debugging
 * add some debugging features printf
 * could add arpeggion code etc
*/

#include <USB-MIDI.h>
USING_NAMESPACE_MIDI;

#define LED 13

typedef USBMIDI_NAMESPACE::usbMidiTransport __umt;
typedef MIDI_NAMESPACE::MidiInterface<__umt> __ss;
__umt usbMIDI(0);  // cableNr
__ss MIDICoreUSB((__umt&)usbMIDI);

typedef Message<MIDI_NAMESPACE::DefaultSettings::SysExMaxSize> MidiMessage;

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDICoreSerial);

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void setup() {
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  MIDICoreUSB.setHandleMessage(onUsbMessage);
  MIDICoreSerial.setHandleMessage(onSerialMessage);
  MIDICoreSerial.setHandleNoteOn(handlerNoteOn);
  MIDICoreSerial.setHandleNoteOff(handlerNoteOff);
  MIDICoreSerial.setHandleControlChange(OnControlChange);
#if 0
  MIDICoreSerial.setHandlePitchBend(OnPitchBend);
  MIDICoreSerial.setHandleSystemExclusive(OnSystemExclusive);
  MIDICoreSerial.setHandleProgramChange(OnProgramChange);
  // MIDICoreSerial.setHandleAfterTouchChannel(OnAfterTouchChannel);
  MIDICoreSerial.setHandleTimeCodeQuarterFrame(OnTimeCodeQuarterFrame);
  MIDICoreSerial.setHandleSongPosition(OnSongPosition);
  MIDICoreSerial.setHandleSongSelect(OnSongSelect);
  MIDICoreSerial.setHandleTuneRequest(OnTuneRequest);
  MIDICoreSerial.setHandleClock(OnClock);
  MIDICoreSerial.setHandleStart(OnStart);
  MIDICoreSerial.setHandleContinue(OnContinue);
  MIDICoreSerial.setHandleStop(OnStop);
  MIDICoreSerial.setHandleActiveSensing(OnActiveSensing)
#endif
  MIDICoreUSB.begin(MIDI_CHANNEL_OMNI);
  MIDICoreSerial.begin(MIDI_CHANNEL_OMNI);
}

void OnPitchBend(byte channel, byte number, byte value) {
  digitalWrite(LED, HIGH);
  MIDICoreUSB.sendControlChange(number, value, channel);
}

void OnSystemExclusive(byte channel, byte number, byte value) {
  digitalWrite(LED, HIGH);
  MIDICoreUSB.sendControlChange(number, value, channel);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void loop() {
  digitalWrite(LED, LOW);
  MIDICoreUSB.read();
  MIDICoreSerial.read();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void onUsbMessage(const MidiMessage& message) {
  digitalWrite(LED, HIGH);
  MIDICoreSerial.send(message);
}

// not really using this though
void onSerialMessage(const MidiMessage& message) {
  digitalWrite(LED, HIGH);
  MIDICoreUSB.send(message);
}

void handlerNoteOn(byte channel, byte pitch, byte velocity) {
  digitalWrite(LED, HIGH);
  MIDICoreUSB.sendNoteOn(pitch, velocity, channel);
}

void handlerNoteOff(byte channel, byte pitch, byte velocity) {
  digitalWrite(LED, HIGH);
  MIDICoreUSB.sendNoteOff(pitch, velocity, channel);
}

void OnControlChange(byte channel, byte number, byte value) {
  digitalWrite(LED, HIGH);
  MIDICoreUSB.sendControlChange(number, value, channel);
}

// optional i guess
void OnControlChange(byte channel, byte number, byte value) {
  digitalWrite(LED, HIGH);
  MIDICoreUSB.sendControlChange(number, value, channel);
}


