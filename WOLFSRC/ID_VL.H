// ID_VL.H

#include <stdint.h>

// wolf compatability

#define MS_Quit Quit

void Quit(char* error);

//===========================================================================

#define VGA_PLANES          4
#define VGA_PLANE_SIZE      0x10000

//===========================================================================

#define SCREENWIDTH     80          // default screen width in bytes
#define MAXSCANLINES    200         // size of ylookup table

#define CHARWIDTH       2
#define TILEWIDTH       4

//===========================================================================

extern uint16_t bufferofs;          // all drawing is reletive to this
extern uint16_t displayofs, pelpan; // last setscreen coordinates

extern uint8_t* screenseg;

extern uint16_t linewidth;
extern uint16_t ylookup[MAXSCANLINES];

extern boolean  screenfaded;
//extern uint16_t bordercolor;

//===========================================================================

//
// VGA hardware routines
//

void VGA_Write(uint16_t mask, uint16_t addr, uint16_t color);

void VL_Startup(void);
void VL_Shutdown(void);

void VL_SetVGAPlaneMode(void);
void VL_ClearVideo(byte color); // trigger VL_Refresh()

void VL_SetLineWidth(uint16_t width);
void VL_WaitVBL(uint16_t vbls);
void VL_SetScreen(uint16_t crtc, uint16_t pelpan); // trigger VL_Refresh()
void VL_Refresh(void);

void VL_FillPalette(int16_t red, int16_t green, int16_t blue); // trigger VL_Refresh()
void VL_SetColor(int16_t color, int16_t red, int16_t green, int16_t blue);
void VL_GetColor(int16_t color, int16_t* red, int16_t* green, int16_t* blue);
void VL_SetPalette(byte* palette); // trigger VL_Refresh()
void VL_GetPalette(byte* palette);
void VL_FadeOut(int16_t start, int16_t end, int16_t red, int16_t green, int16_t blue, int16_t steps);
void VL_FadeIn(int16_t start, int16_t end, byte* palette, int16_t steps);
//void VL_ColorBorder(int16_t color);

void VL_Plot(int16_t x, int16_t y, int16_t color);
void VL_Hlin(uint16_t x, uint16_t y, uint16_t width, uint16_t color);
void VL_Vlin(int16_t x, int16_t y, int16_t height, int16_t color);
void VL_Bar(int16_t x, int16_t y, int16_t width, int16_t height, int16_t color);

void VL_MemToLatch(byte* source, int16_t width, int16_t height, uint16_t dest);
void VL_MemToScreen(byte* source, int16_t width, int16_t height, int16_t x, int16_t y);
void VL_LatchToScreen(uint16_t source, int16_t width, int16_t height, int16_t x, int16_t y);
void VL_ScreenToScreen(uint16_t source, uint16_t dest, int16_t width, int16_t height);

