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

SDL_AudioDeviceID audio_device;
SDL_AudioSpec audio_format;
SDL_AudioStream *pc_sd_stream, *sb_stream;
uint8_t pc_sd_playing, sb_playing;
float sb_level[2];
typedef struct
{
    float coeff[5];
    float state[4];
} lpfilter2_t;
lpfilter2_t sb_filter[2];

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

void CalcLP(lpfilter2_t* filter, float f0, float q, float sr)
{
    float w0 = 2.f * 3.1416f * f0 / sr;
    float sinw0 = sinf(w0);
    float cosw0 = cosf(w0);
    float a = sinw0 / (2.f * q);
    float a0 = 1.f + a;
    float a0inv = 1.f / a0;
    filter->coeff[0] = (1 - cosw0) * 0.5f * a0inv;
    filter->coeff[1] = (1 - cosw0) * a0inv;
    filter->coeff[2] = (1 - cosw0) * 0.5f * a0inv;
    filter->coeff[3] = -2.f * cosw0 * a0inv;
    filter->coeff[4] = (1.f - a) * a0inv;
}

float ProcessLP(lpfilter2_t* filter, float input)
{
    float output = input * filter->coeff[0] +
        filter->state[0] * filter->coeff[1] +
        filter->state[1] * filter->coeff[2] -
        filter->state[2] * filter->coeff[3] -
        filter->state[3] * filter->coeff[4];
    filter->state[3] = filter->state[2];
    filter->state[2] = output;
    filter->state[1] = filter->state[1];
    filter->state[0] = input;
    return output;
}

#define PC_SD_RATE          140
#define PC_SQUARE_V         0.1f    // -20dB
#define PC_SQUARE_CUTOFF    3500.f

void SynthPCSpeakerSound(float* out_data, int out_samples)
{
    int             i, j;
    int             in_read;
    uint8_t         in_data;
    float           out_sample;
    int             in_trigger;
    static int      in_count = 0;
    static int      sqr_trigger = 0;
    static int      sqr_count = 0;
    static float    sqr_val = 0.f;
    static float    lp_state[2] = { 0.f, 0.f };
    float           lp_coeff, coswc;

    in_trigger = audio_format.freq / PC_SD_RATE;

    coswc = cosf(2.f * 3.1416f * (PC_SQUARE_CUTOFF / audio_format.freq));
    lp_coeff = 1.f - (coswc - 1.f + sqrtf((coswc - 1.f) * (coswc - 3.f)));

    for (i = 0, j = 0; i < out_samples; ++i, j += 2)
    {
        // do we need one more sample from input sound?
        if (in_count >= in_trigger)
        {
            in_count = 0;
            sqr_trigger = 0;

            in_read = SDL_AudioStreamGet(pc_sd_stream, &in_data, 1);
            if (in_read == 0)
            {
                if (pc_sd_playing)
                {
                    pc_sd_playing = 0;
                    PCSpeaker_SoundFinished();
                }
            }
            else if (in_data != 0)
            {
                // start (or continue) oscillator, use half period
                sqr_trigger = (int)(in_data * (60.f / 1193181.f) * audio_format.freq) >> 1;
            }
        }

        // generate output
        if (sqr_trigger == 0)
        {
            // off
            out_sample = 0.f;
        }
        else
        {
            // on
            if (sqr_count > sqr_trigger)
            {
                sqr_count = 0;
                sqr_val = (sqr_val > 0.f) ? -PC_SQUARE_V : +PC_SQUARE_V;
            }
            out_sample = sqr_val;
        }

        // low pass
        out_sample += lp_coeff * (lp_state[0] - out_sample);
        lp_state[0] = out_sample;
        out_sample += lp_coeff * (lp_state[1] - out_sample);
        lp_state[1] = out_sample;

        // add to output buffer
        out_data[j + 0] += out_sample;
        out_data[j + 1] += out_sample;

        in_count++;
        sqr_count++;
    }
}

#define SB_RATE     7000

