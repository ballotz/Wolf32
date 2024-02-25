#include "../WOLFSRC/bridge.h"
#include "../spscbuffer/spscbuffer.h"
#include <SDL.h>
#include <stdio.h>

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* frame_texture;

#define src_width   320
#define src_height  200
// windowed
int dst_width = 800;
int dst_height = 600;
// fullscreen
//int dst_width = 0;
//int dst_height = 0;
int port_width;
int port_height;

uint8_t* keyboard_map[SDL_NUM_SCANCODES];

int mouse_x;
int mouse_y;

Uint64 time_count;

// adlib emulation library
#define ADLIB_emu8950
//#define ADLIB_woody_opl

#ifdef ADLIB_emu8950
#include "../emu8950/emu8950.h"
OPL* ym3812;
#define alInit(sr)                  \
    ym3812 = OPL_new(3579545, sr);  \
    OPL_setChipType(ym3812, 2);
#define alDeinit()                  \
    OPL_delete(ym3812);
#define alOut(a, v)                 \
    OPL_writeReg(ym3812, a, v);
//#define alStatus()                  \
//    OPL_status(ym3812);
#define alSample(v)                 \
    v = OPL_calc(ym3812);
#define alVolAdjust 2.f
#endif
#ifdef ADLIB_woody_opl
#include "../woody-opl/opl.h"
#define alInit(sr)                  \
    adlib_init(sr);
#define alDeinit()
#define alOut(a, v)                 \
    adlib_write(a, v);
//#define alStatus()                  \
//    adlib_reg_read(0);
#define alSample(v)                 \
    adlib_getsample(&v, 1);
#define alVolAdjust 1.f
#endif

//	Register addresses
// Operator stuff
#define	alChar      0x20
#define	alScale     0x40
#define	alAttack    0x60
#define	alSus       0x80
#define	alWave      0xe0
// Channel stuff
#define	alFreqL		0xa0
#define	alFreqH		0xb0
#define	alFeedCon	0xc0
// Global stuff
#define	alEffects   0xbd
//
//	Sequencing stuff
//
#define	sqMaxTracks 10

typedef struct
{
    uint8_t addr;
    uint8_t data;
}
alcmd_t;
#define alMakeCmd(p, a, v)  \
    p.addr = a;             \
    p.data = v;
typedef struct
{
    uint8_t addr;
    uint8_t data;
    uint16_t wait;
}
alpacket_t;

typedef struct
{
    float coeff[3];
    float state[2];
} filter1_t;

void filter1_expma(filter1_t* filter, float f0, float sr)
{
    float coswc = cosf(2.f * 3.1416f * (f0 / sr));
    float coeff = 1.f - (coswc - 1.f + sqrtf((coswc - 1.f) * (coswc - 3.f)));
    filter->coeff[0] = 1.f - coeff;
    filter->coeff[1] = 0.f;
    filter->coeff[2] = -coeff;
}

float filter1_process(filter1_t* filter, float input)
{
    float output = input * filter->coeff[0] +
        filter->state[0] * filter->coeff[1] -
        filter->state[1] * filter->coeff[2];
    filter->state[1] = output;
    filter->state[0] = input;
    return output;
}

typedef struct
{
    float coeff[5];
    float state[4];
} filter2_t;

void filter2_rbj_lp(filter2_t* filter, float f0, float q, float sr)
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

float filter2_process(filter2_t* filter, float input)
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

SDL_AudioDeviceID audio_device;
SDL_AudioSpec audio_format;

spscbuffer_t al_cmd_stream;

SDL_mutex *pc_sd_mutex;
uint8_t* pc_sd_data;
uint16_t pc_sd_size;
filter1_t pc_sd_filter[2];

SDL_mutex *al_music_mutex;
spscbuffer_t al_music_stream;
uint8_t *al_music_data, *al_sequencer_data;
uint16_t al_music_size, al_sequencer_size;
uint8_t al_music_active;
filter2_t al_filter;

SDL_mutex *al_sd_mutex;
spscbuffer_t al_sd_stream;
uint8_t *al_sd_data;
uint16_t al_sd_size;
uint8_t al_sd_block;

