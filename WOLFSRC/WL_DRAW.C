// WL_DRAW.C

#include "WL_DEF.H"
//#include <DOS.H>
#pragma hdrstop

//#define DEBUGWALLS
//#define DEBUGTICS

/*
=============================================================================

                         LOCAL CONSTANTS

=============================================================================
*/

// the door is the last picture before the sprites
#define DOORWALL	(PMSpriteStart-8)

#define ACTORSIZE	0x4000

/*
=============================================================================

                         GLOBAL VARIABLES

=============================================================================
*/


#ifdef DEBUGWALLS
uint16_t    screenloc[3] = { 0,0,0 };
#else
uint16_t    screenloc[3] = { PAGE1START,PAGE2START,PAGE3START };
#endif
uint16_t    freelatch = FREESTART;

int32_t     lasttimecount;
int32_t     frameon;

uint16_t    wallheight[MAXVIEWWIDTH];

fixed       tileglobal = TILEGLOBAL;
fixed       mindist = MINDIST;


//
// math tables
//
int16_t     pixelangle[MAXVIEWWIDTH];
int32_t     finetangent[FINEANGLES / 4];
fixed       sintable[ANGLES + ANGLES / 4],
            *costable = sintable + (ANGLES / 4);

//
// refresh variables
//
fixed       viewx, viewy;			// the focal point
int16_t     viewangle;
fixed       viewsin, viewcos;



fixed       FixedByFrac(fixed a, fixed b);
void        TransformActor(objtype* ob);
void        BuildTables(void);
void        ClearScreen(void);
int16_t     CalcRotate(objtype* ob);
void        DrawScaleds(void);
void        CalcTics(void);
void        FixOfs(void);
void        ThreeDRefresh(void);



//
// wall optimization variables
//
int16_t     lastside;		// true for vertical
int32_t     lastintercept;
int16_t     lasttilehit;


//
// ray tracing variables
//
int16_t     focaltx, focalty, viewtx, viewty;

int16_t     midangle, angle;
uint16_t    xpartial, ypartial;
uint16_t    xpartialup, xpartialdown, ypartialup, ypartialdown;
uint16_t    xinttile, yinttile;

uint16_t    tilehit;
uint16_t    pixx;

int16_t	    xtile, ytile;
int16_t     xtilestep, ytilestep;
int32_t     xintercept, yintercept;
int32_t     xstep, ystep;

int16_t     horizwall[MAXWALLTILES], vertwall[MAXWALLTILES];


/*
=============================================================================

                         LOCAL VARIABLES

=============================================================================
*/


//void AsmRefresh(void);			// in WL_DR_A.ASM

#define DEG90   900
#define DEG180  1800
#define DEG270  2700
#define DEG360  3600
#define OP_JLE  0x7e
#define OP_JGE  0x7d

void HitVertWall(void);
void HitHorizWall(void);
void HitHorizDoor(void);
void HitVertDoor(void);
void HitHorizPWall(void);
void HitVertPWall(void);

int32_t xpartialbyystep(word xpartial)
{
    int32_t ystephi = ystep >> 16;
    int32_t ysteplo = ystep & 0xFFFF;
    int32_t xpartiallo = xpartial;

    return ystephi * xpartiallo +
        (ysteplo * xpartiallo >> 16);
}

int32_t ypartialbyxstep(word ypartial)
{
    int32_t xstephi = xstep >> 16;
    int32_t xsteplo = xstep & 0xFFFF;
    int32_t ypartiallo = ypartial;

    return xstephi * ypartiallo +
        (xsteplo * ypartiallo >> 16);
}

