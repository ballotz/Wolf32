// ID_VL.C

//#include <dos.h>
//#include <alloc.h>
//#include <mem.h>
#include <string.h>
#include "ID_HEAD.H"
#include "ID_VL.H"
#include "bridge.h"
#pragma hdrstop

//===========================================================================

uint8_t vgadata[VGA_PLANES][VGA_PLANE_SIZE];
uint8_t vgapal[256][3];
uint16_t vgawidth, vgaheight, vgastart;

//===========================================================================

uint16_t bufferofs;
uint16_t displayofs, pelpan;

uint8_t* screenseg = &vgadata[0][0];

uint16_t linewidth;
uint16_t ylookup[MAXSCANLINES];

boolean  screenfaded;
//uint16_t bordercolor;

byte    palette1[256][3], palette2[256][3];

//===========================================================================

/*
;==============
;
; VL_WaitVBL   ******** NEW *********
;
; Wait for the vertical retrace (returns before the actual vertical sync)
;
;==============
*/
void VL_WaitVBL(uint16_t vbls)
{
    int32_t stoptime = TimeCount_Get() + vbls;
    while (TimeCount_Get() < stoptime)
        ;
}

/*
;==============
;
; VL_SetScreen
;
;==============
*/
void VL_SetScreen(uint16_t crtc, uint16_t pelpan)
{
    vgastart = crtc;
    VL_Refresh();
}

void VL_Refresh()
{
    VGA_Update((uint8_t*)vgadata + vgastart, vgawidth, vgaheight, &vgapal[0][0]);
}

//===========================================================================

void VGA_Write(uint16_t mask, uint16_t addr, uint16_t color)
{
    if (mask & 1)
        vgadata[0][addr] = (uint8_t)color;
    if (mask & 2)
        vgadata[1][addr] = (uint8_t)color;
    if (mask & 4)
        vgadata[2][addr] = (uint8_t)color;
    if (mask & 8)
        vgadata[3][addr] = (uint8_t)color;
}

//===========================================================================

/*
=======================
=
= VL_Startup
=
=======================
*/

void VL_Startup(void)
{
}



/*
=======================
=
= VL_Shutdown
=
=======================
*/

void VL_Shutdown(void)
{
}


/*
=======================
=
= VL_SetVGAPlaneMode
=
=======================
*/

void VL_SetVGAPlaneMode(void)
{
    vgawidth = 320;
    vgaheight = 200;
    VL_ClearVideo(0);
    VL_SetLineWidth(40);
}

//===========================================================================

/*
=================
=
= VL_ClearVideo
=
= Fill the entire video buffer with a given color
=
=================
*/

void VL_ClearVideo(byte color)
{
    memset(vgadata, color, sizeof(vgadata));
    VL_Refresh();
}


//===========================================================================

/*
====================
=
= VL_SetLineWidth
=
= Line witdh is in WORDS, 40 words is normal width for vgaplanegr
=
====================
*/

void VL_SetLineWidth(uint16_t width)
{
    int16_t i, offset;

    //
    // set up lookup tables
    //
    linewidth = width * 2;

    offset = 0;

    for (i = 0; i < MAXSCANLINES; i++)
    {
        ylookup[i] = offset;
        offset += linewidth;
    }
}
/*
=============================================================================

      PALETTE OPS

  To avoid snow, do a WaitVBL BEFORE calling these

=============================================================================
*/


/*
=================
=
= VL_FillPalette
=
=================
*/

void VL_FillPalette(int16_t red, int16_t green, int16_t blue)
{
    int16_t i;

    for (i = 0; i < 256; i++)
    {
        vgapal[i][0] = (uint8_t)red;
        vgapal[i][1] = (uint8_t)green;
        vgapal[i][2] = (uint8_t)blue;
    }

    VL_Refresh();
}

//===========================================================================

/*
=================
=
= VL_SetColor
=
=================
*/

void VL_SetColor(int16_t color, int16_t red, int16_t green, int16_t blue)
{
    vgapal[color][0] = (uint8_t)red;
    vgapal[color][1] = (uint8_t)green;
    vgapal[color][2] = (uint8_t)blue;
}

//===========================================================================

