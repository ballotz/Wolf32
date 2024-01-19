#pragma once
#include <stdint.h>

//------------------------------------------------------------------------------
// System
//------------------------------------------------------------------------------

extern int Main(int argc, char* argv[]);
extern void Quit(char* error);

void Exit(int code);

//------------------------------------------------------------------------------
// FileSystem
//------------------------------------------------------------------------------

typedef struct
{
    uintptr_t internal;
} FileSystemHandle;

enum
{
    FileSystemCreate = 1,
    FileSystemRead = 2,
    FileSystemWrite = 4,
    FileSystemBinary = 8,
    FileSystemText = 16,
};

void FileSystem_Remove(const char* name);

FileSystemHandle FileSystem_Open(const char* name, int32_t options);

void FileSystem_Close(FileSystemHandle handle);

uint8_t FileSystem_ValidHandle(FileSystemHandle handle);

int32_t FileSystem_Seek(FileSystemHandle handle, int32_t position);

size_t FileSystem_Read(FileSystemHandle handle, void* buffer, size_t size);

size_t FileSystem_Write(FileSystemHandle handle, void* buffer, size_t size);

int32_t FileSystem_FileLength(FileSystemHandle handle);

uint8_t FileSystem_FileExisit(const char* name);

//------------------------------------------------------------------------------
// VGA
//------------------------------------------------------------------------------

void VGA_Update(
    uint8_t* frame,
    int16_t width,
    int16_t height,
    uint8_t* palette);

//------------------------------------------------------------------------------
// Keyboard
//------------------------------------------------------------------------------

extern void INL_KeyService_ISR(uint8_t key);

// request keyboard update trough INL_KeyService_ISR()
void Keyboard_Update(void);

//------------------------------------------------------------------------------
// Mouse
//------------------------------------------------------------------------------

// Gets the amount that the mouse has moved
void Mouse_GetDelta(int16_t* dx, int16_t* dy);

// Reset the amount that the mouse has moved
void Mouse_ResetDelta(void);

// Gets the status of the mouse buttons
uint16_t Mouse_GetButtons(void);

// 0, no mouse
// 1, mouse available
uint8_t Mouse_Detect(void);

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

int16_t VR_GetAngle(void);

//------------------------------------------------------------------------------
// TimeCount (70Hz)
//------------------------------------------------------------------------------

uint32_t TimeCount_Get(void);

void TimeCount_Set(uint32_t value);

//------------------------------------------------------------------------------
// AdLib
//------------------------------------------------------------------------------

// starts playing the music pointed to
void AdLib_StartMusic(uint16_t* values, uint16_t length);

// turns off the sequencer and any playing notes
void AdLib_MusicOff(void);

// Determines if there's an AdLib
uint8_t AdLib_Detect(void);

// Totally shuts down the AdLib card
void AdLib_Clean(void);

// Shuts down the AdLib card for sound effects
void AdLib_Shut(void);

#pragma pack(push, 2)
typedef	struct
{
    uint32_t    length;
    uint16_t    priority;
} BridgeSoundCommon;
#pragma pack(pop)

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
void AdLib_StopSound(void);

// Determines if an AdLib sound is playing
uint8_t AdLib_SoundPlaying(void);

void AdLib_SoundFinished(void);

//------------------------------------------------------------------------------
// PC Speaker
//------------------------------------------------------------------------------

// Turns off the pc speaker
void PCSpeaker_Shut(void);

// Stops the current sound playing on the PC Speaker
void PCSpeaker_StopSound(void);

// Plays the specified sound on the PC speaker
// When finished must call PCSpeaker_SoundFinished()
void PCSpeaker_PlaySound(uint8_t* data, uint32_t length);

// Stops a sample playing on the PC speaker
void PCSpeaker_StopSample(void);

// Plays the specified sample on the PC speaker
void PCSpeaker_PlaySample(uint8_t* data, uint32_t length);

// Determines if a PC speaker sound is playing
uint8_t PCSpeaker_SoundPlaying(void);

void PCSpeaker_SoundFinished(void);

//------------------------------------------------------------------------------
// SoundSource
//------------------------------------------------------------------------------

// Determines if there's a SoundSource
uint8_t SoundSource_Detect(void);

// Turns off the Sound Source
void SoundSource_Shut(void);

// Plays the specified sample on the Sound Source
// When finished must call SoundSource_SoundFinished()
void SoundSource_PlaySample(uint8_t* data, uint32_t length);

// Stops a sample playing on the Sound Source
void SoundSource_StopSample(void);

void SoundSource_SoundFinished(void);

//------------------------------------------------------------------------------
// SoundBlaster
//------------------------------------------------------------------------------

// Determines if there's a SoundBlaster
uint8_t SoundBlaster_Detect(void);

// Sets the attenuation levels for the left and right channels
void SoundBlaster_Level(int16_t left, int16_t right);

// Plays a sampled sound on the SoundBlaster
// When finished must call SoundBlaster_SoundFinished()
void SoundBlaster_PlaySample(uint8_t* data, uint32_t length);

// Stops any active sampled sound
void SoundBlaster_StopSample(void);

void SoundBlaster_SoundFinished(void);