SDL_mutex *sb_mutex;
uint8_t* sb_data;
uint16_t sb_size;
filter2_t sb_filter[2];
float sb_level[2];

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

#define PC_SD_RATE          140
#define PC_SD_SHIFT         12
#define PC_SD_FRAC          (1 << PC_SD_SHIFT)
#define PC_SQUARE_VOL       0.05f
#define PC_SQUARE_CUTOFF    3500.f

void SynthPCSpeakerSound(float* out_data, int out_samples)
{
    int             i, j;
    uint8_t         data;
    float           out_sample;
    int             in_trigger;
    static int      in_count = 0;
    static int      sqr_trigger = 0;
    static int      sqr_count = 0;
    static float    sqr_val = 0.f;
    int             lock;

    lock = SDL_TryLockMutex(pc_sd_mutex);

    in_trigger = PC_SD_FRAC * audio_format.freq / PC_SD_RATE;

    for (i = 0, j = 0; i < out_samples; ++i, j += 2)
    {
        // do we need one more sample from input sound?
        if (in_count >= in_trigger)
        {
            in_count -= in_trigger;
            if (lock == 0)
            {
                sqr_trigger = 0;
                if (pc_sd_data)
                {
                    if (pc_sd_size)
                    {
                        data = *pc_sd_data;
                        if (data)
                        {
                            // start (or continue) oscillator, use half period
                            sqr_trigger = (int)(data * (60.f / 1193181.f) * audio_format.freq) >> 1;
                        }
                        pc_sd_size--;
                        pc_sd_data++;
                    }
                    else
                    {
                        pc_sd_data = 0;
                        PCSpeaker_SoundFinished();
                    }
                }
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
            if (sqr_count >= sqr_trigger)
            {
                sqr_count = 0;
                sqr_val = (sqr_val > 0.f) ? -PC_SQUARE_VOL : +PC_SQUARE_VOL;
            }
            out_sample = sqr_val;
        }

        // low pass
        out_sample = filter1_process(&pc_sd_filter[0], out_sample);
        out_sample = filter1_process(&pc_sd_filter[1], out_sample);

        // add to output buffer
        out_data[j + 0] += out_sample;
        out_data[j + 1] += out_sample;

        in_count += PC_SD_FRAC;
        sqr_count++;
    }

    if (lock == 0)
        SDL_UnlockMutex(pc_sd_mutex);
}

#define ADLIB_RATE      700
#define ADLIB_SD_SHIFT  12
#define ADLIB_SD_FRAC   (1 << ADLIB_SD_SHIFT)
#define ADLIB_VOL       1.f
#define ADLIB_CUTOFF    3500.f

void SynthAdLib(float* out_data, int out_samples)
{
    int             i, j;
    int             read;
    alcmd_t         cmd;
    alpacket_t      pak;
    uint8_t         snd;
    int16_t         out_sample_i;
    float           out_sample_f;
    static int      music_in_trigger = 0;
    int             sound_in_trigger;
    static int      music_in_count = 0;
    static int      sound_in_count = 0;
    int             music_lock, sound_lock;

    // process commands
    for (;;)
    {
        read = spscbuffer_read(&al_cmd_stream, &cmd, sizeof(alcmd_t));
        if (read == sizeof(alcmd_t))
            alOut(cmd.addr, cmd.data)
        else
            break;
    }

    music_lock = SDL_TryLockMutex(al_music_mutex);
    sound_lock = SDL_TryLockMutex(al_sd_mutex);
    
    if (music_lock == 0)
    {
        // process music commands
        for (;;)
        {
            read = spscbuffer_read(&al_music_stream, &cmd, sizeof(alcmd_t));
            if (read == sizeof(alcmd_t))
                alOut(cmd.addr, cmd.data)
            else
                break;
        }
    }
    
    if (sound_lock == 0)
    {
        // process sound commands
        for (;;)
        {
            read = spscbuffer_read(&al_sd_stream, &cmd, sizeof(alcmd_t));
            if (read == sizeof(alcmd_t))
                alOut(cmd.addr, cmd.data)
            else
                break;
        }
    }

    sound_in_trigger = ADLIB_SD_FRAC * 5 * audio_format.freq / ADLIB_RATE;

    for (i = 0, j = 0; i < out_samples; ++i, j += 2)
    {
        // do we need more data from music input?
        if (music_in_count >= music_in_trigger)
        {
            music_in_count = 0;
            if (al_music_active)
            {
                if (music_lock == 0)
                {
                    music_in_trigger = 0;
                    while (music_in_trigger == 0)
                    {
                        if (al_sequencer_size)
                        {
                            pak = *(alpacket_t*)al_sequencer_data;
                            alOut(pak.addr, pak.data);
                            music_in_trigger = pak.wait * audio_format.freq / ADLIB_RATE;
                            al_sequencer_size -= sizeof(alpacket_t);
                            al_sequencer_data += sizeof(alpacket_t);
                        }
                        if (!al_sequencer_size)
                        {
                            // start over
                            al_sequencer_data = al_music_data;
                            al_sequencer_size = al_music_size;
                        }
                    }
                }
            }
        }

        // do we need more data from sound input?
        if (sound_in_count >= sound_in_trigger)
        {
            sound_in_count -= sound_in_trigger;
            if (sound_lock == 0)
            {
                if (al_sd_data)
                {
                    if (al_sd_size)
                    {
                        snd = *al_sd_data;
                        if (!snd)
                            alOut(alFreqH, 0)
                        else
                        {
                            alOut(alFreqL, snd);
                            alOut(alFreqH, al_sd_block);
                        }
                        al_sd_size--;
                        al_sd_data++;
                    }
                    else
                    {
                        alOut(alFreqH, 0);
                        al_sd_data = 0;
                        AdLib_SoundFinished();
                    }
                }
            }
        }

        // get audio
        alSample(out_sample_i);
        out_sample_f = out_sample_i / (float)0x8000;

        // filter
        out_sample_f = filter2_process(&al_filter, out_sample_f);

        // add to output buffer
        out_data[j + 0] += out_sample_f * (alVolAdjust * ADLIB_VOL);
        out_data[j + 1] += out_sample_f * (alVolAdjust * ADLIB_VOL);

        music_in_count += 1;
        sound_in_count += ADLIB_SD_FRAC;
    }

    if (music_lock == 0)
        SDL_UnlockMutex(al_music_mutex);
    if (sound_lock == 0)
        SDL_UnlockMutex(al_sd_mutex);
}

#define SB_RATE     7000
#define SB_SHIFT    12
#define SB_FRAC     (1 << SB_SHIFT)
#define SB_VOL      0.5f

void SynthSoundBlaster(float* out_data, int out_samples)
{
    int             i, j;
    uint8_t         data;
    float           out_sample;
    int             in_trigger;
    static int      in_count = 0;
    static float    interp[2] = { 0.f, 0.f };
    float           frac;
    int             lock;

    lock = SDL_TryLockMutex(sb_mutex);

    in_trigger = SB_FRAC * audio_format.freq / SB_RATE;

    for (i = 0, j = 0; i < out_samples; ++i, j += 2)
    {
        // do we need one more sample from input?
        if (in_count >= in_trigger)
        {
            in_count -= in_trigger;
            data = 0x80;
            if (lock == 0)
            {
                if (sb_data)
                {
                    if (sb_size)
                    {
                        data = *sb_data;
                        sb_size--;
                        sb_data++;
                    }
                    else
                    {
                        SoundBlaster_SoundFinished();
                        // check if there is a new chunk
                        if (sb_size)
                        {
                            data = *sb_data;
                            sb_size--;
                            sb_data++;
                        }
                    }
                }
            }
            // push new sample to interpolation
            interp[0] = interp[1];
            interp[1] = (data - 0x80) / (float)0x80;
        }

        // interpolate output
        frac = in_count / (float)in_trigger;
        out_sample = interp[0] * (1.f - frac) + interp[1] * frac;
        out_sample = filter2_process(&sb_filter[0], out_sample);
        out_sample = filter2_process(&sb_filter[1], out_sample);

        // add to output buffer
        out_data[j + 0] += out_sample * sb_level[0] * SB_VOL;
        out_data[j + 1] += out_sample * sb_level[1] * SB_VOL;

        in_count += SB_FRAC;
    }

    if (lock == 0)
        SDL_UnlockMutex(sb_mutex);
}

void AudioCallback(void* userdata, Uint8* stream, int len)
{
    float   *buffer = (float*)stream;
    int     samples = len / (sizeof(float) * 2);

    SDL_memset(stream, 0, len);
    SynthPCSpeakerSound(buffer, samples);
    SynthAdLib(buffer, samples);
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
    Uint32 window_flags = 0;
    if (!dst_width || !dst_height)
    {
        window_flags = SDL_WINDOW_FULLSCREEN_DESKTOP;
        dst_width = mode.h / 3 * 4;
        dst_height = mode.h;
        port_width = mode.w;
        port_height = mode.h;
    }
    else
    {
        port_width = dst_width;
        port_height = dst_height;
    }
    window = SDL_CreateWindow("Wolf3D",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        dst_width,
        dst_height,
        SDL_WINDOW_SHOWN |
        window_flags        
        //SDL_WINDOW_FULLSCREEN_DESKTOP
        //SDL_WINDOW_FULLSCREEN
    );
    if (window == 0)
        exit(1); // window init fail

    renderer = SDL_CreateRenderer(
        window,
        -1,
        //0
        SDL_RENDERER_PRESENTVSYNC
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

    SDL_SetRelativeMouseMode(SDL_TRUE);

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
        static uint8_t al_cmd_buffer[512];
        spscbuffer_init(&al_cmd_stream, al_cmd_buffer, sizeof(al_cmd_buffer));
        static uint8_t al_music_buffer[512];
        spscbuffer_init(&al_music_stream, al_music_buffer, sizeof(al_music_buffer));
        static uint8_t al_sd_buffer[512];
        spscbuffer_init(&al_sd_stream, al_sd_buffer, sizeof(al_sd_buffer));

        pc_sd_mutex = SDL_CreateMutex();
        al_music_mutex = SDL_CreateMutex();
        al_sd_mutex = SDL_CreateMutex();
        sb_mutex = SDL_CreateMutex();

        alInit(audio_format.freq);

        filter1_expma(&pc_sd_filter[0], PC_SQUARE_CUTOFF, (float)audio_format.freq);
        filter1_expma(&pc_sd_filter[1], PC_SQUARE_CUTOFF, (float)audio_format.freq);

        filter2_rbj_lp(&al_filter, ADLIB_CUTOFF, 0.7071f, (float)audio_format.freq);

        filter2_rbj_lp(&sb_filter[0], SB_RATE * 0.5f, 1.306563f, (float)audio_format.freq);
        filter2_rbj_lp(&sb_filter[1], SB_RATE * 0.5f, 0.541196f, (float)audio_format.freq);

        // start audio
        SDL_PauseAudioDevice(audio_device, 0);
    }
}

void Deinitialize(void)
{
    // clean up SDL

    SDL_DestroyTexture(frame_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    if (audio_device)
    {
        SDL_CloseAudioDevice(audio_device);

        alDeinit();

        SDL_DestroyMutex(pc_sd_mutex);
        SDL_DestroyMutex(al_music_mutex);
        SDL_DestroyMutex(al_sd_mutex);
        SDL_DestroyMutex(sb_mutex);
    }

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
// RAM
//------------------------------------------------------------------------------

void RAM_AcquireMemory(void** start, int32_t* length)
{
    int32_t size = 1024*2320;
    *start = malloc(size);
    *length = size;
}

void RAM_ReleaseMemory(void* start)
{
    free(start);
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

    if (port_width != dst_width)
    {
        SDL_RenderClear(renderer);
        SDL_Rect dest_rect =
        {
            (port_width - dst_width) / 2, 0,
            dst_width, dst_height
        };
        SDL_RenderCopy(renderer, frame_texture, NULL, &dest_rect);
    }
    else
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

    while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_KEYDOWN, SDL_KEYUP) > 0)
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

    SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
}

//------------------------------------------------------------------------------
// Mouse
//------------------------------------------------------------------------------

void Mouse_GetDelta(int16_t* dx, int16_t* dy)
{
    int mdx, mdy;
    SDL_GetRelativeMouseState(&mdx, &mdy);
    *dx = mdx;
    *dy = mdy;
}

void Mouse_ResetDelta(void)
{
    SDL_GetRelativeMouseState(0, 0);
}

uint16_t Mouse_GetButtons(void)
{
    Uint32 buttons;

    // not using SDL_GetRelativeMouseState() (don't reset delta)
    buttons = SDL_GetMouseState(0, 0);

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
    mouse_x = x;
    mouse_y = y;
}

void Mouse_GetPos(int16_t* x, int16_t* y)
{
    int dx, dy;
    SDL_GetRelativeMouseState(&dx, &dy);
    mouse_x += dx;
    mouse_y += dy;
    if (mouse_x < 0)
        mouse_x = 0;
    if (mouse_x > src_width - 1)
        mouse_x = src_width - 1;
    if (mouse_y < 0)
        mouse_y = 0;
    if (mouse_y > src_height - 1)
        mouse_y = src_height - 1;
    *x = mouse_x;
    *y = mouse_y;
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
    SDL_LockMutex(pc_sd_mutex);
    pc_sd_data = 0;
    SDL_UnlockMutex(pc_sd_mutex);
}

void PCSpeaker_PlaySound(uint8_t* data, uint32_t length)
{
    SDL_LockMutex(pc_sd_mutex);
    pc_sd_size = length;
    pc_sd_data = data;
    SDL_UnlockMutex(pc_sd_mutex);
}

void PCSpeaker_StopSound(void)
{
    SDL_LockMutex(pc_sd_mutex);
    pc_sd_data = 0;
    SDL_UnlockMutex(pc_sd_mutex);
}

uint8_t PCSpeaker_SoundPlaying(void)
{
    return pc_sd_data != 0;
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

BridgeAdLibInstrument alZeroInst;

void AdLib_SetInstrument(BridgeAdLibInstrument* inst, spscbuffer_t* stream)
{
    alcmd_t p[11];
    uint8_t m, c;

    m = 0;
    c = 3;
    alMakeCmd(p[0], m + alChar,   inst->mChar  );
    alMakeCmd(p[1], m + alScale,  inst->mScale );
    alMakeCmd(p[2], m + alAttack, inst->mAttack);
    alMakeCmd(p[3], m + alSus,    inst->mSus   );
    alMakeCmd(p[4], m + alWave,   inst->mWave  );
    alMakeCmd(p[5], c + alChar,   inst->cChar  );
    alMakeCmd(p[6], c + alScale,  inst->cScale );
    alMakeCmd(p[7], c + alAttack, inst->cAttack);
    alMakeCmd(p[8], c + alSus,    inst->cSus   );
    alMakeCmd(p[9], c + alWave,   inst->cWave  );
    // Note: Switch commenting on these lines for old MUSE compatibility
    //alMakeCmd(p[10], alFeedCon,   inst->nConn  );
    alMakeCmd(p[10], alFeedCon,   0            );

    if (spscbuffer_write(stream, p, sizeof(p)) != sizeof(p))
        SDL_assert(0);
}

uint8_t AdLib_Detect(void)
{
    int     i;
    alcmd_t p;

    for (i = 1; i <= 0xf5; i++) // Zero all the registers
    {
        alMakeCmd(p, i, 0);
        if (spscbuffer_write(&al_cmd_stream, &p, sizeof(p)) != sizeof(p))
            SDL_assert(0);
    }
    alMakeCmd(p, 1, 0x20);      // Set WSE=1
    if (spscbuffer_write(&al_cmd_stream, &p, sizeof(p)) != sizeof(p))
        SDL_assert(0);
    alMakeCmd(p, 8, 0);         // Set CSM=0 & SEL=0
    if (spscbuffer_write(&al_cmd_stream, &p, sizeof(p)) != sizeof(p))
        SDL_assert(0);

    while (spscbuffer_avail(&al_cmd_stream) != 0) { SDL_Delay(1); }

    return 1;
}

void AdLib_Clean(void)
{
    int     i;
    alcmd_t p;

    alMakeCmd(p, alEffects, 0);
    if (spscbuffer_write(&al_cmd_stream, &p, sizeof(p)) != sizeof(p))
        SDL_assert(0);
    for (i = 1; i < 0xf5; i++)
    {
        alMakeCmd(p, i, 0);
        if (spscbuffer_write(&al_cmd_stream, &p, sizeof(p)) != sizeof(p))
            SDL_assert(0);
    }

    while (spscbuffer_avail(&al_cmd_stream) != 0) { SDL_Delay(1); }
}

void AdLib_StartMusic(uint16_t* values, uint16_t length)
{
    SDL_LockMutex(al_music_mutex);
    
    al_music_data = (uint8_t*)values;
    al_music_size = length;
    al_sequencer_data = al_music_data;
    al_sequencer_size = al_music_size;
    
    SDL_UnlockMutex(al_music_mutex);
}

void AdLib_MusicOn(void)
{
    al_music_active = 1;
}

void AdLib_MusicOff(void)
{
    int     i;
    alcmd_t p;

    al_music_active = 0;
    
    SDL_LockMutex(al_music_mutex);

    alMakeCmd(p, alEffects, 0);
    if (spscbuffer_write(&al_music_stream, &p, sizeof(p)) != sizeof(p))
        SDL_assert(0);
    for (i = 0; i < sqMaxTracks; i++)
    {
        alMakeCmd(p, alFreqH + i + 1, 0);
        if (spscbuffer_write(&al_music_stream, &p, sizeof(p)) != sizeof(p))
            SDL_assert(0);
    }

    SDL_UnlockMutex(al_music_mutex);
}

void AdLib_Start(void)
{
    alcmd_t p;
    
    SDL_LockMutex(al_sd_mutex);

    alMakeCmd(p, alEffects, 0);
    if (spscbuffer_write(&al_sd_stream, &p, sizeof(p)) != sizeof(p))
        SDL_assert(0);
    AdLib_SetInstrument(&alZeroInst, &al_sd_stream);

    SDL_UnlockMutex(al_sd_mutex);
}

void AdLib_PlaySound(BridgeAdLibSound* sound)
{
    SDL_LockMutex(al_sd_mutex);

    // send instrument
    AdLib_SetInstrument(&alZeroInst, &al_sd_stream); // DEBUG
    AdLib_SetInstrument(&sound->inst, &al_sd_stream);

    al_sd_size = sound->common.length;
    al_sd_block = ((sound->block & 7) << 2) | 0x20;
    al_sd_data = sound->data;

    SDL_UnlockMutex(al_sd_mutex);
}

void AdLib_StopSound(void)
{
    alcmd_t p;

    SDL_LockMutex(al_sd_mutex);

    al_sd_data = 0;

    alMakeCmd(p, alFreqH, 0);
    if (spscbuffer_write(&al_sd_stream, &p, sizeof(p)) != sizeof(p))
        SDL_assert(0);

    SDL_UnlockMutex(al_sd_mutex);
}

void AdLib_Shut(void)
{
    alcmd_t p;

    SDL_LockMutex(al_sd_mutex);

    al_sd_data = 0;

    alMakeCmd(p, alEffects, 0);
    if (spscbuffer_write(&al_sd_stream, &p, sizeof(p)) != sizeof(p))
        SDL_assert(0);
    alMakeCmd(p, alFreqH, 0);
    if (spscbuffer_write(&al_sd_stream, &p, sizeof(p)) != sizeof(p))
        SDL_assert(0);
    AdLib_SetInstrument(&alZeroInst, &al_sd_stream);

    SDL_UnlockMutex(al_sd_mutex);
}

uint8_t AdLib_SoundPlaying(void)
{
    return al_sd_data != 0;
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
    return 1;
}

void SoundBlaster_Level(int16_t left, int16_t right)
{
    sb_level[0] = left / 15.f;
    sb_level[1] = right / 15.f;
}

void SoundBlaster_PlaySample(uint8_t* data, uint32_t length)
{
    SDL_LockMutex(sb_mutex);
    sb_size = length;
    sb_data = data;
    SDL_UnlockMutex(sb_mutex);
}

void SoundBlaster_StopSample(void)
{
    SDL_LockMutex(sb_mutex);
    sb_data = 0;
    SDL_UnlockMutex(sb_mutex);
}
