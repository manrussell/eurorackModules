/*****************************************************************************************
 * USB-MIDI to Legacy Serial MIDI converter
 *
 * Author:
 *
 * Overview
 * ========
 *
 * a USB(host) to CV converter with: gate, pitch CV and Velocity CV
 *
 * Hardware
 * ========
 *
 * Arduino Leonardo and,
 * USB host shield 2.0
 * 74hc595 shift register
 * a DAC =  AD7302
 *
 *
 * (Optional MIDI out) MIDI Pin assign
 * 2 : GND
 * 4 : +5V(Vcc) with 220ohm
 * 5 : TX
 *
 * Software Overview
 * =================
 *
 *
 * libraries used
 * ==============
 *
 * USB-MIDI by lathoub
 * TODO ...
 *
 * MIDI Protocol
 * =============
 *
 * Curently i'm only interested in these types of messages
 * note on:  0x90, key,     veolcity
 * Note-off: 0x80, key,     velocity
 * CC      : 0xB0, CC num,  value
 *
 * ToDo
 * ====
 *
 * 1) add some timing code i.e. so the logic analyser can display whats going on
 * 2a) Add a SPI dac and midi to CV calc
 * 2b) Add a DAC that uses GPIO's
 * 3) add a PWM dac (8bit fixed i think)
 * 4) +3Volt offset for summing with the output
 * 5) trigger output
 *
 ******************************************************************************/

#include <usbh_midi.h>
#include <usbhub.h>
// #include "pitches.h"

/******************************************************************************
 * defines
 ******************************************************************************/

/*  ToDo: explain this, can we only have 1 UART out? is this just making more portable
 */
#ifdef USBCON
#    define _MIDI_SERIAL_PORT Serial1
#else
#    define _MIDI_SERIAL_PORT Serial
#endif

#define USE_CLASSIC_MIDI_BAUD 1

/* There's no need for my synth's MIDI to go at 31250 baud
 * I can go faster e.g. 115200
 * 1) if i'm going between configurable modules or,
 * 2) If i am connected to 'real' modules like the Yamaha Tx7
 */
#if USE_CLASSIC_MIDI_BAUD
#    define BAUD_RATE ( 31250 )
#else
#    define BAUD_RATE ( 115200 )
#endif

/* Set to 1 if you want to wait for the Serial MIDI transmission to complete.
 * For more information, see https://github.com/felis/USB_Host_Shield_2.0/issues/570
 */
#define ENABLE_MIDI_SERIAL_FLUSH ( 1 )

// #define ENABLE_UHS_DEBUGGING 1

/* DAC has 2 outputs A(bar)/B */
#define DAC1_PITCH_CHANNEL       ( 0u )
#define DAC1_VELOCITY_CHANNEL    ( 1u )

/* could make a better API here i.e. x_ */
/* TODO try using digitalWrite( DACWRPin, LOW ); instead of bit masking */
#define DAC1_WR_PIN              ( 0u )
#define DAC1_WRITE_wait( x_ )    ( PORTD &= ~( 1 << DAC1_WR_PIN ) )
#define DAC1_WRITE_SET( x_ )     ( PORTD |= ( 1 << DAC1_WR_PIN ) )

/* This is a dual output DAC, hence A/B */
#define DAC1_AB_PIN              ( 1u )
#define DAC1_CHANNEL_A( x_ )     ( PORTD &= ~( 1 << DAC1_AB_PIN ) )
#define DAC1_CHANNEL_B( x_ )     ( PORTD |= ( 1 << DAC1_AB_PIN ) )

#define DAC2_CC1_CHANNEL         ( 0u )
#define DAC2_CC2_CHANNEL         ( 1u )

/* midi message type e.g. noteon/off */
#define MIDI_MSGTYPE             ( outBuf[0] )

/* midi note value e.g. pitch */
#define MIDI_PITCH               ( outBuf[1] )
#define MIDI_VELOCITY            ( outBuf[2] )

/* midi CC number e.g. 64 for cutoff */
#define MIDI_CC_NUM              ( outBuf[1] )
#define MIDI_CC_VAL              ( outBuf[2] )

