// ID_CA.C

// this has been customized for WOLF

/*
=============================================================================

Id Software Caching Manager
---------------------------

Must be started BEFORE the memory manager, because it needs to get the headers
loaded into the data segment

=============================================================================
*/

#include "ID_HEADS.H"
#pragma hdrstop

#define THREEBYTEGRSTARTS

/*
=============================================================================

                         LOCAL CONSTANTS

=============================================================================
*/

typedef struct
{
    uint16_t bit0, bit1; // 0-255 is a character, > is a pointer to a node
} huffnode;

#pragma pack(push, 2)
typedef struct
{
    uint16_t RLEWtag;
    int32_t headeroffsets[100];
    byte tileinfo[];
} mapfiletype;
#pragma pack(pop)

/*
=============================================================================

                         GLOBAL VARIABLES

=============================================================================
*/

byte* tinf;
int16_t mapon;

uint16_t* mapsegs[MAPPLANES];
maptype* mapheaderseg[NUMMAPS];
byte* audiosegs[NUMSNDCHUNKS];
void* grsegs[NUMCHUNKS];

byte grneeded[NUMCHUNKS];
byte ca_levelbit, ca_levelnum;

FileSystemHandle profilehandle, debughandle;

char audioname[13] = "AUDIO.";

/*
=============================================================================

                         LOCAL VARIABLES

=============================================================================
*/

extern  int32_t CGAhead;
extern  int32_t EGAhead;
extern  byte CGAdict;
extern  byte EGAdict;
extern  byte maphead;
extern  byte mapdict;
extern  byte audiohead;
extern  byte audiodict;


char extension[5],  // Need a string, not constant to change cache files
gheadname[10] = GREXT"HEAD.",
gfilename[10] = GREXT"GRAPH.",
gdictname[10] = GREXT"DICT.",
mheadname[10] = "MAPHEAD.",
mfilename[10] = "MAPTEMP.",
aheadname[10] = "AUDIOHED.",
afilename[10] = "AUDIOT.";

void CA_CannotOpen(char* string);

int32_t* grstarts;      // array of offsets in egagraph, -1 for sparse
int32_t* audiostarts;   // array of offsets in audio / audiot

#ifdef GRHEADERLINKED
huffnode* grhuffman;
#else
huffnode grhuffman[255];
#endif

#ifdef AUDIOHEADERLINKED
huffnode* audiohuffman;
#else
huffnode audiohuffman[255];
#endif


FileSystemHandle grhandle;       // handle to EGAGRAPH
FileSystemHandle maphandle;      // handle to MAPTEMP / GAMEMAPS
FileSystemHandle audiohandle;    // handle to AUDIOT / AUDIO

int32_t chunkcomplen, chunkexplen;

SDMode oldsoundmode;



void CAL_CarmackExpand(uint16_t* source, uint16_t* dest,
    uint16_t length);


#ifdef THREEBYTEGRSTARTS
#define FILEPOSSIZE 3
//#define GRFILEPOS(c) (*(int32_t *)(((byte *)grstarts)+(c)*3)&0xffffff)
int32_t GRFILEPOS(int16_t c)
{
    int32_t value;
    int16_t offset;

    offset = c * 3;

    value = *(int32_t*)(((byte*)grstarts) + offset);

    value &= 0x00ffffffl;

    if (value == 0xffffffl)
        value = -1;

    return value;
};
#else
#define FILEPOSSIZE 4
#define GRFILEPOS(c) (grstarts[c])
#endif

/*
=============================================================================

                       LOW LEVEL ROUTINES

=============================================================================
*/

/*
============================
=
= CA_OpenDebug / CA_CloseDebug
=
= Opens a binary file with the handle "debughandle"
=
============================
*/

void CA_OpenDebug(void)
{
    FileSystem_Remove("DEBUG.TXT");
    debughandle = FileSystem_Open("DEBUG.TXT", FileSystemCreate | FileSystemWrite | FileSystemText);
}

void CA_CloseDebug(void)
{
    FileSystem_Close(debughandle);
}



/*
============================
=
= CAL_GetGrChunkLength
=
= Gets the length of an explicit length chunk (not tiles)
= The file pointer is positioned so the compressed data can be read in next.
=
============================
*/

