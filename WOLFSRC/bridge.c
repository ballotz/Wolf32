#include "bridge.h"
#include <SDL.h>
#include <stdio.h>

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* frame_texture;

int src_width = 320;
int src_height = 200;
int dst_width = 800;
int dst_height = 600;

uint8_t* keyboard_map[SDL_NUM_SCANCODES];

Uint64 time_count;

void InitKeyMap(void)
{
    keyboard_map[SDL_SCANCODE_ESCAPE] = "\x01";
    keyboard_map[SDL_SCANCODE_1] = "\x02";
    keyboard_map[SDL_SCANCODE_2] = "\x03";
    keyboard_map[SDL_SCANCODE_3] = "\x04";
    keyboard_map[SDL_SCANCODE_4] = "\x05";
    keyboard_map[SDL_SCANCODE_5] = "\x06";
    keyboard_map[SDL_SCANCODE_6] = "\x07";
    keyboard_map[SDL_SCANCODE_7] = "\x08";
    keyboard_map[SDL_SCANCODE_8] = "\x09";
    keyboard_map[SDL_SCANCODE_9] = "\x0A";
    keyboard_map[SDL_SCANCODE_0] = "\x0B";
    keyboard_map[SDL_SCANCODE_BACKSPACE] = "\x0E";
    keyboard_map[SDL_SCANCODE_TAB] = "\x0F";
    keyboard_map[SDL_SCANCODE_Q] = "\x10";
    keyboard_map[SDL_SCANCODE_W] = "\x11";
    keyboard_map[SDL_SCANCODE_E] = "\x12";
    keyboard_map[SDL_SCANCODE_R] = "\x13";
    keyboard_map[SDL_SCANCODE_T] = "\x14";
    keyboard_map[SDL_SCANCODE_Y] = "\x15";
    keyboard_map[SDL_SCANCODE_U] = "\x16";
    keyboard_map[SDL_SCANCODE_I] = "\x17";
    keyboard_map[SDL_SCANCODE_O] = "\x18";
    keyboard_map[SDL_SCANCODE_P] = "\x19";
    keyboard_map[SDL_SCANCODE_RETURN] = "\x1C";
    keyboard_map[SDL_SCANCODE_LCTRL] = "\x1D";
    keyboard_map[SDL_SCANCODE_A] = "\x1E";
    keyboard_map[SDL_SCANCODE_S] = "\x1F";
    keyboard_map[SDL_SCANCODE_D] = "\x20";
    keyboard_map[SDL_SCANCODE_F] = "\x21";
    keyboard_map[SDL_SCANCODE_G] = "\x22";
    keyboard_map[SDL_SCANCODE_H] = "\x23";
    keyboard_map[SDL_SCANCODE_J] = "\x24";
    keyboard_map[SDL_SCANCODE_K] = "\x25";
    keyboard_map[SDL_SCANCODE_L] = "\x26";
    keyboard_map[SDL_SCANCODE_LSHIFT] = "\x2A";
    keyboard_map[SDL_SCANCODE_Z] = "\x2C";
    keyboard_map[SDL_SCANCODE_X] = "\x2D";
    keyboard_map[SDL_SCANCODE_C] = "\x2E";
    keyboard_map[SDL_SCANCODE_V] = "\x2F";
    keyboard_map[SDL_SCANCODE_B] = "\x30";
    keyboard_map[SDL_SCANCODE_N] = "\x31";
    keyboard_map[SDL_SCANCODE_M] = "\x32";
    keyboard_map[SDL_SCANCODE_RSHIFT] = "\x36";
    keyboard_map[SDL_SCANCODE_LALT] = "\x38";
    keyboard_map[SDL_SCANCODE_SPACE] = "\x39";
    keyboard_map[SDL_SCANCODE_CAPSLOCK] = "\x3A";
    keyboard_map[SDL_SCANCODE_F1] = "\x3B";
    keyboard_map[SDL_SCANCODE_F2] = "\x3C";
    keyboard_map[SDL_SCANCODE_F3] = "\x3D";
    keyboard_map[SDL_SCANCODE_F4] = "\x3E";
    keyboard_map[SDL_SCANCODE_F5] = "\x3F";
    keyboard_map[SDL_SCANCODE_F6] = "\x40";
    keyboard_map[SDL_SCANCODE_F7] = "\x41";
    keyboard_map[SDL_SCANCODE_F8] = "\x42";
    keyboard_map[SDL_SCANCODE_F9] = "\x43";
    keyboard_map[SDL_SCANCODE_F10] = "\x44";
    keyboard_map[SDL_SCANCODE_F11] = "\x57";
    keyboard_map[SDL_SCANCODE_F12] = "\x58";
    keyboard_map[SDL_SCANCODE_RCTRL] = "\xE0\x1D";
    keyboard_map[SDL_SCANCODE_RALT] = "\xE0\x38";
    keyboard_map[SDL_SCANCODE_UP] = "\xE0\x48";
    keyboard_map[SDL_SCANCODE_LEFT] = "\xE0\x4B";
    keyboard_map[SDL_SCANCODE_RIGHT] = "\xE0\x4D";
    keyboard_map[SDL_SCANCODE_DOWN] = "\xE0\x50";
    keyboard_map[SDL_SCANCODE_INSERT] = "\xE0\x52";
    keyboard_map[SDL_SCANCODE_DELETE] = "\xE0\x53";
    keyboard_map[SDL_SCANCODE_PAUSE] = "\xE1\x1D\x45\xE1\x9D\xC5";
}
}

void Initialize(void)
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

void Deinitialize(void)
{
    SDL_ShowCursor(SDL_ENABLE);

    // clean up SDL
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
}