void AsmRefresh()
{
    int16_t angle; // angle of the ray through pixx
    int16_t xspot; // xspot (yinttile<<6)+xtile (index into tilemap and spotvis)
    int16_t yspot; // yspot (xinttile<<6)+ytile (index into tilemap and spotvis)

    byte horizop, vertop;
    int16_t temp16;
    int32_t temp32;

    horizop = OP_JLE;
    vertop = OP_JLE;

    for (pixx = 0; pixx < viewwidth; ++pixx)
    {
        // Setup to trace a ray through pixx view pixel
        angle = midangle + pixelangle[pixx];
        if (angle < 0)
        {
            // -90 - -1 degree arc
            angle += FINEANGLES;
            goto entry360;
        }
        if (angle < DEG90)
        {
            // 0-89 degree arc
        entry90:
            xtilestep = 1;
            ytilestep = -1;
            horizop = OP_JGE;
            vertop = OP_JLE;
            xstep = finetangent[DEG90 - 1 - angle];
            ystep = -finetangent[angle];
            xpartial = xpartialup;
            ypartial = ypartialdown;
            goto initvars;
        }
        if (angle < DEG180)
        {
            // 90-179 degree arc
            xtilestep = -1;
            ytilestep = -1;
            horizop = OP_JLE;
            vertop = OP_JLE;
            xstep = -finetangent[angle - DEG90];
            ystep = -finetangent[DEG180 - 1 - angle];
            xpartial = xpartialdown;
            ypartial = ypartialdown;
            goto initvars;
        }
        if (angle < DEG270)
        {
            // 180-269 degree arc
            xtilestep = -1;
            ytilestep = 1;
            horizop = OP_JLE;
            vertop = OP_JGE;
            xstep = -finetangent[DEG270 - 1 - angle];
            ystep = finetangent[angle - DEG180];
            xpartial = xpartialdown;
            ypartial = ypartialup;
            goto initvars;
        }
        if (angle < DEG360)
        {
            // 270-359 degree arc
        entry360:
            xtilestep = 1;
            ytilestep = 1;
            horizop = OP_JGE;
            vertop = OP_JGE;
            xstep = finetangent[angle - DEG270];
            ystep = finetangent[DEG360 - 1 - angle];
            xpartial = xpartialup;
            ypartial = ypartialup;
            goto initvars;
        }
        angle -= FINEANGLES;
        goto entry90;

        // initialise variables for intersection testing
    initvars:
        yintercept = xpartialbyystep(xpartial);
        yintercept += viewy;
        xtile = focaltx + xtilestep;
        xspot = (xtile << 6) + yinttile;
        xintercept = ypartialbyxstep(ypartial);
        xintercept += viewx;
        ytile = focalty + ytilestep;
        yspot = (xinttile << 6) + ytile;

        // trace along this angle until we hit a wall
        // CORE LOOP!

        // check intersections with vertical walls
    vertcheck:
        if (vertop == OP_JLE)
            if ((yintercept >> 16) <= ytile) // (ytilestep==-1)
                goto horizentry;
        if (vertop == OP_JGE)
            if ((yintercept >> 16) >= ytile) // (ytilestep==1)
                goto horizentry;
    vertentry:
        if (*((byte*)tilemap + xspot) == 0)
        {
        passvert:
            *((byte*)spotvis + xspot) = true;
            xtile += xtilestep;
            yintercept += ystep;
            xspot = (xtile << 6) + yinttile;
            goto vertcheck;
        }

        tilehit = *((byte*)tilemap + xspot);
        if (tilehit < 0)
            goto vertdoor;
        xintercept = xtile << 16;
        yintercept = (ytile << 16) + (yintercept & 0xFFFF);
        HitVertWall();
        continue;

        // check intersections with horizontal walls
    horizcheck:
        if (horizop == OP_JLE)
            if ((xintercept >> 16) <= xtile) // (xtilestep==-1)
                goto vertentry;
        if (horizop == OP_JGE)
            if ((xintercept >> 16) >= xtile) // (xtilestep==1)
                goto vertentry;
    horizentry:
        if (*((byte*)tilemap + yspot) == 0)
        {
        passhoriz:
            *((byte*)spotvis + yspot) = true;
            ytile += ytilestep;
            xintercept += xstep;
            yspot = (xinttile << 6) + ytile;
            goto horizcheck;
        }

        tilehit = *((byte*)tilemap + yspot);
        if (tilehit < 0)
            goto horizdoor;
        xintercept = (xtile << 16) + (xintercept & 0xFFFF);
        yintercept = ytile << 16;
        HitHorizWall();
        continue;

        // hit a special horizontal wall, so find which coordinate a door would be
        // intersected at, and check to see if the door is open past that point
    horizdoor:
        if (tilehit & 0x40) // both high bits set == pushable wall
            goto horizpushwall;
        // strip high bit
        // index into word width door table
        temp16 = tilehit & 0x7F;
        // half a step gets to door position
        // add half step to current intercept
        temp32 = (xstep >> 1) + xintercept;
        if ((xintercept >> 16) != (temp32 >> 16)) // is it still in the same tile?
            // midpoint is outside tile, so it hit the side of the wall before a door
            goto passhoriz; // continue tracing
        // the trace hit the door plane at pixel position (temp32 & 0xFFFF), see if the door is
        // closed that much
        if ((temp32 & 0xFFFF) < doorposition[temp16]) // position of leading edge of door
            goto passhoriz;
        // draw the door
        xintercept = (xintercept & 0xFFFF0000) + (temp32 & 0xFFFF); // save pixel intercept position
        yintercept = (ytile << 16) + 0x8000; // intercept in middle of tile
        HitHorizDoor();
        continue;

        // hit a sliding horizontal wall
    horizpushwall:
        temp32 = xstep * pwallpos; // multiply xstep by pwallmove (0-63)
        temp32 >>= 6; // then divide by 64 to accomplish a fixed point multiplication
        temp32 += xintercept; // add partial step to current intercept
        if ((xintercept >> 16) != (temp32 >> 16)) // is it still in the same tile?
            goto passhoriz; // no, it hit the side
        // draw the pushable wall at the new height
        xintercept = temp32; // save pixel intercept position
        yintercept = ytile << 16;
        HitHorizPWall();
        continue;

        // hit a special vertical wall, so find which coordinate a door would be
        // intersected at, and check to see if the door is open past that point
    vertdoor:
        if (tilehit & 0x40) // both high bits set == pushable wall
            goto vertpushwall;
        // strip high bit
        // index into word width doorposition
        temp16 = tilehit & 0x7F;
        // half a step gets to door position
        // add half step to current intercept pos
        temp32 = (ystep >> 1) + yintercept;
        if ((yintercept >> 16) != (temp32 >> 16)) // is it still in the same tile?
            // midpoint is outside tile, so it hit the side of the wall before a door
            goto passvert;
        // the trace hit the door plane at pixel position (temp32 & 0xFFFF), see if the door is
        // closed that much
        if ((temp32 & 0xFFFF) < doorposition[temp16]) // position of leading edge of door
            goto passvert; // continue tracing
        // draw the door
        yintercept = (yintercept & 0xFFFF0000) + (temp32 & 0xFFFF); // save pixel intercept position
        xintercept = (xtile << 16) + 0x8000; // intercept in middle of tile
        HitVertDoor();
        continue;

        // hit a sliding vertical wall
    vertpushwall:
        temp32 = ystep * pwallpos; // multiply ystep by pwallmove (0-63)
        temp32 >>= 6; // then divide by 64 to accomplish a fixed point multiplication
        temp32 += yintercept; // add partial step to current intercept
        if ((yintercept >> 16) != (temp32 >> 16)) // is it still in the same tile?
            goto passvert; // no, it hit the side
        // draw the pushable wall at the new height
        yintercept = temp32; // save pixel intercept position
        xintercept = xtile << 16;
        HitVertPWall();
        continue;
    }
}

/*
============================================================================

               3 - D  DEFINITIONS

============================================================================
*/


//==========================================================================


/*
========================
=
= FixedByFrac
=
= multiply a 16/16 bit, 2's complement fixed point number by a 16 bit
= fraction, passed as a signed magnitude 32 bit number
=
========================
*/

fixed FixedByFrac(fixed a, fixed b)
{
    // ((ah << 16) + al) * ((bh << 16) + bl) >> 16 =
    // ((ah << 16) * (bh << 16) >> 16) +
    // ((ah << 16) * bl >> 16) +
    // (al * (bh << 16) >> 16) +
    // (al * bl >> 16) =
    // (ah * bh << 16) +
    // ah * bl +
    // al * bh +
    // (al * bl >> 16)
    int32_t ah = a >> 16;
    int32_t al = a & 0xFFFF;
    int32_t bh = b >> 16;
    int32_t bl = b & 0xFFFF;
    return
        (ah * bh << 16) +
        ah * bl +
        al * bh +
        (al * bl >> 16);
}


//==========================================================================