void CAL_GetGrChunkLength(int16_t chunk)
{
    FileSystem_Seek(grhandle, GRFILEPOS(chunk));
    FileSystem_Read(grhandle, &chunkexplen, sizeof(chunkexplen));
    chunkcomplen = GRFILEPOS(chunk + 1) - GRFILEPOS(chunk) - 4;
}


/*
==========================
=
= CA_FarRead
=
= Read from a file to a far pointer
=
==========================
*/

boolean CA_FarRead(FileSystemHandle handle, byte* dest, int32_t length)
{
    if (FileSystem_Read(handle, dest, length) != length)
        return false;

    return true;
}


/*
==========================
=
= CA_FarWrite
=
= Write from a file to a far pointer
=
==========================
*/

boolean CA_FarWrite(FileSystemHandle handle, byte* source, int32_t length)
{
    if (FileSystem_Write(handle, source, length) != length)
        return false;

    return true;
}


/*
==========================
=
= CA_ReadFile
=
= Reads a file into an allready allocated buffer
=
==========================
*/

boolean CA_ReadFile(char* filename, memptr* ptr)
{
    FileSystemHandle handle;
    int32_t size;

    if (FileSystem_ValidHandle(handle = FileSystem_Open(filename, FileSystemRead | FileSystemBinary)) == false)
        return false;

    size = FileSystem_FileLength(handle);
    if (!CA_FarRead(handle, *ptr, size))
    {
        FileSystem_Close(handle);
        return false;
    }
    FileSystem_Close(handle);
    return true;
}


/*
==========================
=
= CA_WriteFile
=
= Writes a file from a memory buffer
=
==========================
*/

boolean CA_WriteFile(char* filename, void* ptr, int32_t length)
{
    FileSystemHandle handle;

    handle = FileSystem_Open(filename, FileSystemCreate | FileSystemBinary | FileSystemWrite);

    if (FileSystem_ValidHandle(handle) == false)
        return false;

    if (!CA_FarWrite(handle, ptr, length))
    {
        FileSystem_Close(handle);
        return false;
    }
    FileSystem_Close(handle);
    return true;
}



/*
==========================
=
= CA_LoadFile
=
= Allocate space for and load a file
=
==========================
*/

boolean CA_LoadFile(char* filename, memptr* ptr)
{
    FileSystemHandle handle;
    int32_t size;

    if (FileSystem_ValidHandle(handle = FileSystem_Open(filename, FileSystemRead | FileSystemBinary)) == false)
        return false;

    size = FileSystem_FileLength(handle);
    MM_GetPtr(ptr, size);
    if (!CA_FarRead(handle, *ptr, size))
    {
        FileSystem_Close(handle);
        return false;
    }
    FileSystem_Close(handle);
    return true;
}

/*
============================================================================

        COMPRESSION routines, see JHUFF.C for more

============================================================================
*/



/*
======================
=
= CAL_HuffExpand
=
= Length is the length of the EXPANDED data
= If screenhack, the data is decompressed in four planes directly
= to the screen
=
======================
*/

void CAL_HuffExpand(byte* source, byte* dest,
    int32_t length, huffnode* hufftable, boolean screenhack)
{
    uint8_t bit, byte, *end;
    uint16_t code;
    huffnode *nodeon, *headptr;
    uint16_t vgaplane;

    headptr = hufftable + 254;  // head node is always node 254

    if (screenhack)
    {
        vgaplane = 0;
        length >>= 2;
    }

    end = dest + length;

    nodeon = headptr;

    byte = *source++;           // load first byte
    bit = 1;

    for (;;)
    {
        if (byte & bit)             // bit set?
            code = nodeon->bit1;    // take bit1 path
        else
            code = nodeon->bit0;    // take bit0 path from node

        bit <<= 1;                  // advance to next bit position

        if (!bit)
        {
            byte = *source++;       // load next byte
            bit = 1;                // back to first bit
        }

        if (code < 256)             // if code<256 its a byte, else move node
        {
            *dest++ = (uint8_t)code;    // write a decompressed byte out
            nodeon = headptr;           // back to the head node for next bit
            if (dest == end)            // done?
            {
                if (screenhack)
                {
                    dest += VGA_PLANE_SIZE - length;    // next vga plane
                    end = dest + length;
                    if (++vgaplane == 4)
                        break;
                }
                else
                    break;
            }
        }
        else
            nodeon = hufftable + (code - 256);
    }
}


