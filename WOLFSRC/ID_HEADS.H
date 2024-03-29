// ID_GLOB.H

#include <stdint.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define __ID_GLOB__
#include "VERSION.H"

//--------------------------------------------------------------------------

extern	uint8_t		signon[];

#define	introscn	*signon

#ifdef JAPAN
#ifdef JAPDEMO
#include "FOREIGN\JAPAN\GFXV_WJ1.H"
#else
#include "FOREIGN\JAPAN\GFXV_WJ6.H"
#endif
#include "AUDIOWL6.H"
#include "MAPSWL6.H"
#else

#ifndef SPEAR

#include "GFXV_WL6.H"
#include "AUDIOWL6.H"
#include "MAPSWL6.H"

#else

#ifndef SPEARDEMO
#include "GFXV_SOD.H"
#include "AUDIOSOD.H"
#include "MAPSSOD.H"
#else
#include "GFXV_SDM.H"
#include "AUDIOSDM.H"
#include "MAPSSDM.H"
#endif

#endif
#endif
//-----------------


#define GREXT	"VGA"

//
//	ID Engine
//	Types.h - Generic types, #defines, etc.
//	v1.0d1
//

typedef	enum { false, true } boolean;
typedef	uint8_t byte;
typedef	uint16_t word;
typedef	uint32_t longword;
typedef	byte* Ptr;

typedef	struct
{
    int16_t	x, y;
} Point;
typedef	struct
{
    Point	ul, lr;
} Rect;

#define	nil	((void *)0)

#include "bridge.h"

#include "ID_MM.H"
#include "ID_PM.H"
#include "ID_CA.H"
#include "ID_VL.H"
#include "ID_VH.H"
#include "ID_IN.H"
#include "ID_SD.H"
#include "ID_US.H"


void	Quit(char* error);		// defined in user program

//
// replacing refresh manager with custom routines
//

#define	PORTTILESWIDE		20      // all drawing takes place inside a
#define	PORTTILESHIGH		13		// non displayed port of this size

#define UPDATEWIDE			PORTTILESWIDE
#define UPDATEHIGH			PORTTILESHIGH

#define	MAXTICS				10
#define DEMOTICS			4

#define	UPDATETERMINATE	0x0301

extern	uint16_t	mapwidth, mapheight, tics;
extern	boolean		compatability;

extern	byte        *updateptr;
extern	uint16_t	uwidthtable[UPDATEHIGH];
extern	uint16_t	blockstarts[UPDATEWIDE * UPDATEHIGH];

extern	byte		fontcolor, backcolor;

#define SETFONTCOLOR(f,b) fontcolor=f;backcolor=b;