/*
========================
=
= TransformActor
=
= Takes paramaters:
=   gx,gy		: globalx/globaly of point
=
= globals:
=   viewx,viewy		: point of view
=   viewcos,viewsin	: sin/cos of viewangle
=   scale		: conversion from global value to screen value
=
= sets:
=   screenx,transx,transy,screenheight: projected edge location and size
=
========================
*/


//
// transform actor
//
void TransformActor(objtype* ob)
{
    fixed gx, gy, gxt, gyt, nx, ny;
    int32_t	temp;

    //
    // translate point to view centered coordinates
    //
    gx = ob->x - viewx;
    gy = ob->y - viewy;

    //
    // calculate newx
    //
    gxt = FixedByFrac(gx, viewcos);
    gyt = FixedByFrac(gy, viewsin);
    nx = gxt - gyt - ACTORSIZE;		// fudge the shape forward a bit, because
                                // the midpoint could put parts of the shape
                                // into an adjacent wall

    //
    // calculate newy
    //
    gxt = FixedByFrac(gx, viewsin);
    gyt = FixedByFrac(gy, viewcos);
    ny = gyt + gxt;

    //
    // calculate perspective ratio
    //
    ob->transx = nx;
    ob->transy = ny;

    if (nx < mindist)			// too close, don't overflow the divide
    {
        ob->viewheight = 0;
        return;
    }

    ob->viewx = centerx + ny * scale / nx;	// DEBUG: use assembly divide

    //
    // calculate height (heightnumerator/(nx>>8))
    //
    //asm	mov	ax, [WORD PTR heightnumerator]
    //asm	mov	dx, [WORD PTR heightnumerator + 2]
    //asm	idiv[WORD PTR nx + 1]			// nx>>8
    //asm	mov[WORD PTR temp], ax
    //asm	mov[WORD PTR temp + 2], dx
    temp = heightnumerator / (nx >> 8);

    ob->viewheight = temp;
}

//==========================================================================

/*
========================
=
= TransformTile
=
= Takes paramaters:
=   tx,ty		: tile the object is centered in
=
= globals:
=   viewx,viewy		: point of view
=   viewcos,viewsin	: sin/cos of viewangle
=   scale		: conversion from global value to screen value
=
= sets:
=   screenx,transx,transy,screenheight: projected edge location and size
=
= Returns true if the tile is withing getting distance
=
========================
*/

boolean TransformTile(int16_t tx, int16_t ty, int16_t* dispx, int16_t* dispheight)
{
    fixed gx, gy, gxt, gyt, nx, ny;
    int32_t	temp;

    //
    // translate point to view centered coordinates
    //
    gx = ((int32_t)tx << TILESHIFT) + 0x8000 - viewx;
    gy = ((int32_t)ty << TILESHIFT) + 0x8000 - viewy;

    //
    // calculate newx
    //
    gxt = FixedByFrac(gx, viewcos);
    gyt = FixedByFrac(gy, viewsin);
    nx = gxt - gyt - 0x2000;		// 0x2000 is size of object

    //
    // calculate newy
    //
    gxt = FixedByFrac(gx, viewsin);
    gyt = FixedByFrac(gy, viewcos);
    ny = gyt + gxt;


    //
    // calculate perspective ratio
    //
    if (nx < mindist)			// too close, don't overflow the divide
    {
        *dispheight = 0;
        return false;
    }

    *dispx = centerx + ny * scale / nx;	// DEBUG: use assembly divide

    //
    // calculate height (heightnumerator/(nx>>8))
    //
    //asm	mov	ax, [WORD PTR heightnumerator]
    //asm	mov	dx, [WORD PTR heightnumerator + 2]
    //asm	idiv[WORD PTR nx + 1]			// nx>>8
    //asm	mov[WORD PTR temp], ax
    //asm	mov[WORD PTR temp + 2], dx
    temp = heightnumerator / (nx >> 8);

    *dispheight = temp;

    //
    // see if it should be grabbed
    //
    if (nx<TILEGLOBAL && ny>-TILEGLOBAL / 2 && ny < TILEGLOBAL / 2)
        return true;
    else
        return false;
}

//==========================================================================

/*
====================
=
= CalcHeight
=
= Calculates the height of xintercept,yintercept from viewx,viewy
=
====================
*/

//#pragma warn -rvl			// I stick the return value in with ASMs

int16_t	CalcHeight(void)
{
    fixed gxt, gyt, nx;
    int32_t	gx, gy;

    gx = xintercept - viewx;
    gxt = FixedByFrac(gx, viewcos);

    gy = yintercept - viewy;
    gyt = FixedByFrac(gy, viewsin);

    nx = gxt - gyt;

    //
    // calculate perspective ratio (heightnumerator/(nx>>8))
    //
    if (nx < mindist)
        nx = mindist;			// don't let divide overflow

    //asm	mov	ax, [WORD PTR heightnumerator]
    //asm	mov	dx, [WORD PTR heightnumerator + 2]
    //asm	idiv[WORD PTR nx + 1]			// nx>>8
    return heightnumerator / (nx >> 8);
}


//==========================================================================

/*
===================
=
= ScaleWallLine
=
===================
*/

void ScaleWallLine(byte mask, uint16_t dest, byte* source, uint16_t height)
{
    int32_t step, texel;

    step = (64 << 16) / height;
    texel = 0;

    if (height > viewheight)
    {
        texel += step * (height - viewheight) >> 1;
        height = viewheight;
    }

    dest += ((viewheight - height) >> 1) * SCREENBWIDE;

    while (height)
    {
        VGA_Write(mask, dest, source[texel >> 16]);
        dest += SCREENBWIDE;
        texel += step;
        height--;
    }
}

/*
===================
=
= ScalePost
=
===================
*/

//
// bit mask tables for drawing scaled strips up to eight pixels wide
//


byte	mapmasks1[4][8] = {
{1 ,3 ,7 ,15,15,15,15,15},
{2 ,6 ,14,14,14,14,14,14},
{4 ,12,12,12,12,12,12,12},
{8 ,8 ,8 ,8 ,8 ,8 ,8 ,8} };

byte	mapmasks2[4][8] = {
{0 ,0 ,0 ,0 ,1 ,3 ,7 ,15},
{0 ,0 ,0 ,1 ,3 ,7 ,15,15},
{0 ,0 ,1 ,3 ,7 ,15,15,15},
{0 ,1 ,3 ,7 ,15,15,15,15} };