/*
======================
=
= CAL_CarmackExpand
=
= Length is the length of the EXPANDED data
=
======================
*/

#define NEARTAG 0xa7
#define FARTAG 0xa8

void CAL_CarmackExpand(uint16_t* source, uint16_t* dest, uint16_t length)
{
    uint16_t ch, chhigh, count, offset;
    uint16_t *copyptr, *inptr, *outptr;

    length /= 2;

    inptr = source;
    outptr = dest;

    while (length)
    {
        ch = *inptr++;
        chhigh = ch >> 8;
        if (chhigh == NEARTAG)
        {
            count = ch & 0xff;
            if (!count)
            {   // have to insert a word containing the tag byte
                ch |= *(uint8_t*)inptr;
                inptr = (uint16_t*)((intptr_t)inptr + 1);
                *outptr++ = ch;
                length--;
            }
            else
            {
                offset = *(uint8_t*)inptr;
                inptr = (uint16_t*)((intptr_t)inptr + 1);
                copyptr = outptr - offset;
                length -= count;
                while (count--)
                    *outptr++ = *copyptr++;
            }
        }
        else if (chhigh == FARTAG)
        {
            count = ch & 0xff;
            if (!count)
            {   // have to insert a word containing the tag byte
                ch |= *(uint8_t*)inptr;
                inptr = (uint16_t*)((intptr_t)inptr + 1);
                *outptr++ = ch;
                length--;
            }
            else
            {
                offset = *inptr++;
                copyptr = dest + offset;
                length -= count;
                while (count--)
                    *outptr++ = *copyptr++;
            }
        }
        else
        {
            *outptr++ = ch;
            length--;
        }
    }
}



/*
======================
=
= CA_RLEWcompress
=
======================
*/

int32_t CA_RLEWCompress(uint16_t* source, int32_t length, uint16_t* dest,
    uint16_t rlewtag)
{
    int32_t complength;
    uint16_t value, count, i;
    uint16_t* start, * end;

    start = dest;

    end = source + (length + 1) / 2;

    //
    // compress it
    //
    do
    {
        count = 1;
        value = *source++;
        while (*source == value && source < end)
        {
            count++;
            source++;
        }
        if (count > 3 || value == rlewtag)
        {
            //
            // send a tag / count / value string
            //
            *dest++ = rlewtag;
            *dest++ = count;
            *dest++ = value;
        }
        else
        {
            //
            // send word without compressing
            //
            for (i = 1; i <= count; i++)
                *dest++ = value;
        }

    } while (source < end);

    complength = 2 * (dest - start);
    return complength;
}


/*
======================
=
= CA_RLEWexpand
= length is EXPANDED length
=
======================
*/

void CA_RLEWexpand(uint16_t* source, uint16_t* dest, int32_t length,
    uint16_t rlewtag)
{
    uint16_t value,count,i;
    uint16_t* end;

    end = dest + (length) / 2;

    //
    // expand it
    //
    do
    {
        value = *source++;
        if (value != rlewtag)
            //
            // uncompressed
            //
            *dest++ = value;
        else
        {
            //
            // compressed string
            //
            count = *source++;
            value = *source++;
            for (i = 1; i <= count; i++)
                *dest++ = value;
        }
    } while (dest < end);
}



/*
=============================================================================

                     CACHE MANAGER ROUTINES

=============================================================================
*/


/*
======================
=
= CAL_SetupGrFile
=
======================
*/

