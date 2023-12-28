#pragma once
#include <stdint.h>

//------------------------------------------------------------------------------
// state request by application
//------------------------------------------------------------------------------

uint8_t IN_HasMouse();

// Gets the amount that the mouse has moved
void INL_GetMouseDelta(int16_t* x, int16_t* y);

// Gets the status of the mouse buttons
uint16_t INL_GetMouseButtons(void);

// Reads the absolute position of the specified joystick
void IN_GetJoyAbs(uint16_t joy, uint16_t* xp, uint16_t* yp);

// Returns the button status of the specified joystick
uint16_t INL_GetJoyButtons(uint16_t joy);

//------------------------------------------------------------------------------
// update request by application
// calls added to update app state that won't change without ISR
//------------------------------------------------------------------------------

extern uint32_t TimeCount;

// request keyboard update trough INL_KeyService_ISR()
void Update_Key();

// request time update (TimeCount)
void Update_Time();

//------------------------------------------------------------------------------
// events to application
//------------------------------------------------------------------------------

// keyboard event (key up/down)
void INL_KeyService_ISR(uint8_t key);

//------------------------------------------------------------------------------
// events from application
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

// Plays the specified sound on the AdLib card
void AdLib_PlaySound(AdLibSound* sound);

// Turns off any sound effects playing through the AdLib card
void AdLib_StopSound();

// Determines if an AdLib sound is playing
uint8_t AdLib_SoundPlaying();

// Turns off the pc speaker
void PCSpeaker_Shut();

// Stops the current sound playing on the PC Speaker
void PCSpeaker_StopSound();

// Plays the specified sound on the PC speaker
void PCSpeaker_PlaySound(uint8_t* data, uint32_t length);

// Stops a sample playing on the PC speaker
void PCSpeaker_StopSample();

// Plays the specified sample on the PC speaker
void PCSpeaker_PlaySample(uint8_t* data, uint32_t length);

// Determines if a PC speaker sound is playing
uint8_t PCSpeaker_SoundPlaying();

// Determines if there's a SoundSource
uint8_t SoundSource_Detect();

// Turns off the Sound Source
void SoundSource_Shut();

// Plays the specified sample on the Sound Source
void SoundSource_PlaySample(uint8_t* data, uint32_t length);

// Stops a sample playing on the Sound Source
void SoundSource_StopSample();