byte	mapmasks3[4][8] = {
{0 ,0 ,0 ,0 ,0 ,0 ,0 ,0},
{0 ,0 ,0 ,0 ,0 ,0 ,0 ,1},
{0 ,0 ,0 ,0 ,0 ,0 ,1 ,3},
{0 ,0 ,0 ,0 ,0 ,1 ,3 ,7} };

byte*       postsourceaddress;
uint16_t    postsourceoffset;
uint16_t    postx;
uint16_t    postwidth;

void ScalePost(void)		// VGA version
{
    uint16_t    height, dest;
    int16_t     maskindex;
    byte        mask;

    //asm	mov	ax, SCREENSEG
    //asm	mov	es, ax

    //asm	mov	bx, [postx]
    //asm	shl	bx, 1
    //asm	mov	bp, WORD PTR[wallheight + bx]		// fractional height (low 3 bits frac)
    height = wallheight[postx];

    //asm	and bp, 0xfff8				// bp = heightscaler*4
    //asm	shr	bp, 1
    height = (height & 0xfff8) >> 1;

    //asm	cmp	bp, [maxscaleshl2]
    //asm	jle	heightok
    //asm	mov	bp, [maxscaleshl2]
    //heightok:
    //if (height > maxscaleshl2)
    //    height = maxscaleshl2;

    //asm	add	bp, OFFSET fullscalefarcall

    //
    // scale a byte wide strip of wall
    //
    //asm	mov	bx, [postx]
    //asm	mov	di, bx
    //asm	shr	di, 2						// X in bytes
    //asm	add	di, [bufferofs]
    dest = (postx >> 2) + bufferofs;

    //asm	and bx, 3
    //asm	shl	bx, 3						// bx = pixel*8+pixwidth
    //asm	add	bx, [postwidth]
    //asm	mov	al, BYTE PTR[mapmasks1 - 1 + bx]	// -1 because no widths of 0
    maskindex = -1 + ((postx & 3) << 3) + postwidth;
    mask = *((byte*)mapmasks1 + maskindex);

    //asm	mov	dx, SC_INDEX + 1
    //asm	out	dx, al						// set bit mask register
    //asm	lds	si, DWORD PTR[postsource]
    //asm	call DWORD PTR[bp]				// scale the line of pixels
    ScaleWallLine(mask, dest, postsourceaddress + postsourceoffset, height);

    //asm	mov	al, BYTE PTR[ss:mapmasks2 - 1 + bx]   // -1 because no widths of 0
    //asm	or al, al
    //asm	jz	nomore
    mask = *((byte*)mapmasks2 + maskindex);
    if (mask == 0)
        goto nomore;

    //
    // draw a second byte for vertical strips that cross two bytes
    //
    //asm	inc	di
    ++dest;

    //asm	out	dx, al						// set bit mask register
    //asm	call DWORD PTR[bp]				// scale the line of pixels
    ScaleWallLine(mask, dest, postsourceaddress + postsourceoffset, height);

    //asm	mov	al, BYTE PTR[ss:mapmasks3 - 1 + bx]	// -1 because no widths of 0
    //asm	or al, al
    //asm	jz	nomore
    mask = *((byte*)mapmasks3 + maskindex);
    if (mask == 0)
        goto nomore;

    //
    // draw a third byte for vertical strips that cross three bytes
    //
    //asm	inc	di
    ++dest;

    //asm	out	dx, al						// set bit mask register
    //asm	call DWORD PTR[bp]				// scale the line of pixels
    ScaleWallLine(mask, dest, postsourceaddress + postsourceoffset, height);

nomore:
    //asm	mov	ax, ss
    //asm	mov	ds, ax
    ;
}

void  FarScalePost(void)				// just so other files can call
{
    ScalePost();
}


/*
====================
=
= HitVertWall
=
= tilehit bit 7 is 0, because it's not a door tile
= if bit 6 is 1 and the adjacent tile is a door tile, use door side pic
=
====================
*/

void HitVertWall(void)
{
    int16_t     wallpic;
    uint16_t    texture;

    texture = (yintercept >> 4) & 0xfc0;
    if (xtilestep == -1)
    {
        texture = 0xfc0 - texture;
        xintercept += TILEGLOBAL;
    }
    wallheight[pixx] = CalcHeight();

    if (lastside == 1 && lastintercept == xtile && lasttilehit == tilehit)
    {
        // in the same wall type as last time, so check for optimized draw
        if (texture == postsourceoffset)
        {
            // wide scale
            postwidth++;
            wallheight[pixx] = wallheight[pixx - 1];
            return;
        }
        else
        {
            ScalePost();
            postsourceoffset = texture;
            postwidth = 1;
            postx = pixx;
        }
    }
    else
    {
        // new wall
        if (lastside != -1)				// if not the first scaled post
            ScalePost();

        lastside = true;
        lastintercept = xtile;

        lasttilehit = tilehit;
        postx = pixx;
        postwidth = 1;

        if (tilehit & 0x40)
        {								// check for adjacent doors
            ytile = yintercept >> TILESHIFT;
            if (tilemap[xtile - xtilestep][ytile] & 0x80)
                wallpic = DOORWALL + 3;
            else
                wallpic = vertwall[tilehit & ~0x40];
        }
        else
            wallpic = vertwall[tilehit];

        postsourceaddress = PM_GetPage(wallpic);
        postsourceoffset = texture;

    }
}


/*
====================
=
= HitHorizWall
=
= tilehit bit 7 is 0, because it's not a door tile
= if bit 6 is 1 and the adjacent tile is a door tile, use door side pic
=
====================
*/

void HitHorizWall(void)
{
    int16_t     wallpic;
    uint16_t    texture;

    texture = (xintercept >> 4) & 0xfc0;
    if (ytilestep == -1)
        yintercept += TILEGLOBAL;
    else
        texture = 0xfc0 - texture;
    wallheight[pixx] = CalcHeight();

    if (lastside == 0 && lastintercept == ytile && lasttilehit == tilehit)
    {
        // in the same wall type as last time, so check for optimized draw
        if (texture == postsourceoffset)
        {
            // wide scale
            postwidth++;
            wallheight[pixx] = wallheight[pixx - 1];
            return;
        }
        else
        {
            ScalePost();
            postsourceoffset = texture;
            postwidth = 1;
            postx = pixx;
        }
    }
    else
    {
        // new wall
        if (lastside != -1)				// if not the first scaled post
            ScalePost();

        lastside = 0;
        lastintercept = ytile;

        lasttilehit = tilehit;
        postx = pixx;
        postwidth = 1;

        if (tilehit & 0x40)
        {								// check for adjacent doors
            xtile = xintercept >> TILESHIFT;
            if (tilemap[xtile][ytile - ytilestep] & 0x80)
                wallpic = DOORWALL + 2;
            else
                wallpic = horizwall[tilehit & ~0x40];
        }
        else
            wallpic = horizwall[tilehit];

        postsourceaddress = PM_GetPage(wallpic);
        postsourceoffset = texture;
    }

}