void CAL_SetupGrFile(void)
{
    char fname[13];
    FileSystemHandle handle;
    memptr compseg;

#ifdef GRHEADERLINKED

    grhuffman = (huffnode*)&EGAdict;
    grstarts = (int32_t _seg*)FP_SEG(&EGAhead);

#else

    //
    // load ???dict.ext (huffman dictionary for graphics files)
    //

    strcpy(fname, gdictname);
    strcat(fname, extension);

    if (FileSystem_ValidHandle(handle = FileSystem_Open(fname,
        FileSystemRead | FileSystemBinary)) == false)
        CA_CannotOpen(fname);

    FileSystem_Read(handle, &grhuffman, sizeof(grhuffman));
    FileSystem_Close(handle);
    //
    // load the data offsets from ???head.ext
    //
    MM_GetPtr(&grstarts, (NUMCHUNKS + 1) * FILEPOSSIZE);

    strcpy(fname, gheadname);
    strcat(fname, extension);

    if (FileSystem_ValidHandle(handle = FileSystem_Open(fname,
        FileSystemRead | FileSystemBinary)) == false)
        CA_CannotOpen(fname);

    CA_FarRead(handle, (memptr)grstarts, (NUMCHUNKS + 1) * FILEPOSSIZE);

    FileSystem_Close(handle);


#endif

    //
    // Open the graphics file, leaving it open until the game is finished
    //
    strcpy(fname, gfilename);
    strcat(fname, extension);

    grhandle = FileSystem_Open(fname, FileSystemRead | FileSystemBinary);
    if (FileSystem_ValidHandle(grhandle) == false)
        CA_CannotOpen(fname);


    //
    // load the pic and sprite headers into the arrays in the data segment
    //
    MM_GetPtr(&pictable, NUMPICS * sizeof(pictabletype));
    CAL_GetGrChunkLength(STRUCTPIC);  // position file pointer
    MM_GetPtr(&compseg, chunkcomplen);
    CA_FarRead(grhandle, compseg, chunkcomplen);
    CAL_HuffExpand(compseg, (byte*)pictable, NUMPICS * sizeof(pictabletype), grhuffman, false);
    MM_FreePtr(&compseg);
}

//==========================================================================


/*
======================
=
= CAL_SetupMapFile
=
======================
*/

void CAL_SetupMapFile(void)
{
    int16_t i;
    FileSystemHandle handle;
    int32_t length, pos;
    char fname[13];

    //
    // load maphead.ext (offsets and tileinfo for map file)
    //
#ifndef MAPHEADERLINKED
    strcpy(fname, mheadname);
    strcat(fname, extension);

    if (FileSystem_ValidHandle(handle = FileSystem_Open(fname,
        FileSystemRead | FileSystemBinary)) == false)
        CA_CannotOpen(fname);

    length = FileSystem_FileLength(handle);
    MM_GetPtr(&tinf, length);
    CA_FarRead(handle, tinf, length);
    FileSystem_Close(handle);
#else

    tinf = (byte _seg*)FP_SEG(&maphead);

#endif

    //
    // open the data file
    //
#ifdef CARMACIZED
    strcpy(fname, "GAMEMAPS.");
    strcat(fname, extension);

    if (FileSystem_ValidHandle(maphandle = FileSystem_Open(fname,
        FileSystemRead | FileSystemBinary)) == false)
        CA_CannotOpen(fname);
#else
    strcpy(fname, mfilename);
    strcat(fname, extension);

    if ((maphandle = open(fname,
        O_RDONLY | O_BINARY, S_IREAD)) == -1)
        CA_CannotOpen(fname);
#endif

    //
    // load all map header
    //
    for (i = 0; i < NUMMAPS; i++)
    {
        pos = ((mapfiletype*)tinf)->headeroffsets[i];
        if (pos < 0)      // $FFFFFFFF start is a sparse map
            continue;

        MM_GetPtr(&mapheaderseg[i], sizeof(maptype));
        MM_SetLock(&mapheaderseg[i], true);
        FileSystem_Seek(maphandle, pos);
        CA_FarRead(maphandle, (memptr)mapheaderseg[i], sizeof(maptype));
    }

    //
    // allocate space for 3 64*64 planes
    //
    for (i = 0; i < MAPPLANES; i++)
    {
        MM_GetPtr(&mapsegs[i], 64 * 64 * 2);
        MM_SetLock(&mapsegs[i], true);
    }
}


//==========================================================================


/*
======================
=
= CAL_SetupAudioFile
=
======================
*/