void SynthSoundBlaster(float* out_data, int out_samples)
{
    int             i, j;
    int             in_read;
    uint8_t         in_data;
    float           out_sample;
    int             in_trigger;
    static int      in_count = 0;
    static float    interp[2] = { 0.f, 0.f };
    float           frac;

    in_trigger = audio_format.freq / SB_RATE;

    CalcLP(&sb_filter[0], SB_RATE * 0.5f, 1.306563f, (float)audio_format.freq);
    CalcLP(&sb_filter[1], SB_RATE * 0.5f, 0.541196f, (float)audio_format.freq);

    for (i = 0, j = 0; i < out_samples; ++i, j += 2)
    {
        // do we need one more sample from input?
        if (in_count >= in_trigger)
        {
            in_count = 0;
            in_data = 0x80;
            in_read = SDL_AudioStreamGet(sb_stream, &in_data, 1);
            if (in_read == 0)
            {
                if (sb_playing)
                {
                    SoundBlaster_SoundFinished();
                    // check if there is a new chunk
                    in_read = SDL_AudioStreamGet(sb_stream, &in_data, 1);
                    if (in_read == 0)
                    {
                        // no, end of stream
                        sb_playing = 0;
                    }
                }
            }
            // push new sample to interpolation
            interp[0] = interp[1];
            interp[1] = (in_data - 0x80) / (float)0x80;
        }

        // interpolate output
        frac = in_count / (float)in_trigger;
        out_sample = interp[0] * (1.f - frac) + interp[1] * frac;
        out_sample = ProcessLP(&sb_filter[0], out_sample);
        out_sample = ProcessLP(&sb_filter[1], out_sample);

        // add to output buffer
        out_data[j + 0] += out_sample * sb_level[0];
        out_data[j + 1] += out_sample * sb_level[1];

        in_count++;
    }
}

void AudioCallback(void* userdata, Uint8* stream, int len)
{
    float   *buffer = (float*)stream;
    int     samples = len / (sizeof(float) * 2);

    SDL_memset(stream, 0, len);

    if (pc_sd_stream)
        SynthPCSpeakerSound(buffer, samples);

    if (sb_stream)
        SynthSoundBlaster(buffer, samples);
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

    //const char* device_name;
    //for (int i = 0; device_name = SDL_GetAudioDriver(i, 0); ++i)
    //{
    //}
    //device_name = SDL_GetCurrentAudioDriver();

    SDL_AudioSpec want_format;
    SDL_memset(&want_format, 0, sizeof(want_format));
    // require float format and 2 channels
    want_format.freq = 48000;
    want_format.format = AUDIO_F32;
    want_format.channels = 2;
    want_format.samples = 2048;
    want_format.callback = AudioCallback;
    audio_device = SDL_OpenAudioDevice(NULL, 0, &want_format, &audio_format,
        SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_SAMPLES_CHANGE);

    if (audio_device)
    {
        SDL_PauseAudioDevice(audio_device, 0);

        // keep the same input and output format
        // using AudioStream as a threadsafe data queue
        pc_sd_stream = SDL_NewAudioStream(
            AUDIO_U8, 1, PC_SD_RATE,
            AUDIO_U8, 1, PC_SD_RATE);
        sb_stream = SDL_NewAudioStream(
            AUDIO_U8, 1, SB_RATE,
            AUDIO_U8, 1, SB_RATE);
    }
}

void Deinitialize(void)
{
    SDL_ShowCursor(SDL_ENABLE);

    // clean up SDL

    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);

    if (audio_device)
        SDL_CloseAudioDevice(audio_device);
    if (pc_sd_stream)
        SDL_FreeAudioStream(pc_sd_stream);
    if (sb_stream)
        SDL_FreeAudioStream(sb_stream);

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
// PC Speaker
//------------------------------------------------------------------------------

void PCSpeaker_Shut(void)
{
    SDL_AudioStreamClear(pc_sd_stream);
}

void PCSpeaker_PlaySound(uint8_t* data, uint32_t length)
{
    if (pc_sd_stream)
    {
        SDL_AudioStreamClear(pc_sd_stream);
        pc_sd_playing = 1;
        SDL_AudioStreamPut(pc_sd_stream, data, length);
    }
}

void PCSpeaker_StopSound(void)
{
    SDL_AudioStreamClear(pc_sd_stream);
}

uint8_t PCSpeaker_SoundPlaying(void)
{
    return pc_sd_playing;
}

void PCSpeaker_PlaySample(uint8_t* data, uint32_t length)
{

}

void PCSpeaker_StopSample(void)
{

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
    return 1;
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
    return (sb_stream != 0);
}

void SoundBlaster_Level(int16_t left, int16_t right)
{
    sb_level[0] = left / 15.f;
    sb_level[1] = right / 15.f;
}

void SoundBlaster_PlaySample(uint8_t* data, uint32_t length)
{
    sb_playing = 1;
    SDL_AudioStreamPut(sb_stream, data, length);
}

void SoundBlaster_StopSample(void)
{
    SDL_AudioStreamClear(sb_stream);
}