//==========================================================================

/*
====================
=
= HitHorizDoor
=
====================
*/

void HitHorizDoor(void)
{
    uint16_t	texture, doorpage, doornum;

    doornum = tilehit & 0x7f;
    texture = ((xintercept - doorposition[doornum]) >> 4) & 0xfc0;

    wallheight[pixx] = CalcHeight();

    if (lasttilehit == tilehit)
    {
        // in the same door as last time, so check for optimized draw
        if (texture == postsourceoffset)
        {
            // wide scale
            postwidth++;
            wallheight[pixx] = wallheight[pixx - 1];
            return;
        }
        else
        {
            ScalePost();
            postsourceoffset = texture;
            postwidth = 1;
            postx = pixx;
        }
    }
    else
    {
        if (lastside != -1)         // if not the first scaled post
            ScalePost();            // draw last post
        // first pixel in this door
        lastside = 2;
        lasttilehit = tilehit;
        postx = pixx;
        postwidth = 1;

        switch (doorobjlist[doornum].lock)
        {
        case dr_normal:
            doorpage = DOORWALL;
            break;
        case dr_lock1:
        case dr_lock2:
        case dr_lock3:
        case dr_lock4:
            doorpage = DOORWALL + 6;
            break;
        case dr_elevator:
            doorpage = DOORWALL + 4;
            break;
        }

        postsourceaddress = PM_GetPage(doorpage);
        postsourceoffset = texture;
    }
}

//==========================================================================

/*
====================
=
= HitVertDoor
=
====================
*/

void HitVertDoor(void)
{
    uint16_t	texture, doorpage, doornum;

    doornum = tilehit & 0x7f;
    texture = ((yintercept - doorposition[doornum]) >> 4) & 0xfc0;

    wallheight[pixx] = CalcHeight();

    if (lasttilehit == tilehit)
    {
        // in the same door as last time, so check for optimized draw
        if (texture == postsourceoffset)
        {
            // wide scale
            postwidth++;
            wallheight[pixx] = wallheight[pixx - 1];
            return;
        }
        else
        {
            ScalePost();
            postsourceoffset = texture;
            postwidth = 1;
            postx = pixx;
        }
    }
    else
    {
        if (lastside != -1)         // if not the first scaled post
            ScalePost();            // draw last post
        // first pixel in this door
        lastside = 2;
        lasttilehit = tilehit;
        postx = pixx;
        postwidth = 1;

        switch (doorobjlist[doornum].lock)
        {
        case dr_normal:
            doorpage = DOORWALL;
            break;
        case dr_lock1:
        case dr_lock2:
        case dr_lock3:
        case dr_lock4:
            doorpage = DOORWALL + 6;
            break;
        case dr_elevator:
            doorpage = DOORWALL + 4;
            break;
        }

        postsourceaddress = PM_GetPage(doorpage + 1);
        postsourceoffset = texture;
    }
}

//==========================================================================


/*
====================
=
= HitHorizPWall
=
= A pushable wall in action has been hit
=
====================
*/

void HitHorizPWall(void)
{
    int16_t		wallpic;
    uint16_t	texture, offset;

    texture = (xintercept >> 4) & 0xfc0;
    offset = pwallpos << 10;
    if (ytilestep == -1)
        yintercept += TILEGLOBAL - offset;
    else
    {
        texture = 0xfc0 - texture;
        yintercept += offset;
    }

    wallheight[pixx] = CalcHeight();

    if (lasttilehit == tilehit)
    {
        // in the same wall type as last time, so check for optimized draw
        if (texture == postsourceoffset)
        {
            // wide scale
            postwidth++;
            wallheight[pixx] = wallheight[pixx - 1];
            return;
        }
        else
        {
            ScalePost();
            postsourceoffset = texture;
            postwidth = 1;
            postx = pixx;
        }
    }
    else
    {
        // new wall
        if (lastside != -1)				// if not the first scaled post
            ScalePost();

        lasttilehit = tilehit;
        postx = pixx;
        postwidth = 1;

        wallpic = horizwall[tilehit & 63];

        postsourceaddress = PM_GetPage(wallpic);
        postsourceoffset = texture;
    }

}


/*
====================
=
= HitVertPWall
=
= A pushable wall in action has been hit
=
====================
*/

void HitVertPWall(void)
{
    int16_t     wallpic;
    uint16_t    texture, offset;

    texture = (yintercept >> 4) & 0xfc0;
    offset = pwallpos << 10;
    if (xtilestep == -1)
    {
        xintercept += TILEGLOBAL - offset;
        texture = 0xfc0 - texture;
    }
    else
        xintercept += offset;

    wallheight[pixx] = CalcHeight();

    if (lasttilehit == tilehit)
    {
        // in the same wall type as last time, so check for optimized draw
        if (texture == postsourceoffset)
        {
            // wide scale
            postwidth++;
            wallheight[pixx] = wallheight[pixx - 1];
            return;
        }
        else
        {
            ScalePost();
            postsourceoffset = texture;
            postwidth = 1;
            postx = pixx;
        }
    }
    else
    {
        // new wall
        if (lastside != -1)				// if not the first scaled post
            ScalePost();

        lasttilehit = tilehit;
        postx = pixx;
        postwidth = 1;

        wallpic = vertwall[tilehit & 63];

        postsourceaddress = PM_GetPage(wallpic);
        postsourceoffset = texture;
    }

}

//==========================================================================

//==========================================================================

#if 0
/*
=====================
=
= ClearScreen
=
=====================
*/