void CAL_SetupAudioFile(void)
{
    FileSystemHandle handle;
    int32_t length;
    char fname[13];

    //
    // load maphead.ext (offsets and tileinfo for map file)
    //
#ifndef AUDIOHEADERLINKED
    strcpy(fname, aheadname);
    strcat(fname, extension);

    if (FileSystem_ValidHandle(handle = FileSystem_Open(fname,
        FileSystemRead | FileSystemBinary)) == false)
        CA_CannotOpen(fname);

    length = FileSystem_FileLength(handle);
    MM_GetPtr(&audiostarts, length);
    CA_FarRead(handle, (byte*)audiostarts, length);
    FileSystem_Close(handle);
#else
    audiohuffman = (huffnode*)&audiodict;
    audiostarts = (int32_t*)FP_SEG(&audiohead);
#endif

    //
    // open the data file
    //
#ifndef AUDIOHEADERLINKED
    strcpy(fname, afilename);
    strcat(fname, extension);

    if (FileSystem_ValidHandle(audiohandle = FileSystem_Open(fname,
        FileSystemRead | FileSystemBinary)) == false)
        CA_CannotOpen(fname);
#else
    if ((audiohandle = open("AUDIO."EXTENSION,
        O_RDONLY | O_BINARY, S_IREAD)) == -1)
        Quit("Can't open AUDIO."EXTENSION"!");
#endif
}

//==========================================================================


/*
======================
=
= CA_Startup
=
= Open all files and load in headers
=
======================
*/

void CA_Startup(void)
{
#ifdef PROFILE
    FileSystem_Remove("PROFILE.TXT");
    profilehandle = open("PROFILE.TXT", O_CREAT | O_WRONLY | O_TEXT);
#endif

    CAL_SetupMapFile();
    CAL_SetupGrFile();
    CAL_SetupAudioFile();

    mapon = -1;
    ca_levelbit = 1;
    ca_levelnum = 0;

}

//==========================================================================


/*
======================
=
= CA_Shutdown
=
= Closes all files
=
======================
*/

void CA_Shutdown(void)
{
#ifdef PROFILE
    close(profilehandle);
#endif

    if (FileSystem_ValidHandle(maphandle))
        FileSystem_Close(maphandle);
    if (FileSystem_ValidHandle(grhandle))
        FileSystem_Close(grhandle);
    if (FileSystem_ValidHandle(audiohandle))
        FileSystem_Close(audiohandle);
}

//===========================================================================

/*
======================
=
= CA_CacheAudioChunk
=
======================
*/

void CA_CacheAudioChunk(int16_t chunk)
{
    int32_t pos, compressed;
#ifdef AUDIOHEADERLINKED
    int32_t expanded;
    memptr bigbufferseg;
    byte far* source;
#endif

    if (audiosegs[chunk])
    {
        MM_SetPurge(&audiosegs[chunk], 0);
        return;       // allready in memory
    }

    //
    // load the chunk into a buffer, either the miscbuffer if it fits, or allocate
    // a larger buffer
    //
    pos = audiostarts[chunk];
    compressed = audiostarts[chunk + 1] - pos;

    FileSystem_Seek(audiohandle, pos);

#ifndef AUDIOHEADERLINKED

    MM_GetPtr(&audiosegs[chunk], compressed);
    if (mmerror)
        return;

    CA_FarRead(audiohandle, audiosegs[chunk], compressed);

#else

    if (compressed <= BUFFERSIZE)
    {
        CA_FarRead(audiohandle, bufferseg, compressed);
        source = bufferseg;
    }
    else
    {
        MM_GetPtr(&bigbufferseg, compressed);
        if (mmerror)
            return;
        MM_SetLock(&bigbufferseg, true);
        CA_FarRead(audiohandle, bigbufferseg, compressed);
        source = bigbufferseg;
    }

    expanded = *(int32_t far*)source;
    source += 4;   // skip over length
    MM_GetPtr(&(memptr)audiosegs[chunk], expanded);
    if (mmerror)
        goto done;
    CAL_HuffExpand(source, audiosegs[chunk], expanded, audiohuffman, false);

done:
    if (compressed > BUFFERSIZE)
        MM_FreePtr(&bigbufferseg);
#endif
}

//===========================================================================

/*
======================
=
= CA_LoadAllSounds
=
= Purges all sounds, then loads all new ones (mode switch)
=
======================
*/