/*******************************************************************************
 * typedefs
 ******************************************************************************/

typedef enum
{
    noteOnMsg  = 0x90,
    noteOffMsg = 0x80,
    CCMsg      = 0xB0
} midiTypes_t;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

void DACOut1( uint8_t channel, uint8_t val );
void DACOut2( uint8_t channel, uint8_t val );
uint8_t LOOK_UP_MIDI_TO_DAC1_TABLE( uint8_t note ); // remove?

/*******************************************************************************
 * global data
 ******************************************************************************/

/* TODO comments */
USB Usb;
USBH_MIDI Midi( &Usb );
USBHub Hub( &Usb ); // not tried usng a hub might work(??)

const int DACABPinPin           = 2; /* DAC pins */
const int DACWRPin              = 3; /* DAC pins */

const int gatePin               = 8; /* noteon/off */

const int latchPin              = 5; /* 74HC595 Latch pin of  */
const int clockPin              = 6; /* 74HC595 Clock pin of  */
const int dataPin               = 4; /* 74HC595 Data pin of  */

int builtInLEDPin               = 13; /* for gate/status only */

/* -3 octave offset, to remove from the midi nore values as i will add an offset
 * electrically */
/* this is a midi note value */
const uint8_t minOctavePlayable = 3;
const int noteOffest            = ( 12 * minOctavePlayable );

/* this is a midi note value */
const uint8_t maxOctavePlayable = 8;
const int noteHighestPlayable   = ( 12 * maxOctavePlayable );

/* look up table, to get from midi note value to a value for the DAC
 * Array index is the midi note value, array contents is the mapped value
 */
uint8_t DAC1_TABLE[255]         = { 0 };

/*******************************************************************************
 * functions
 ******************************************************************************/

void setup( )
{
    /* midi note to gate output LED for midi gate status */
    pinMode( builtInLEDPin, OUTPUT );
    digitalWrite( builtInLEDPin, LOW );
    pinMode( gatePin, OUTPUT );
    digitalWrite( gatePin, LOW );

    /* DAC outputs */
    pinMode( DACABPinPin, OUTPUT );
    pinMode( DACWRPin, OUTPUT );
    digitalWrite( DACABPinPin, LOW );
    digitalWrite( DACWRPin, LOW );

    /* initial state (no input) for the DAC */
    DAC1_WRITE_wait( );

    /* Set all the pins of 74HC595 */
    pinMode( latchPin, OUTPUT );
    pinMode( dataPin, OUTPUT );
    pinMode( clockPin, OUTPUT );
    digitalWrite( latchPin, LOW );
    digitalWrite( dataPin, LOW );
    digitalWrite( clockPin, LOW );

    /* Initialise the look up table */
    for( int i; i < sizeof( DAC1_TABLE ); ++i )
    {
        DAC1_TABLE[i] = i;
    }

    // _MIDI_SERIAL_PORT.begin( BAUD_RATE );

    if( Usb.Init( ) == -1 )
    {
        while( 1 )
        {
            /* REVIEWNOTE: Can i try to init again? Or is it game over? */
            while( 1 )
            {
                /* Debug blink pattern */
                digitalWrite( builtInLEDPin, HIGH );
                delay( 500 );
                digitalWrite( builtInLEDPin, LOW );
                delay( 1000 );
            }
        }
    }
    delay( 200 );

    // Serial.begin( 115200 ); /* to PC */
}