void ClearScreen(void)
{
    uint16_t floor = egaFloor[gamestate.episode * 10 + mapon],
        ceiling = egaCeiling[gamestate.episode * 10 + mapon];

    //
    // clear the screen
    //
    asm	mov	dx, GC_INDEX
    asm	mov	ax, GC_MODE + 256 * 2		// read mode 0, write mode 2
    asm	out	dx, ax
    asm	mov	ax, GC_BITMASK + 255 * 256
    asm	out	dx, ax

    asm	mov	dx, 40
    asm	mov	ax, [viewwidth]
    asm	shr	ax, 3
    asm	sub	dx, ax					// dx = 40-viewwidth/8

    asm	mov	bx, [viewwidth]
    asm	shr	bx, 4					// bl = viewwidth/16
    asm	mov	bh, BYTE PTR[viewheight]
    asm	shr	bh, 1					// half height

    asm	mov	ax, [ceiling]
    asm	mov	es, [screenseg]
    asm	mov	di, [bufferofs]

    toploop:
    asm	mov	cl, bl
    asm	rep	stosw
    asm	add	di, dx
    asm	dec	bh
    asm	jnz	toploop

    asm	mov	bh, BYTE PTR[viewheight]
    asm	shr	bh, 1					// half height
    asm	mov	ax, [floor]

    bottomloop:
    asm	mov	cl, bl
    asm	rep	stosw
    asm	add	di, dx
    asm	dec	bh
    asm	jnz	bottomloop


    asm	mov	dx, GC_INDEX
    asm	mov	ax, GC_MODE + 256 * 10		// read mode 1, write mode 2
    asm	out	dx, ax
    asm	mov	al, GC_BITMASK
    asm	out	dx, al

}
#endif
//==========================================================================

uint16_t vgaCeiling[] =
{
#ifndef SPEAR
 0x1d1d,0x1d1d,0x1d1d,0x1d1d,0x1d1d,0x1d1d,0x1d1d,0x1d1d,0x1d1d,0xbfbf,
 0x4e4e,0x4e4e,0x4e4e,0x1d1d,0x8d8d,0x4e4e,0x1d1d,0x2d2d,0x1d1d,0x8d8d,
 0x1d1d,0x1d1d,0x1d1d,0x1d1d,0x1d1d,0x2d2d,0xdddd,0x1d1d,0x1d1d,0x9898,

 0x1d1d,0x9d9d,0x2d2d,0xdddd,0xdddd,0x9d9d,0x2d2d,0x4d4d,0x1d1d,0xdddd,
 0x7d7d,0x1d1d,0x2d2d,0x2d2d,0xdddd,0xd7d7,0x1d1d,0x1d1d,0x1d1d,0x2d2d,
 0x1d1d,0x1d1d,0x1d1d,0x1d1d,0xdddd,0xdddd,0x7d7d,0xdddd,0xdddd,0xdddd
#else
 0x6f6f,0x4f4f,0x1d1d,0xdede,0xdfdf,0x2e2e,0x7f7f,0x9e9e,0xaeae,0x7f7f,
 0x1d1d,0xdede,0xdfdf,0xdede,0xdfdf,0xdede,0xe1e1,0xdcdc,0x2e2e,0x1d1d,0xdcdc
#endif
};

/*
=====================
=
= VGAClearScreen
=
=====================
*/

void VGAClearScreen(void)
{
    uint16_t ceiling = vgaCeiling[gamestate.episode * 10 + mapon];

    uint16_t width, halfheight, dest, i, j;

    width = viewwidth >> 2;
    halfheight = viewheight >> 1;
    dest = bufferofs;

    for (i = 0; i < halfheight; ++i)
    {
        for (j = 0; j < width; ++j)
            VGA_Write(0xF, dest + j, ceiling);
        dest += SCREENBWIDE;
    }

    for (i = 0; i < halfheight; ++i)
    {
        for (j = 0; j < width; ++j)
            VGA_Write(0xF, dest + j, 0x1919);
        dest += SCREENBWIDE;
    }

    //
    // clear the screen
    //
    //asm	mov	dx, SC_INDEX
    //asm	mov	ax, SC_MAPMASK + 15 * 256	// write through all planes
    //asm	out	dx, ax

    //asm	mov	dx, 80
    //asm	mov	ax, [viewwidth]
    //asm	shr	ax, 2
    //asm	sub	dx, ax					// dx = 40-viewwidth/2

    //asm	mov	bx, [viewwidth]
    //asm	shr	bx, 3					// bl = viewwidth/8
    //asm	mov	bh, BYTE PTR[viewheight]
    //asm	shr	bh, 1					// half height

    //asm	mov	es, [screenseg]
    //asm	mov	di, [bufferofs]
    //asm	mov	ax, [ceiling]

    //toploop:
    //asm	mov	cl, bl
    //asm	rep	stosw
    //asm	add	di, dx
    //asm	dec	bh
    //asm	jnz	toploop

    //asm	mov	bh, BYTE PTR[viewheight]
    //asm	shr	bh, 1					// half height
    //asm	mov	ax, 0x1919

    //bottomloop:
    //asm	mov	cl, bl
    //asm	rep	stosw
    //asm	add	di, dx
    //asm	dec	bh
    //asm	jnz	bottomloop
}

//==========================================================================

/*
=====================
=
= CalcRotate
=
=====================
*/

int16_t	CalcRotate(objtype* ob)
{
    int16_t	angle, viewangle;

    // this isn't exactly correct, as it should vary by a trig value,
    // but it is close enough with only eight rotations

    viewangle = player->angle + (centerx - ob->viewx) / 8;

    if (ob->obclass == rocketobj || ob->obclass == hrocketobj)
        angle = (viewangle - 180) - ob->angle;
    else
        angle = (viewangle - 180) - dirangle[ob->dir];

    angle += ANGLES / 16;
    while (angle >= ANGLES)
        angle -= ANGLES;
    while (angle < 0)
        angle += ANGLES;

    if (ob->state->rotate == 2)             // 2 rotation pain frame
        return 4 * (angle / (ANGLES / 2));        // seperated by 3 (art layout...)

    return angle / (ANGLES / 8);
}

typedef struct
{
    byte offset;
    byte length;
} linecmd_t;