void CA_LoadAllSounds(void)
{
    uint16_t start, i;

    switch (oldsoundmode)
    {
    case sdm_Off:
        goto cachein;
    case sdm_PC:
        start = STARTPCSOUNDS;
        break;
    case sdm_AdLib:
        start = STARTADLIBSOUNDS;
        break;
    }

    for (i = 0; i < NUMSOUNDS; i++, start++)
        if (audiosegs[start])
            MM_SetPurge(&audiosegs[start], 3);  // make purgable

cachein:

    switch (SoundMode)
    {
    case sdm_Off:
        return;
    case sdm_PC:
        start = STARTPCSOUNDS;
        break;
    case sdm_AdLib:
        start = STARTADLIBSOUNDS;
        break;
    }

    for (i = 0; i < NUMSOUNDS; i++, start++)
        CA_CacheAudioChunk(start);

    oldsoundmode = SoundMode;
}

//===========================================================================


/*
======================
=
= CAL_ExpandGrChunk
=
= Does whatever is needed with a pointer to a compressed chunk
=
======================
*/

void CAL_ExpandGrChunk(int16_t chunk, byte* source)
{
    int32_t expanded;


    if (chunk >= STARTTILE8 && chunk < STARTEXTERNS)
    {
        //
        // expanded sizes of tile8/16/32 are implicit
        //

#define BLOCK  64
#define MASKBLOCK 128

        if (chunk < STARTTILE8M)   // tile 8s are all in one chunk!
            expanded = BLOCK * NUMTILE8;
        else if (chunk < STARTTILE16)
            expanded = MASKBLOCK * NUMTILE8M;
        else if (chunk < STARTTILE16M) // all other tiles are one/chunk
            expanded = BLOCK * 4;
        else if (chunk < STARTTILE32)
            expanded = MASKBLOCK * 4;
        else if (chunk < STARTTILE32M)
            expanded = BLOCK * 16;
        else
            expanded = MASKBLOCK * 16;
    }
    else
    {
        //
        // everything else has an explicit size longword
        //
        expanded = *(int32_t*)source;
        source += 4;   // skip over length
    }

    //
    // allocate final space, decompress it, and free bigbuffer
    // Sprites need to have shifts made and various other junk
    //
    MM_GetPtr(&grsegs[chunk], expanded);
    if (mmerror)
        return;
    CAL_HuffExpand(source, grsegs[chunk], expanded, grhuffman, false);
}


/*
======================
=
= CA_CacheGrChunk
=
= Makes sure a given chunk is in memory, loadiing it if needed
=
======================
*/

void CA_CacheGrChunk(int16_t chunk)
{
    int32_t pos, compressed;
    memptr bigbufferseg;
    byte* source;
    int16_t  next;

    grneeded[chunk] |= ca_levelbit;  // make sure it doesn't get removed
    if (grsegs[chunk])
    {
        MM_SetPurge(&grsegs[chunk], 0);
        return;       // allready in memory
    }

    //
    // load the chunk into a buffer, either the miscbuffer if it fits, or allocate
    // a larger buffer
    //
    pos = GRFILEPOS(chunk);
    if (pos < 0)       // $FFFFFFFF start is a sparse tile
        return;

    next = chunk + 1;
    while (GRFILEPOS(next) == -1)  // skip past any sparse tiles
        next++;

    compressed = GRFILEPOS(next) - pos;

    FileSystem_Seek(grhandle, pos);

    if (compressed <= BUFFERSIZE)
    {
        CA_FarRead(grhandle, bufferseg, compressed);
        source = bufferseg;
    }
    else
    {
        MM_GetPtr(&bigbufferseg, compressed);
        MM_SetLock(&bigbufferseg, true);
        CA_FarRead(grhandle, bigbufferseg, compressed);
        source = bigbufferseg;
    }

    CAL_ExpandGrChunk(chunk, source);

    if (compressed > BUFFERSIZE)
        MM_FreePtr(&bigbufferseg);
}



//==========================================================================

/*
======================
=
= CA_CacheScreen
=
= Decompresses a chunk from disk straight onto the screen
=
======================
*/

