#include "bridge.h"
#include <SDL.h>

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* frame_texture;

int src_width = 320;
int src_height = 200;
int dst_width = 800;
int dst_height = 600;

uint8_t keyboard_map[SDL_NUM_SCANCODES];

Uint64 time_count;

void InitKeyMap()
{
    keyboard_map[SDL_SCANCODE_ESCAPE] = 0x01;
    keyboard_map[SDL_SCANCODE_Y] = 0x15;
    keyboard_map[SDL_SCANCODE_LCTRL] = 0x1D;
    keyboard_map[SDL_SCANCODE_RCTRL] = 0x1D;
    keyboard_map[SDL_SCANCODE_LSHIFT] = 0x2A;
    keyboard_map[SDL_SCANCODE_N] = 0x31;
    keyboard_map[SDL_SCANCODE_RSHIFT] = 0x36;
    keyboard_map[SDL_SCANCODE_LALT] = 0x38;
    keyboard_map[SDL_SCANCODE_RALT] = 0x38;
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
    keyboard_map[SDL_SCANCODE_LEFT] = 0x4B;
    keyboard_map[SDL_SCANCODE_UP] = 0x48;
    keyboard_map[SDL_SCANCODE_RIGHT] = 0x4D;
    keyboard_map[SDL_SCANCODE_DOWN] = 0x50;
    keyboard_map[SDL_SCANCODE_RETURN] = 0x1C;
}

void Initialize()
{
    // attempt to initialize SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
        exit(1); // SDL init fail

    SDL_DisplayMode mode;
    if (SDL_GetDesktopDisplayMode(0, &mode) != 0)
        exit(1);

    // init the window
    window = SDL_CreateWindow("Wolf3D",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        dst_width,
        dst_height,
        SDL_WINDOW_SHOWN
        //SDL_WINDOW_FULLSCREEN_DESKTOP
        //SDL_WINDOW_FULLSCREEN
    );
    if (window == 0)
        exit(1); // window init fail

    renderer = SDL_CreateRenderer(
        window,
        -1,
        0
        //SDL_RENDERER_PRESENTVSYNC
        //SDL_RENDERER_ACCELERATED
    );
    if (renderer == 0)
        exit(1); // renderer init fail

    frame_texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        src_width,
        src_height);
    if (frame_texture == 0)
        exit(1);

    SDL_SetTextureScaleMode(frame_texture, SDL_ScaleModeLinear);

    void* pixels;
    int pitch;
    SDL_LockTexture(frame_texture, 0, &pixels, &pitch);
    SDL_UnlockTexture(frame_texture);

    //SDL_SetRelativeMouseMode(SDL_TRUE);
    //SDL_GetRelativeMouseState(0, 0); // flush first

    SDL_ShowCursor(SDL_DISABLE);

    InitKeyMap();

    time_count = SDL_GetPerformanceCounter();
}

void Deinitialize()
{
    SDL_ShowCursor(SDL_ENABLE);

    // clean up SDL
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
}

int SDL_main(int argc, char* argv[])
{
    Initialize();
    Main(argc, argv);
    return 0;
}

//------------------------------------------------------------------------------
// System
//------------------------------------------------------------------------------

void Exit(int code)
{
    Deinitialize();
    exit(code);
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

void Mouse_GetDelta(int16_t* dx, int16_t* dy)
{
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    SDL_WarpMouseInWindow(window, dst_width / 2, dst_height / 2);
    *dx = mx - dst_width / 2;
    *dy = my - dst_height / 2;
}

void Mouse_ResetDelta()
{
    SDL_WarpMouseInWindow(window, dst_width / 2, dst_height / 2);
}

uint16_t Mouse_GetButtons(void)
{
    int mx, my;
    Uint32 buttons = SDL_GetMouseState(&mx, &my);

    return
        ((1 << 0) & ~(((buttons & SDL_BUTTON_LMASK) != 0) - 1)) +
        ((1 << 1) & ~(((buttons & SDL_BUTTON_RMASK) != 0) - 1));
}

uint8_t Mouse_Detect()
{
    return 1;
}

void Mouse_SetPos(int16_t x, int16_t y)
{
    SDL_WarpMouseInWindow(
        window,
        (int)((float)x / 240 * dst_width),
        (int)((float)y / 240 * dst_height));
}

void Mouse_GetPos(int16_t* x, int16_t* y)
{
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    *x = (int16_t)((float)mx / dst_width * 240);
    *y = (int16_t)((float)my / dst_height * 240);
}

//------------------------------------------------------------------------------
// Joystick
//------------------------------------------------------------------------------

void IN_GetJoyAbs(uint16_t joy, uint16_t* xp, uint16_t* yp)
{
    *xp = 0;
    *yp = 0;
}

uint16_t INL_GetJoyButtons(uint16_t joy)
{
    return 0;   
}

//------------------------------------------------------------------------------
// VR
//------------------------------------------------------------------------------

int16_t VR_GetAngle()
{
    return 0;
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
    timer70hz_offset = Timer70Hz() - value;
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
    return 0;
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
    return 0;
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

