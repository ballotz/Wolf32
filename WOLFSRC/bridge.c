#include "bridge.h"

//------------------------------------------------------------------------------
// VGA
//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
// Keyboard
//------------------------------------------------------------------------------

void Update_Key()
{

}

//------------------------------------------------------------------------------
// Mouse
//------------------------------------------------------------------------------

void INL_GetMouseDelta(int16_t* dx, int16_t* dy)
{

}

void Mouse_ResetDelta()
{

}

uint16_t INL_GetMouseButtons(void)
{

}

uint8_t IN_HasMouse()
{
    return 0;
}

void Mouse_SetPos(int16_t x, int16_t y)
{

}

void Mouse_GetPos(int16_t* x, int16_t* y)
{

}

//------------------------------------------------------------------------------
// Joystick
//------------------------------------------------------------------------------

void IN_GetJoyAbs(uint16_t joy, uint16_t* xp, uint16_t* yp)
{

}

uint16_t INL_GetJoyButtons(uint16_t joy)
{

}

//------------------------------------------------------------------------------
// VR
//------------------------------------------------------------------------------

int16_t VR_GetAngle()
{

}

//------------------------------------------------------------------------------
// TimeCount
//------------------------------------------------------------------------------

uint32_t TimeCount;

void Update_Time()
{

}

//------------------------------------------------------------------------------
// AdLib
//------------------------------------------------------------------------------

void AdLib_StartMusic(uint16_t* values, uint16_t length)
{

}

void AdLib_MusicOff()
{

}

uint8_t AdLib_Detect()
{
    return 0;
}

void AdLib_Clean()
{

}

void AdLib_Shut()
{

}

void AdLib_PlaySound(BridgeAdLibSound* sound)
{

}

void AdLib_StopSound()
{

}

uint8_t AdLib_SoundPlaying()
{

}

//------------------------------------------------------------------------------
// PC Speaker
//------------------------------------------------------------------------------

void PCSpeaker_Shut()
{

}

void PCSpeaker_StopSound()
{

}

void PCSpeaker_PlaySound(uint8_t* data, uint32_t length)
{

}

void PCSpeaker_StopSample()
{

}

void PCSpeaker_PlaySample(uint8_t* data, uint32_t length)
{

}

uint8_t PCSpeaker_SoundPlaying()
{

}

//------------------------------------------------------------------------------
// SoundSource
//------------------------------------------------------------------------------

uint8_t SoundSource_Detect()
{
    return 0;
}

void SoundSource_Shut()
{

}

void SoundSource_PlaySample(uint8_t* data, uint32_t length)
{

}

void SoundSource_StopSample()
{

}

//------------------------------------------------------------------------------
// SoundBlaster
//------------------------------------------------------------------------------

uint8_t SoundBlaster_Detect()
{
    return 0;
}

void SoundBlaster_Level(int16_t left, int16_t right)
{

}

void SoundBlaster_PlaySample(uint8_t* data, uint32_t length)
{

}

void SoundBlaster_StopSample()
{

}

//------------------------------------------------------------------------------

#include <windows.h>

int __stdcall WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nShowCmd
)
{
    main(0, "");
}