void CA_CacheScreen(int16_t chunk)
{
    int32_t pos, compressed, expanded;
    memptr bigbufferseg;
    byte* source;
    int16_t  next;

    //
    // load the chunk into a buffer
    //
    pos = GRFILEPOS(chunk);
    next = chunk + 1;
    while (GRFILEPOS(next) == -1)  // skip past any sparse tiles
        next++;
    compressed = GRFILEPOS(next) - pos;

    FileSystem_Seek(grhandle, pos);

    MM_GetPtr(&bigbufferseg, compressed);
    MM_SetLock(&bigbufferseg, true);
    CA_FarRead(grhandle, bigbufferseg, compressed);
    source = bigbufferseg;

    expanded = *(int32_t*)source;
    source += 4;   // skip over length

//
// allocate final space, decompress it, and free bigbuffer
// Sprites need to have shifts made and various other junk
//
    CAL_HuffExpand(source, &screenseg[bufferofs], expanded, grhuffman, true);
    VW_MarkUpdateBlock(0, 0, 319, 199);
    MM_FreePtr(&bigbufferseg);
}

//==========================================================================

/*
======================
=
= CA_CacheMap
=
= WOLF: This is specialized for a 64*64 map size
=
======================
*/

void CA_CacheMap(int16_t mapnum)
{
    int32_t pos, compressed;
    int16_t  plane;
    memptr* dest, bigbufferseg;
    uint16_t size;
    uint16_t* source;
#ifdef CARMACIZED
    memptr buffer2seg;
    int32_t expanded;
#endif

    mapon = mapnum;

    //
    // load the planes into the allready allocated buffers
    //
    size = 64 * 64 * 2;

    for (plane = 0; plane < MAPPLANES; plane++)
    {
        pos = mapheaderseg[mapnum]->planestart[plane];
        compressed = mapheaderseg[mapnum]->planelength[plane];

        dest = &mapsegs[plane];

        FileSystem_Seek(maphandle, pos);
        if (compressed <= BUFFERSIZE)
            source = bufferseg;
        else
        {
            MM_GetPtr(&bigbufferseg, compressed);
            MM_SetLock(&bigbufferseg, true);
            source = bigbufferseg;
        }

        CA_FarRead(maphandle, (byte*)source, compressed);
#ifdef CARMACIZED
        //
        // unhuffman, then unRLEW
        // The huffman'd chunk has a two byte expanded length first
        // The resulting RLEW chunk also does, even though it's not really
        // needed
        //
        expanded = *source;
        source++;
        MM_GetPtr(&buffer2seg, expanded);
        CAL_CarmackExpand(source, (uint16_t*)buffer2seg, expanded);
        CA_RLEWexpand(((uint16_t*)buffer2seg) + 1, *dest, size,
            ((mapfiletype*)tinf)->RLEWtag);
        MM_FreePtr(&buffer2seg);

#else
        //
        // unRLEW, skipping expanded length
        //
        CA_RLEWexpand(source + 1, *dest, size,
            ((mapfiletype*)tinf)->RLEWtag);
#endif

        if (compressed > BUFFERSIZE)
            MM_FreePtr(&bigbufferseg);
    }
}

//===========================================================================

/*
======================
=
= CA_UpLevel
=
= Goes up a bit level in the needed lists and clears it out.
= Everything is made purgable
=
======================
*/

void CA_UpLevel(void)
{
    int16_t i;

    if (ca_levelnum == 7)
        Quit("CA_UpLevel: Up past level 7!");

    for (i = 0; i < NUMCHUNKS; i++)
        if (grsegs[i])
            MM_SetPurge(&grsegs[i], 3);
    ca_levelbit <<= 1;
    ca_levelnum++;
}

//===========================================================================

/*
======================
=
= CA_DownLevel
=
= Goes down a bit level in the needed lists and recaches
= everything from the lower level
=
======================
*/

void CA_DownLevel(void)
{
    if (!ca_levelnum)
        Quit("CA_DownLevel: Down past level 0!");
    ca_levelbit >>= 1;
    ca_levelnum--;
    CA_CacheMarks();
}

//===========================================================================

/*
======================
=
= CA_ClearMarks
=
= Clears out all the marks at the current level
=
======================
*/

void CA_ClearMarks(void)
{
    int16_t i;

    for (i = 0; i < NUMCHUNKS; i++)
        grneeded[i] &= ~ca_levelbit;
}


//===========================================================================

/*
======================
=
= CA_ClearAllMarks
=
= Clears out all the marks on all the levels
=
======================
*/

void CA_ClearAllMarks(void)
{
    memset(grneeded, 0, sizeof(grneeded));
    ca_levelbit = 1;
    ca_levelnum = 0;
}


