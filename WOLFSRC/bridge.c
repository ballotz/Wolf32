#include "bridge.h"
#include <SDL.h>

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* frame_texture;

int width = 320;
int height = 200;

uint8_t keyboard_map[SDL_NUM_SCANCODES];

Uint64 time_count;

void InitKeyMap()
{
    keyboard_map[SDL_SCANCODE_ESCAPE] = 0x01;
    keyboard_map[SDL_SCANCODE_Y] = 0x15;
    keyboard_map[SDL_SCANCODE_LCTRL] = 0x1D;
    keyboard_map[SDL_SCANCODE_RCTRL] = 0x1D;
    keyboard_map[SDL_SCANCODE_N] = 0x31;
    keyboard_map[SDL_SCANCODE_SPACE] = 0x39;
    keyboard_map[SDL_SCANCODE_F1] = 0x3B;
    keyboard_map[SDL_SCANCODE_F2] = 0x3C;
    keyboard_map[SDL_SCANCODE_F3] = 0x3D;
    keyboard_map[SDL_SCANCODE_F4] = 0x3E;
    keyboard_map[SDL_SCANCODE_F5] = 0x3F;
    keyboard_map[SDL_SCANCODE_F6] = 0x40;
    keyboard_map[SDL_SCANCODE_F7] = 0x41;
    keyboard_map[SDL_SCANCODE_F8] = 0x42;
    keyboard_map[SDL_SCANCODE_F9] = 0x43;
    keyboard_map[SDL_SCANCODE_F10] = 0x44;
    keyboard_map[SDL_SCANCODE_UP] = 0x48;
    keyboard_map[SDL_SCANCODE_LEFT] = 0xCB;
    keyboard_map[SDL_SCANCODE_UP] = 0x48;
    keyboard_map[SDL_SCANCODE_RIGHT] = 0x4D;
    keyboard_map[SDL_SCANCODE_DOWN] = 0x50;
    keyboard_map[SDL_SCANCODE_RETURN] = 0x1C;
}

void Initialize()
{
    // attempt to initialize SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
        return 1; // SDL init fail

    SDL_DisplayMode mode;
    if (SDL_GetDesktopDisplayMode(0, &mode) != 0)
        return 1;

    // init the window
    window = SDL_CreateWindow("Wolf3D",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_SHOWN
        //SDL_WINDOW_FULLSCREEN_DESKTOP
        //SDL_WINDOW_FULLSCREEN
    );
    if (window == 0)
        return 1; // window init fail

    renderer = SDL_CreateRenderer(
        window,
        -1,
        0
        //SDL_RENDERER_PRESENTVSYNC
    );
    if (renderer == 0)
        return 1; // renderer init fail

    frame_texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        width,
        height);
    if (frame_texture == 0)
        return 1;

    void* pixels;
    int pitch;
    SDL_LockTexture(frame_texture, 0, &pixels, &pitch);
    SDL_UnlockTexture(frame_texture);

    //SDL_SetRelativeMouseMode(SDL_TRUE);
    //SDL_GetRelativeMouseState(0, 0); // flush first

    //SDL_ShowCursor(SDL_DISABLE);

    InitKeyMap();

    time_count = SDL_GetPerformanceCounter();
}

void Deinitialize()
{
    //SDL_ShowCursor(SDL_ENABLE);

    // clean up SDL
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();

    return 0;
}

int SDL_main(int argc, char* argv[])
{
    Initialize();
    Main(argc, argv);
}

//------------------------------------------------------------------------------

void QuitHook()
{
    Deinitialize();
}

//------------------------------------------------------------------------------
// VGA
//------------------------------------------------------------------------------

void VGA_Update(
    uint8_t* frame,
    int16_t width,
    int16_t height,
    uint8_t* palette)
{
    uint32_t* pixels;
    int vgastride;
    int pitch, x, y, color;

    SDL_LockTexture(frame_texture, 0, &pixels, &pitch);
    pitch /= sizeof(*pixels);

    vgastride = width >> 2;
    for (y = 0; y < height; ++y)
    {
        for (x = 0; x < width; ++x)
        {
            // addressing the vga memory
            color = frame[(x >> 2) + y * vgastride + (x & 3) * 0x10000];
            // extend vga palette from 6 to 8 bit
            pixels[x + y * pitch] =
                (palette[color * 3 + 0] << 18) +
                (palette[color * 3 + 1] << 10) +
                (palette[color * 3 + 2] <<  2);
        }
    }

    SDL_UnlockTexture(frame_texture);

    SDL_RenderCopy(renderer, frame_texture, NULL, NULL);
    SDL_RenderPresent(renderer); // draw to the screen
}

//------------------------------------------------------------------------------
// Keyboard
//------------------------------------------------------------------------------

void Keyboard_Update()
{
    SDL_Event event;
    SDL_Scancode code;
    uint8_t key;

    SDL_PumpEvents();
    if (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_KEYDOWN, SDL_KEYUP))
    {
        code = event.key.keysym.scancode;

        if (code == SDL_SCANCODE_PAUSE)
        {
            INL_KeyService_ISR(0xE1);
            INL_KeyService_ISR(0x1D);
            INL_KeyService_ISR(0x45);
            INL_KeyService_ISR(0xE1);
            INL_KeyService_ISR(0x9D);
            INL_KeyService_ISR(0xC5);
        }
        else
        {
            key = keyboard_map[code];

            if (key)
            {
                if (code == SDL_SCANCODE_SLASH ||
                    code == SDL_SCANCODE_RETURN ||
                    code == SDL_SCANCODE_INSERT ||
                    code == SDL_SCANCODE_DELETE ||
                    code == SDL_SCANCODE_HOME ||
                    code == SDL_SCANCODE_END ||
                    code == SDL_SCANCODE_PAGEUP ||
                    code == SDL_SCANCODE_PAGEDOWN ||
                    code == SDL_SCANCODE_LEFT ||
                    code == SDL_SCANCODE_RIGHT ||
                    code == SDL_SCANCODE_UP ||
                    code == SDL_SCANCODE_DOWN ||
                    code == SDL_SCANCODE_RALT ||
                    code == SDL_SCANCODE_RCTRL)
                {
                    INL_KeyService_ISR(0xE0);
                }

                if (event.key.type == SDL_KEYUP)
                    key |= 0x80;

                INL_KeyService_ISR(key);
            }
        }
    }
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

Uint64 timer70hz_offset;

Uint64 Timer70Hz()
{
    Uint64 counter = SDL_GetPerformanceCounter();
    return counter / (SDL_GetPerformanceFrequency() / 70);
}

uint32_t TimeCount_Get()
{
    return (uint32_t)(Timer70Hz() - timer70hz_offset);
}

void TimeCount_Set(uint32_t value)
{
    timer70hz_offset = Timer70Hz() + value;
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