void DecodeSpriteLine(byte* linecmds, byte line[64])
{
    linecmd_t*  command;
    int16_t     count, i;

    count = 0;
    while (1)
    {
        command = (linecmd_t*)linecmds;
        if (command->length == 0)
            break;
        else
        {
            for (i = 0; i < command->offset; ++i)
                line[count++] = 0xFF;
            for (i = 0; i < command->length; ++i)
                line[count++] = linecmds[sizeof(linecmd_t) + i];
        }
        linecmds += sizeof(linecmd_t) + command->length;
    }

    for (i = count; i < 64; ++i)
        line[count++] = 0xFF;
}

/*
=======================
=
= ScaleShape
=
=======================
*/

void ScaleShape(int16_t xcenter, int16_t shapenum, uint16_t height)
{
    t_compshape *shape;
    int32_t     step, texu0, texv0, texu, texv;
    int16_t     scale, x0, x1, x, y0, y1, y, u, lastu;
    byte        *linecmds;
    byte        spriteline[64], texel;
    uint16_t    mask, dest;

    shape = PM_GetSpritePage(shapenum);

    scale = height >> 3;    // low three bits are fractional
    //if (!scale || scale > maxscale)
    //    return;				// too close or far away
    if (!scale)
        return;				// too far away

    step = (64 << 16) / scale;
    texu0 = 0;
    texv0 = 0;

    // view area - clipping - x
    x0 = xcenter - (scale >> 1);
    x1 = x0 + scale;
    if (x0 < 0)
    {
        texu0 += step * -x0;
        x0 = 0;
    }
    if (x1 > viewwidth - 1)
        x1 = viewwidth - 1;

    // view area - clipping - y
    y0 = (viewheight - scale) >> 1;
    y1 = y0 + scale;
    if (scale > viewheight)
    {
        texv0 += step * (scale - viewheight) >> 1;
        y0 = 0;
        y1 = viewheight - 1;
    }

    lastu = -1;

    texu = texu0;
    for (x = x0; x <= x1; ++x)
    {
        if (wallheight[x] >= height)
            continue;       // obscured by closer wall

        u = texu >> 16;

        if (u < shape->leftpix)
            continue;       // left limit of sprite
        if (u > shape->rightpix)
            continue;       // right limit of sprite

        // do in only once for column
        if (lastu != u)
        {
            lastu = u;
            // decode the sprite commands into a column of texels
            linecmds = (byte*)shape + shape->dataofs[u - shape->leftpix];
            DecodeSpriteLine(linecmds, spriteline);
        }

        mask = 1 << (x & 3);
        dest = bufferofs + (x >> 2) + y0 * SCREENBWIDE;
        texv = texv0;
        for (y = y0; y <= y1; ++y)
        {
            texel = spriteline[texv >> 16];
            if (texel != 0xFF)
                VGA_Write(mask, dest, texel);
            dest += SCREENBWIDE;
            texv += step;
        }

        texu += step;
    }
}

/*
=======================
=
= SimpleScaleShape
=
= NO CLIPPING, height in pixels
=
*/

void SimpleScaleShape(int16_t xcenter, int16_t shapenum, uint16_t height)
{
    t_compshape *shape;
    int32_t     step, texu, texv;
    int16_t     scale, x0, x1, x, y0, y1, y, u, lastu;
    byte        *linecmds;
    byte        spriteline[64], texel;
    uint16_t    mask, dest;

    shape = PM_GetSpritePage(shapenum);

    scale = height >> 1;

    step = (64 << 16) / scale;

    x0 = xcenter - (scale >> 1);
    x1 = x0 + scale;
    y0 = (viewheight - scale) >> 1;
    y1 = y0 + scale;

    lastu = -1;

    texu = 0;
    for (x = x0; x <= x1; ++x)
    {
        u = texu >> 16;

        if (u < shape->leftpix)
            continue;       // left limit of sprite
        if (u > shape->rightpix)
            continue;       // right limit of sprite

        // do in only once for column
        if (lastu != u)
        {
            lastu = u;
            // decode the sprite commands into a column of texels
            linecmds = (byte*)shape + shape->dataofs[u - shape->leftpix];
            DecodeSpriteLine(linecmds, spriteline);
        }

        mask = 1 << (x & 3);
        dest = bufferofs + (x >> 2) + y0 * SCREENBWIDE;
        texv = 0;
        for (y = y0; y <= y1; ++y)
        {
            texel = spriteline[texv >> 16];
            if (texel != 0xFF)
                VGA_Write(mask, dest, texel);
            dest += SCREENBWIDE;
            texv += step;
        }

        texu += step;
    }
}

/*
=====================
=
= DrawScaleds
=
= Draws all objects that are visable
=
=====================
*/

#define MAXVISABLE	50

typedef struct
{
    int16_t	viewx,
        viewheight,
        shapenum;
} visobj_t;

visobj_t	vislist[MAXVISABLE], *visptr, *visstep, *farthest;

void DrawScaleds(void)
{
    int16_t     i, /*j,*/ least, numvisable, height;
    //memptr      shape;
    byte        *tilespot, *visspot;
    //int16_t     shapenum;
    uint16_t    spotloc;

    statobj_t   *statptr;
    objtype     *obj;

    visptr = &vislist[0];

    //
    // place static objects
    //
    for (statptr = &statobjlist[0]; statptr != laststatobj; statptr++)
    {
        if ((visptr->shapenum = statptr->shapenum) == -1)
            continue;						// object has been deleted

        if (!*statptr->visspot)
            continue;						// not visable

        if (TransformTile(statptr->tilex, statptr->tiley
            , &visptr->viewx, &visptr->viewheight) && statptr->flags & FL_BONUS)
        {
            GetBonus(statptr);
            continue;
        }

        if (!visptr->viewheight)
            continue;						// to close to the object

        if (visptr < &vislist[MAXVISABLE - 1])	// don't let it overflow
            visptr++;
    }

    //
    // place active objects
    //
    for (obj = player->next; obj; obj = obj->next)
    {
        if (!(visptr->shapenum = obj->state->shapenum))
            continue;						// no shape

        spotloc = (obj->tilex << 6) + obj->tiley;	// optimize: keep in struct?
        visspot = &spotvis[0][0] + spotloc;
        tilespot = &tilemap[0][0] + spotloc;

        //
        // could be in any of the nine surrounding tiles
        //
        if (*visspot
            || (*(visspot - 1) && !*(tilespot - 1))
            || (*(visspot + 1) && !*(tilespot + 1))
            || (*(visspot - 65) && !*(tilespot - 65))
            || (*(visspot - 64) && !*(tilespot - 64))
            || (*(visspot - 63) && !*(tilespot - 63))
            || (*(visspot + 65) && !*(tilespot + 65))
            || (*(visspot + 64) && !*(tilespot + 64))
            || (*(visspot + 63) && !*(tilespot + 63)))
        {
            obj->active = true;
            TransformActor(obj);
            if (!obj->viewheight)
                continue;						// too close or far away

            visptr->viewx = obj->viewx;
            visptr->viewheight = obj->viewheight;
            if (visptr->shapenum == -1)
                visptr->shapenum = obj->temp1;	// special shape

            if (obj->state->rotate)
                visptr->shapenum += CalcRotate(obj);

            if (visptr < &vislist[MAXVISABLE - 1])	// don't let it overflow
                visptr++;
            obj->flags |= FL_VISABLE;
        }
        else
            obj->flags &= ~FL_VISABLE;
    }

    //
    // draw from back to front
    //
    numvisable = visptr - &vislist[0];

    if (!numvisable)
        return;									// no visable objects

    for (i = 0; i < numvisable; i++)
    {
        least = 32000;
        for (visstep = &vislist[0]; visstep < visptr; visstep++)
        {
            height = visstep->viewheight;
            if (height < least)
            {
                least = height;
                farthest = visstep;
            }
        }
        //
        // draw farthest
        //
        ScaleShape(farthest->viewx, farthest->shapenum, farthest->viewheight);

        farthest->viewheight = 32000;
    }

}