//===========================================================================


/*
======================
=
= CA_FreeGraphics
=
======================
*/


void CA_SetGrPurge(void)
{
    int16_t i;

    //
    // free graphics
    //
    CA_ClearMarks();

    for (i = 0; i < NUMCHUNKS; i++)
        if (grsegs[i])
            MM_SetPurge(&grsegs[i], 3);
}



/*
======================
=
= CA_SetAllPurge
=
= Make everything possible purgable
=
======================
*/

void CA_SetAllPurge(void)
{
    int16_t i;


    //
    // free sounds
    //
    for (i = 0; i < NUMSNDCHUNKS; i++)
        if (audiosegs[i])
            MM_SetPurge(&audiosegs[i], 3);

    //
    // free graphics
    //
    CA_SetGrPurge();
}


//===========================================================================

/*
======================
=
= CA_CacheMarks
=
======================
*/
#define MAXEMPTYREAD 1024

void CA_CacheMarks(void)
{
    int16_t  i, next, numcache;
    int32_t pos, endpos, nextpos, nextendpos, compressed;
    int32_t bufferstart, bufferend; // file position of general buffer
    byte* source;
    memptr bigbufferseg;

    numcache = 0;
    //
    // go through and make everything not needed purgable
    //
    for (i = 0; i < NUMCHUNKS; i++)
        if (grneeded[i] & ca_levelbit)
        {
            if (grsegs[i])     // its allready in memory, make
                MM_SetPurge(&grsegs[i], 0); // sure it stays there!
            else
                numcache++;
        }
        else
        {
            if (grsegs[i])     // not needed, so make it purgeable
                MM_SetPurge(&grsegs[i], 3);
        }

    if (!numcache)   // nothing to cache!
        return;


    //
    // go through and load in anything still needed
    //
    bufferstart = bufferend = 0;  // nothing good in buffer now

    for (i = 0; i < NUMCHUNKS; i++)
        if ((grneeded[i] & ca_levelbit) && !grsegs[i])
        {
            pos = GRFILEPOS(i);
            if (pos < 0)
                continue;

            next = i + 1;
            while (GRFILEPOS(next) == -1)  // skip past any sparse tiles
                next++;

            compressed = GRFILEPOS(next) - pos;
            endpos = pos + compressed;

            if (compressed <= BUFFERSIZE)
            {
                if (bufferstart <= pos
                    && bufferend >= endpos)
                {
                    // data is allready in buffer
                    source = (byte*)bufferseg + (pos - bufferstart);
                }
                else
                {
                    // load buffer with a new block from disk
                    // try to get as many of the needed blocks in as possible
                    while (next < NUMCHUNKS)
                    {
                        while (next < NUMCHUNKS &&
                            !(grneeded[next] & ca_levelbit && !grsegs[next]))
                            next++;
                        if (next == NUMCHUNKS)
                            continue;

                        nextpos = GRFILEPOS(next);
                        while (GRFILEPOS(++next) == -1) // skip past any sparse tiles
                            ;
                        nextendpos = GRFILEPOS(next);
                        if (nextpos - endpos <= MAXEMPTYREAD
                            && nextendpos - pos <= BUFFERSIZE)
                            endpos = nextendpos;
                        else
                            next = NUMCHUNKS;   // read pos to posend
                    }

                    FileSystem_Seek(grhandle, pos);
                    CA_FarRead(grhandle, bufferseg, endpos - pos);
                    bufferstart = pos;
                    bufferend = endpos;
                    source = bufferseg;
                }
            }
            else
            {
                // big chunk, allocate temporary buffer
                MM_GetPtr(&bigbufferseg, compressed);
                if (mmerror)
                    return;
                MM_SetLock(&bigbufferseg, true);
                FileSystem_Seek(grhandle, pos);
                CA_FarRead(grhandle, bigbufferseg, compressed);
                source = bigbufferseg;
            }

            CAL_ExpandGrChunk(i, source);
            if (mmerror)
                return;

            if (compressed > BUFFERSIZE)
                MM_FreePtr(&bigbufferseg);

        }
}

void CA_CannotOpen(char* string)
{
    char str[30];

    strcpy(str, "Can't open ");
    strcat(str, string);
    strcat(str, "!\n");
    Quit(str);
}