void loop( )
{
    static uint8_t outBuf[MIDI_EVENT_PACKET_SIZE];
    uint8_t size;
    uint16_t rcvd;
    static uint8_t noteOnPitch; // last note on pitch/ most recently played TODO
                                // increase midi buffer size

    Usb.Task( );

    if( Midi ) /* What does this 'if' line actuall do? */
    {
        /* Poll USB MIDI Controller and send to serial MIDI */
        do
        {
            /* REVIEWNOTE MIDI message is not always 3 bytes, what bout sysex?*/
            if( ( size = Midi.RecvData( outBuf ) ) > 0 )
            {
                /* set CV/gate outputs */
                switch( MIDI_MSGTYPE )
                {
                case noteOnMsg:

                    noteOnPitch = MIDI_PITCH; // improve but basic midi not buffer
                    // if( ( noteOffest < noteOnPitch  ) && ( noteOffest > noteOnPitch
                    // ) )
                    if( ( 36 < noteOnPitch ) && ( ( 36 * 5 ) > noteOnPitch ) )
                    {
                        digitalWrite( builtInLEDPin, HIGH );

                        digitalWrite( gatePin, HIGH );

                        /* set pitchCV output */
                        DACOut1( DAC1_PITCH_CHANNEL,
                                 LOOK_UP_MIDI_TO_DAC1_TABLE( noteOnPitch ) );

                        /* set velocity CV output
                         * REVIEWNOTE multiply by 2 as max midi cc val == 127
                         */
                        DACOut1( DAC1_VELOCITY_CHANNEL, MIDI_VELOCITY * 2 );

                        /* TODO trigger out would be nice ... */
                    }
                    break;
                case noteOffMsg:
                    /* check if note off == noteOnPitch ... already portamento'd to */
                    if( MIDI_PITCH == noteOnPitch )
                    {
                        digitalWrite( builtInLEDPin, LOW );

                        // set gate low if no note on's exist
                        digitalWrite( gatePin, LOW );

                        /*  set velocity - CV output to '0' */
                        // DACOut1( DAC1_VELOCITY_CHANNEL, 0 ); // TMP TMP TMP

                        /* NOTE no need to set PITCHCV just leave as is */
                    }

                    break;
                case CCMsg:
                    /* check for cc1 and cc2 only */
                    if( MIDI_CC_NUM == 1 )
                    {
                        /* multiply by 2 as max midi cc val == 127 */
                        DACOut2( DAC2_CC1_CHANNEL, ( MIDI_CC_VAL * 2 ) );
                    }
                    else if( MIDI_CC_NUM == 2 )
                    {
                        /* multiply by 2 as max midi cc val == 127 */
                        DACOut2( DAC2_CC2_CHANNEL, ( MIDI_CC_VAL * 2 ) );
                    }
                    else
                    {
                        /* ignore any other cc values */
                    }

                    break;
                default:
                    digitalWrite( builtInLEDPin, LOW );
                }

                /* MIDI Output */
                // _MIDI_SERIAL_PORT.write( outBuf, size );
                // Midi.SendData(buf);

#if ENABLE_MIDI_SERIAL_FLUSH
                // _MIDI_SERIAL_PORT.flush( );
#endif
            }
        }
        while( size > 0 );
    }
    // delayMicroseconds( 1 );
}

void DACOut1( uint8_t channel, uint8_t val )
{
    if( channel == 1 )
    {
        DAC1_CHANNEL_B( channel );
    }
    else
    {
        /* set channel A (active low) */
        DAC1_CHANNEL_A( );
    }

    delayMicroseconds( 2 );

    /* DAC CS pin electronically held low */
    /* DAC WR already low, write to register */

    /* shift register updates DAC pins D7-D0 */
    updateShiftRegister( val, latchPin, dataPin, clockPin );

    delayMicroseconds( 2 );

    /* DAC WR high, updates DAC output */
    DAC1_WRITE_SET( );

    delayMicroseconds( 2 );

    /* DAC WR low, write to register */
    DAC1_WRITE_wait( );

    /* DAC LDAC pin electronically held high */
}

/*
 * updateShiftRegister() - This function sets the latchPin to low, then calls the
 * Arduino function 'shiftOut' to shift out contents of variable 'leds' in the shift
 * register before putting the 'latchPin' high again.
 */
void updateShiftRegister( uint8_t val, uint8_t latchP, uint8_t dataP, uint8_t clockP )
{
    digitalWrite( latchP, LOW );
    shiftOut( dataP, clockP, LSBFIRST, val );
    digitalWrite( latchP, HIGH );
}

void DACOut2( uint8_t channel, uint8_t val )
{
}

uint8_t LOOK_UP_MIDI_TO_DAC1_TABLE( uint8_t note )
{
    // remove note offset e.g. - 36
    // no notes below/above the 5 octave range
    return DAC1_TABLE[note - noteOffest];
}