/*
=================
=
= VL_GetColor
=
=================
*/

void VL_GetColor(int16_t color, int16_t* red, int16_t* green, int16_t* blue)
{
    *red = vgapal[color][0];
    *green = vgapal[color][1];
    *blue = vgapal[color][2];
}

//===========================================================================

/*
=================
=
= VL_SetPalette
=
=================
*/

void VL_SetPalette(byte* palette)
{
    memcpy(vgapal, palette, sizeof(vgapal));
    VL_Refresh();
}


//===========================================================================

/*
=================
=
= VL_GetPalette
=
=================
*/

void VL_GetPalette(byte* palette)
{
    memcpy(palette, vgapal, sizeof(vgapal));
}


//===========================================================================

/*
=================
=
= VL_FadeOut
=
= Fades the current palette to the given color in the given number of steps
=
=================
*/

void VL_FadeOut(int16_t start, int16_t end, int16_t red, int16_t green, int16_t blue, int16_t steps)
{
    int16_t  i, j, orig, delta;
    byte* origptr, * newptr;

    VL_WaitVBL(1);
    VL_GetPalette(&palette1[0][0]);
    memcpy(palette2, palette1, 768);

    //
    // fade through intermediate frames
    //
    for (i = 0; i < steps; i++)
    {
        origptr = &palette1[start][0];
        newptr = &palette2[start][0];
        for (j = start; j <= end; j++)
        {
            orig = *origptr++;
            delta = red - orig;
            *newptr++ = orig + delta * i / steps;
            orig = *origptr++;
            delta = green - orig;
            *newptr++ = orig + delta * i / steps;
            orig = *origptr++;
            delta = blue - orig;
            *newptr++ = orig + delta * i / steps;
        }

        VL_WaitVBL(1);
        VL_SetPalette(&palette2[0][0]);
    }

    //
    // final color
    //
    VL_FillPalette(red, green, blue);

    screenfaded = true;
}


/*
=================
=
= VL_FadeIn
=
=================
*/

void VL_FadeIn(int16_t start, int16_t end, byte* palette, int16_t steps)
{
    int16_t  i, j, delta;

    VL_WaitVBL(1);
    VL_GetPalette(&palette1[0][0]);
    memcpy(&palette2[0][0], &palette1[0][0], sizeof(palette1));

    start *= 3;
    end = end * 3 + 2;

    //
    // fade through intermediate frames
    //
    for (i = 0; i < steps; i++)
    {
        for (j = start; j <= end; j++)
        {
            delta = palette[j] - palette1[0][j];
            palette2[0][j] = palette1[0][j] + delta * i / steps;
        }

        VL_WaitVBL(1);
        VL_SetPalette(&palette2[0][0]);
    }

    //
    // final color
    //
    VL_SetPalette(palette);
    screenfaded = false;
}



///*
//==================
//=
//= VL_ColorBorder
//=
//==================
//*/
//
//void VL_ColorBorder(int16_t color)
//{
//    bordercolor = color;
//}



/*
=============================================================================

       PIXEL OPS

=============================================================================
*/

byte pixmasks[4] = { 1,2,4,8 };
byte leftmasks[4] = { 15,14,12,8 };
byte rightmasks[4] = { 1,3,7,15 };


/*
=================
=
= VL_Plot
=
=================
*/

void VL_Plot(int16_t x, int16_t y, int16_t color)
{
    VGA_Write(pixmasks[x & 3], bufferofs + (ylookup[y] + (x >> 2)), color);
}


/*
=================
=
= VL_Hlin
=
=================
*/

void VL_Hlin(uint16_t x, uint16_t y, uint16_t width, uint16_t color)
{
    uint16_t xbyte;
    uint16_t dest;
    byte leftmask, rightmask;
    int16_t midbytes;
    int16_t i;

    xbyte = x >> 2;
    leftmask = leftmasks[x & 3];
    rightmask = rightmasks[(x + width - 1) & 3];
    midbytes = ((x + width + 3) >> 2) - xbyte - 2;

    dest = bufferofs + ylookup[y] + xbyte;

    if (midbytes < 0)
    {
        // all in one byte
        VGA_Write(leftmask & rightmask, dest, color);
        return;
    }

    VGA_Write(leftmask, dest++, color);

    for (i = 0; i < midbytes; ++i)
        VGA_Write(15, dest++, color);

    VGA_Write(rightmask, dest, color);
}


