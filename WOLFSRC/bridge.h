#pragma once
#include <stdint.h>

//------------------------------------------------------------------------------
// Keyboard
//------------------------------------------------------------------------------

// keyboard event (key up/down)
void INL_KeyService_ISR(uint8_t key);

// request keyboard update trough INL_KeyService_ISR()
void Update_Key();

//------------------------------------------------------------------------------
// Mouse
//------------------------------------------------------------------------------

// Gets the amount that the mouse has moved
void INL_GetMouseDelta(int16_t* dx, int16_t* dy);

// Reset the amount that the mouse has moved
void Mouse_ResetDelta();

// Gets the status of the mouse buttons
uint16_t INL_GetMouseButtons(void);

// 0, no mouse
// 1, mouse available
uint8_t IN_HasMouse();

void Mouse_SetPos(int16_t x, int16_t y);

void Mouse_GetPos(int16_t* x, int16_t* y);

//------------------------------------------------------------------------------
// Joystick
//------------------------------------------------------------------------------

// Reads the absolute position of the specified joystick
void IN_GetJoyAbs(uint16_t joy, uint16_t* xp, uint16_t* yp);

// Returns the button status of the specified joystick
uint16_t INL_GetJoyButtons(uint16_t joy);

//------------------------------------------------------------------------------
// VR
//------------------------------------------------------------------------------

int16_t VR_GetAngle();

//------------------------------------------------------------------------------
// TimeCount
//------------------------------------------------------------------------------

extern uint32_t TimeCount;

// request time update (TimeCount)
void Update_Time();

//------------------------------------------------------------------------------
// AdLib
//------------------------------------------------------------------------------

// starts playing the music pointed to
void AdLib_StartMusic(uint16_t* values, uint16_t length);

// turns off the sequencer and any playing notes
void AdLib_MusicOff();

// Determines if there's an AdLib
uint8_t AdLib_Detect();

// Totally shuts down the AdLib card
void AdLib_Clean();

// Shuts down the AdLib card for sound effects
void AdLib_Shut();

typedef	struct
{
    uint32_t    length;
    uint16_t    priority;
} BridgeSoundCommon;

typedef	struct
{
    uint8_t
        mChar, cChar,
        mScale, cScale,
        mAttack, cAttack,
        mSus, cSus,
        mWave, cWave,
        nConn,

        // These are only for Muse - these bytes are really unused
        voice,
        mode,
        unused[3];
} BridgeAdLibInstrument;

typedef	struct
{
    BridgeSoundCommon       common;
    BridgeAdLibInstrument   inst;
    uint8_t                 block, data[1];
} BridgeAdLibSound;

// Plays the specified sound on the AdLib card
// When finished must call AdLib_SoundFinished()
void AdLib_PlaySound(BridgeAdLibSound* sound);

// Turns off any sound effects playing through the AdLib card
void AdLib_StopSound();

// Determines if an AdLib sound is playing
uint8_t AdLib_SoundPlaying();

void AdLib_SoundFinished();

//------------------------------------------------------------------------------
// PC Speaker
//------------------------------------------------------------------------------

// Turns off the pc speaker
void PCSpeaker_Shut();

// Stops the current sound playing on the PC Speaker
void PCSpeaker_StopSound();

// Plays the specified sound on the PC speaker
// When finished must call PCSpeaker_SoundFinished()
void PCSpeaker_PlaySound(uint8_t* data, uint32_t length);

// Stops a sample playing on the PC speaker
void PCSpeaker_StopSample();

// Plays the specified sample on the PC speaker
void PCSpeaker_PlaySample(uint8_t* data, uint32_t length);

// Determines if a PC speaker sound is playing
uint8_t PCSpeaker_SoundPlaying();

void PCSpeaker_SoundFinished();

//------------------------------------------------------------------------------
// SoundSource
//------------------------------------------------------------------------------

// Determines if there's a SoundSource
uint8_t SoundSource_Detect();

// Turns off the Sound Source
void SoundSource_Shut();

// Plays the specified sample on the Sound Source
// When finished must call SoundSource_SoundFinished()
void SoundSource_PlaySample(uint8_t* data, uint32_t length);

// Stops a sample playing on the Sound Source
void SoundSource_StopSample();

void SoundSource_SoundFinished();

//------------------------------------------------------------------------------
// SoundBlaster
//------------------------------------------------------------------------------

// Determines if there's a SoundBlaster
uint8_t SoundBlaster_Detect();

// Sets the attenuation levels for the left and right channels
void SoundBlaster_Level(int16_t left, int16_t right);

// Plays a sampled sound on the SoundBlaster
// When finished must call SoundBlaster_SoundFinished()
void SoundBlaster_PlaySample(uint8_t* data, uint32_t length);

// Stops any active sampled sound
void SoundBlaster_StopSample();

void SoundBlaster_SoundFinished();