//==========================================================================

/*
==============
=
= DrawPlayerWeapon
=
= Draw the player's hands
=
==============
*/

int16_t	weaponscale[NUMWEAPONS] = { SPR_KNIFEREADY,SPR_PISTOLREADY
    ,SPR_MACHINEGUNREADY,SPR_CHAINREADY };

void DrawPlayerWeapon(void)
{
    int16_t	shapenum;

#ifndef SPEAR
    if (gamestate.victoryflag)
    {
        if (player->state == &s_deathcam && (TimeCount_Get() & 32))
            SimpleScaleShape(viewwidth / 2, SPR_DEATHCAM, viewheight + 1);
        return;
    }
#endif

    if (gamestate.weapon != -1)
    {
        shapenum = weaponscale[gamestate.weapon] + gamestate.weaponframe;
        SimpleScaleShape(viewwidth / 2, shapenum, viewheight + 1);
    }

    if (demorecord || demoplayback)
        SimpleScaleShape(viewwidth / 2, SPR_DEMO, viewheight + 1);
}


//==========================================================================


/*
=====================
=
= CalcTics
=
=====================
*/

void CalcTics(void)
{
    int32_t	newtime/*, oldtimecount*/;

    //
    // calculate tics since last refresh for adaptive timing
    //
    if (lasttimecount > TimeCount_Get())
        TimeCount_Set(lasttimecount);	// if the game was paused a LONG time

    do
    {
        newtime = TimeCount_Get();
        tics = newtime - lasttimecount;
    } while (!tics);			// make sure at least one tic passes

    lasttimecount = newtime;

#ifdef FILEPROFILE
    strcpy(scratch, "\tTics:");
    itoa(tics, str, 10);
    strcat(scratch, str);
    strcat(scratch, "\n");
    write(profilehandle, scratch, strlen(scratch));
#endif

    if (tics > MAXTICS)
    {
        TimeCount_Set(TimeCount_Get() - (tics - MAXTICS));
        tics = MAXTICS;
    }
}


//==========================================================================


/*
========================
=
= FixOfs
=
========================
*/

void	FixOfs(void)
{
    VW_ScreenToScreen(displayofs, bufferofs, viewwidth / 8, viewheight);
}


//==========================================================================


/*
====================
=
= WallRefresh
=
====================
*/

void WallRefresh(void)
{
    //
    // set up variables for this view
    //
    viewangle = player->angle;
    midangle = viewangle * (FINEANGLES / ANGLES);
    viewsin = sintable[viewangle];
    viewcos = costable[viewangle];
    viewx = player->x - FixedByFrac(focallength, viewcos);
    viewy = player->y + FixedByFrac(focallength, viewsin);

    focaltx = viewx >> TILESHIFT;
    focalty = viewy >> TILESHIFT;

    viewtx = player->x >> TILESHIFT;
    viewty = player->y >> TILESHIFT;

    xpartialdown = viewx & (TILEGLOBAL - 1);
    xpartialup = (uint16_t)(TILEGLOBAL - xpartialdown);
    ypartialdown = viewy & (TILEGLOBAL - 1);
    ypartialup = (uint16_t)(TILEGLOBAL - ypartialdown);

    lastside = -1;			// the first pixel is on a new wall
    AsmRefresh();
    ScalePost();			// no more optimization on last post
}

//==========================================================================

/*
========================
=
= ThreeDRefresh
=
========================
*/

void	ThreeDRefresh(void)
{
    //int16_t tracedir;

    // this wouldn't need to be done except for my debugger/video wierdness
    //outportb(SC_INDEX, SC_MAPMASK);

    //
    // clear out the traced array
    //
    memset(spotvis, 0, sizeof(spotvis));

    bufferofs += screenofs;

    //
    // follow the walls from there to the right, drawwing as we go
    //
    VGAClearScreen();

    WallRefresh();

    //
    // draw all the scaled images
    //
    DrawScaleds();      // draw scaled stuff
    DrawPlayerWeapon(); // draw player's hands

    //
    // show screen and time last cycle
    //
    if (fizzlein)
    {
        FizzleFade(bufferofs, displayofs + screenofs, viewwidth, viewheight, 20, false);
        fizzlein = false;

        TimeCount_Set(0);		// don't make a big tic count
        lasttimecount = 0;

    }

    bufferofs -= screenofs;
    displayofs = bufferofs;

    //asm	cli
    //asm	mov	cx, [displayofs]
    //asm	mov	dx, 3d4h		// CRTC address register
    //asm	mov	al, 0ch		// start address high register
    //asm	out	dx, al
    //asm	inc	dx
    //asm	mov	al, ch
    //asm	out	dx, al   	// set the high byte
    //asm	sti
    VW_SetScreen(displayofs, pelpan);

    bufferofs += SCREENSIZE;
    if (bufferofs > PAGE3START)
        bufferofs = PAGE1START;

    frameon++;
    PM_NextFrame();
}


//===========================================================================