/*
=================
=
= VL_Vlin
=
=================
*/

void VL_Vlin(int16_t x, int16_t y, int16_t height, int16_t color)
{
    int16_t dest;
    byte mask;

    mask = pixmasks[x & 3];

    dest = bufferofs + ylookup[y] + (x >> 2);

    while (height--)
    {
        VGA_Write(mask, dest, color);
        dest += linewidth;
    }
}


/*
=================
=
= VL_Bar
=
=================
*/

void VL_Bar(int16_t x, int16_t y, int16_t width, int16_t height, int16_t color)
{
    int16_t dest;
    byte leftmask, rightmask;
    int16_t midbytes, linedelta;
    int16_t i;

    leftmask = leftmasks[x & 3];
    rightmask = rightmasks[(x + width - 1) & 3];
    midbytes = ((x + width + 3) >> 2) - (x >> 2) - 2;
    linedelta = linewidth - (midbytes + 1);

    dest = bufferofs + ylookup[y] + (x >> 2);

    if (midbytes < 0)
    {
        // all in one byte
        while (height--)
        {
            VGA_Write(leftmask & rightmask, dest, color);
            dest += linewidth;
        }
        return;
    }

    while (height--)
    {
        VGA_Write(leftmask, dest++, color);

        for (i = 0; i < midbytes; ++i)
            VGA_Write(15, dest++, color);

        VGA_Write(rightmask, dest, color);

        dest += linedelta;
    }
}

/*
============================================================================

       MEMORY OPS

============================================================================
*/

/*
=================
=
= VL_MemToLatch
=
=================
*/

void VL_MemToLatch(byte* source, int16_t width, int16_t height, uint16_t dest)
{
    uint16_t count, plane;

    count = ((width + 3) / 4) * height;
    for (plane = 0; plane < 4; plane++)
    {
        memcpy(&vgadata[plane][dest], source, count);
        source += count;
    }
}


//===========================================================================


/*
=================
=
= VL_MemToScreen
=
= Draws a block of data to the screen.
=
=================
*/

void VL_MemToScreen(byte* source, int16_t width, int16_t height, int16_t x, int16_t y)
{
    byte* screen;
    uint16_t dest, plane, i;

    width >>= 2;
    dest = bufferofs + ylookup[y] + (x >> 2);
    plane = x & 3;

    for (i = 0; i < 4; i++)
    {
        screen = &vgadata[plane][dest];
        for (y = 0; y < height; y++, screen += linewidth, source += width)
            memcpy(screen, source, width);

        plane = (plane + 1) & 3;
    }
}

//==========================================================================

/*
=================
=
= VL_LatchToScreen
=
=================
*/

void VL_LatchToScreen(uint16_t source, int16_t width, int16_t height, int16_t x, int16_t y)
{
    uint16_t dest, i;

    dest = bufferofs + ylookup[y] + (x >> 2);

    for (i = 0; i < height; i++)
    {
        memcpy(&vgadata[0][dest], &vgadata[0][source], width);
        memcpy(&vgadata[1][dest], &vgadata[1][source], width);
        memcpy(&vgadata[2][dest], &vgadata[2][source], width);
        memcpy(&vgadata[3][dest], &vgadata[3][source], width);

        dest += linewidth;
        source += width;
    }
}

//===========================================================================

/*
=================
=
= VL_ScreenToScreen
=
= Basic block copy routine.  Copies one block of screen memory to another
=
=================
*/

void VL_ScreenToScreen(uint16_t source, uint16_t dest, int16_t width, int16_t height)
{
    int16_t i;

    for (i = 0; i < height; i++)
    {
        memcpy(&vgadata[0][dest], &vgadata[0][source], width);
        memcpy(&vgadata[1][dest], &vgadata[1][source], width);
        memcpy(&vgadata[2][dest], &vgadata[2][source], width);
        memcpy(&vgadata[3][dest], &vgadata[3][source], width);

        source += linewidth;
        dest += linewidth;
    }
}


//===========================================================================