#ifndef __APPLE__
int SDL_main(int argc, char* argv[])
{
    Initialize();
    Main(argc, argv);
    return 0;
}
#else
int main(int argc, const char *argv[])
{
    Initialize();
    Main(argc, argv);
    return 0;
}
#endif


//------------------------------------------------------------------------------
// System
//------------------------------------------------------------------------------

void Exit(int code)
{
    Deinitialize();
    exit(code);
}

//------------------------------------------------------------------------------
// FileSystem
//------------------------------------------------------------------------------

void FileSystem_Remove(const char* name)
{
    remove(name);
}

void FileSystem_GetMode(int32_t options, char* mode)
{
    int index = 0;
    if (options & FileSystemRead)
        mode[index++] = 'r';
    if (options & FileSystemWrite)
        mode[index++] = 'w';
    if (options & FileSystemBinary)
        mode[index++] = 'b';
    mode[index++] = '\0';
}

//char searchpath[] = "../../../";
char searchpath[] = "./";

FileSystemHandle FileSystem_Open(const char* name, int32_t options)
{
    FileSystemHandle ret;
    char mode[4];
    char path[256];
    
    snprintf(path, sizeof(path), "%s%s", searchpath, name);
    FileSystem_GetMode(options, mode);
    ret.internal = (uintptr_t)fopen(path, mode);
    return ret;
}

void FileSystem_Close(FileSystemHandle handle)
{
    fclose((FILE*)handle.internal);
}

uint8_t FileSystem_ValidHandle(FileSystemHandle handle)
{
    return (handle.internal != 0);
}

int32_t FileSystem_Seek(FileSystemHandle handle, int32_t position)
{
    if (fseek((FILE*)handle.internal, position, SEEK_SET) == 0)
        return position;
    else
        return -1;
}

size_t FileSystem_Read(FileSystemHandle handle, void* buffer, size_t size)
{
    return fread(buffer, size, 1, (FILE*)handle.internal) * size;
}

size_t FileSystem_Write(FileSystemHandle handle, void* buffer, size_t size)
{
    return fwrite(buffer, size, 1, (FILE*)handle.internal);
}

int32_t FileSystem_FileLength(FileSystemHandle handle)
{
    int32_t size;
    if (fseek((FILE*)handle.internal, 0L, SEEK_END) != 0)
        return 0;
    size = ftell((FILE*)handle.internal);
    if (fseek((FILE*)handle.internal, 0L, SEEK_SET) != 0)
        return 0;
    return size;
}

uint8_t FileSystem_FileExisit(const char* name)
{
    FileSystemHandle h = FileSystem_Open(name, FileSystemRead);
    if (FileSystem_ValidHandle(h))
    {
        FileSystem_Close(h);
        return 1;
    }
    return 0;
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

void Keyboard_Update(void)
{
    SDL_Event event;
    SDL_Scancode code;
    uint8_t* sequence;
    uint8_t key;

    SDL_PumpEvents();
    if (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_KEYDOWN, SDL_KEYUP))
    {
        code = event.key.keysym.scancode;

        sequence = keyboard_map[code];
        if (sequence)
        {
            if (event.key.type == SDL_KEYDOWN)
            {
                while (key = *sequence++)
                    INL_KeyService_ISR(key);
            }
            else
            {
                if (code == SDL_SCANCODE_PAUSE)
                    return;
                while (key = *sequence++)
                {
                    if (key == 0xE0)
                        INL_KeyService_ISR(key);
                    else
                        INL_KeyService_ISR(key | 0x80);
                }
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

void Mouse_ResetDelta(void)
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

uint8_t Mouse_Detect(void)
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

int16_t VR_GetAngle(void)
{
    return 0;
}

//------------------------------------------------------------------------------
// TimeCount
//------------------------------------------------------------------------------

Uint64 timer70hz_offset;

Uint64 Timer70Hz(void)
{
    Uint64 counter = SDL_GetPerformanceCounter();
    return counter / (SDL_GetPerformanceFrequency() / 70);
}

uint32_t TimeCount_Get(void)
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

void AdLib_MusicOff(void)
{

}

uint8_t AdLib_Detect(void)
{
    return 0;
}

void AdLib_Clean(void)
{

}

void AdLib_Shut(void)
{

}

void AdLib_PlaySound(BridgeAdLibSound* sound)
{

}

void AdLib_StopSound(void)
{

}

uint8_t AdLib_SoundPlaying(void)
{
    return 0;
}

//------------------------------------------------------------------------------
// PC Speaker
//------------------------------------------------------------------------------

void PCSpeaker_Shut(void)
{

}

void PCSpeaker_StopSound(void)
{

}

void PCSpeaker_PlaySound(uint8_t* data, uint32_t length)
{

}

void PCSpeaker_StopSample(void)
{

}

void PCSpeaker_PlaySample(uint8_t* data, uint32_t length)
{

}

uint8_t PCSpeaker_SoundPlaying(void)
{
    return 0;
}

//------------------------------------------------------------------------------
// SoundSource
//------------------------------------------------------------------------------

uint8_t SoundSource_Detect(void)
{
    return 0;
}

void SoundSource_Shut(void)
{

}

void SoundSource_PlaySample(uint8_t* data, uint32_t length)
{

}

void SoundSource_StopSample(void)
{

}

//------------------------------------------------------------------------------
// SoundBlaster
//------------------------------------------------------------------------------

uint8_t SoundBlaster_Detect(void)
{
    return 0;
}

void SoundBlaster_Level(int16_t left, int16_t right)
{

}

void SoundBlaster_PlaySample(uint8_t* data, uint32_t length)
{

}

void SoundBlaster_StopSample(void)
{

}

