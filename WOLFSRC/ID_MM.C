// NEWMM.C

/*
=============================================================================

           ID software memory manager
           --------------------------

Primary coder: John Carmack

RELIES ON
---------
Quit (char *error) function


WORK TO DO
----------
MM_SizePtr to change the size of a given pointer

Multiple purge levels utilized

EMS / XMS unmanaged routines

=============================================================================
*/

#include "ID_HEADS.H"
#pragma hdrstop

/*
=============================================================================

                            LOCAL INFO

=============================================================================
*/

#ifndef SPEAR
#define MAXMEM          235000L
#else
#define MAXMEM          257000L
#endif

#define USEHEAP         0

#define LOCKBIT         0x80        // if set in attributes, block cannot be moved
#define PURGEBITS       3           // 0-3 level, 0= unpurgable, 3= purge first
#define PURGEMASK       0xfffffffc
#define BASEATTRIBUTES  0           // unlocked, non purgable

typedef struct mmblockstruct
{
    uintptr_t start, length;
    memptr* useptr; // pointer to the segment start
    struct mmblockstruct* next;
    uint32_t attributes;
} mmblocktype;


//#define GETNEWBLOCK {if(!(mmnew=mmfree))Quit("MM_GETNEWBLOCK: No free blocks!")\
//	;mmfree=mmfree->next;}

#define GETNEWBLOCK {if(!mmfree)MML_ClearBlock();mmnew=mmfree;mmfree=mmfree->next;}

#define FREEBLOCK(x) {*x->useptr=NULL;x->next=mmfree;mmfree=x;}

/*
=============================================================================

                         GLOBAL VARIABLES

=============================================================================
*/

memptr		bufferseg;

void		(*beforesort) (void);
void		(*aftersort) (void);

/*
=============================================================================

                         LOCAL VARIABLES

=============================================================================
*/

boolean     mmstarted;

#if USEHEAP==1
void* heap;
#else
uint32_t mmbuffer[(MAXMEM + sizeof(uint32_t) - 1) / sizeof(uint32_t)];
#endif

mmblocktype mmblocks[MAXBLOCKS], * mmhead, * mmfree, * mmrover, * mmnew;

//==========================================================================

//
// local prototypes
//

void MML_UseSpace(uintptr_t start, uintptr_t length);
void MML_ClearBlock(void);

//==========================================================================


/*
======================
=
= MML_UseSpace
=
= Marks a range of address as usable by the memory manager
=
======================
*/

void MML_UseSpace(uintptr_t start, uintptr_t length)
{
    mmblocktype* scan, * last;
    uintptr_t oldend;
    uintptr_t extra;

    scan = last = mmhead;
    mmrover = mmhead;		// reset rover to start of memory

    //
    // search for the block that contains the range of segments
    //
    while (scan->start + scan->length < start)
    {
        last = scan;
        scan = scan->next;
    }

    //
    // take the given range out of the block
    //
    oldend = scan->start + scan->length;
    extra = oldend - (start + length);
    if (extra < 0)
        Quit("MML_UseSpace: Segment spans two blocks!");

    if (start == scan->start)
    {
        last->next = scan->next; // unlink block
        FREEBLOCK(scan);
        scan = last;
    }
    else
        scan->length = start - scan->start; // shorten block

    if (extra > 0)
    {
        GETNEWBLOCK;
        mmnew->useptr = NULL;

        mmnew->next = scan->next;
        scan->next = mmnew;
        mmnew->start = start + length;
        mmnew->length = extra;
        mmnew->attributes = LOCKBIT;
    }
}

//==========================================================================

/*
====================
=
= MML_ClearBlock
=
= We are out of blocks, so free a purgable block
=
====================
*/

void MML_ClearBlock(void)
{
    mmblocktype* scan;

    scan = mmhead->next;

    while (scan)
    {
        if (!(scan->attributes & LOCKBIT) && (scan->attributes & PURGEBITS))
        {
            MM_FreePtr(scan->useptr);
            return;
        }
        scan = scan->next;
    }

    Quit("MM_ClearBlock: No purgable blocks!");
}


//==========================================================================

/*
===================
=
= MM_Startup
=
= Grabs MAXMEM space
= Allocates bufferseg misc buffer
=
===================
*/

void MM_Startup(void)
{
    int16_t i;
    uintptr_t start, length;

    if (mmstarted)
        MM_Shutdown();

    mmstarted = true;

    //
    // set up the linked list (everything in the free list;
    //
    mmhead = NULL;
    mmfree = &mmblocks[0];
    for (i = 0; i < MAXBLOCKS - 1; i++)
        mmblocks[i].next = &mmblocks[i + 1];
    mmblocks[i].next = NULL;

    //
    // locked block of all memory until we punch out free space
    //
    GETNEWBLOCK;
    mmhead = mmnew; // this will allways be the first node
    mmnew->start = 0;
    mmnew->length = UINTPTR_MAX;
    mmnew->attributes = LOCKBIT;
    mmnew->next = NULL;
    mmrover = mmhead;

    //
    // get MAXMEM memory
    //
    length = MAXMEM;
#if USEHEAP==1
    start = (uintptr_t)(heap = malloc(length));
#else
    start = (uintptr_t)mmbuffer;
#endif
    MML_UseSpace(start, length);

    //
    // allocate the misc buffer
    //
    mmrover = mmhead; // start looking for space after low block

    MM_GetPtr(&bufferseg, BUFFERSIZE);
}

//==========================================================================

/*
====================
=
= MM_Shutdown
=
= Frees all conventional, EMS, and XMS allocated
=
====================
*/

void MM_Shutdown(void)
{
    if (!mmstarted)
        return;

#if USEHEAP==1
    free(heap);
#endif
}

//==========================================================================

/*
====================
=
= MM_GetPtr
=
= Allocates an unlocked, unpurgable block
=
====================
*/

void MM_GetPtr(memptr* baseptr, size_t size)
{
    mmblocktype* scan, * lastscan, * endscan, * purge, * next;
    int16_t search;
    uintptr_t start;

    GETNEWBLOCK; // fill in start and next after a spot is found
    mmnew->length = size;
    mmnew->useptr = baseptr;
    mmnew->attributes = BASEATTRIBUTES;

    scan = NULL;
    lastscan = NULL;
    endscan = NULL;

    for (search = 0; search < 3; search++)
    {
        //
        // first search:	try to allocate right after the rover, then on up
        // second search: 	search from the head pointer up to the rover
        // third search:	compress memory, then scan from start
        if (search == 1 && mmrover == mmhead)
            search++;

        switch (search)
        {
        case 0:
            lastscan = mmrover;
            scan = mmrover->next;
            endscan = NULL;
            break;
        case 1:
            lastscan = mmhead;
            scan = mmhead->next;
            endscan = mmrover;
            break;
        case 2:
            MM_SortMem();
            lastscan = mmhead;
            scan = mmhead->next;
            endscan = NULL;
            break;
        }

        start = lastscan->start + lastscan->length;

        while (scan != endscan)
        {
            if (scan->start - start >= size)
            {
                //
                // got enough space between the end of lastscan and
                // the start of scan, so throw out anything in the middle
                // and allocate the new block
                //
                purge = lastscan->next;
                lastscan->next = mmnew;
                mmnew->start = *(uintptr_t*)baseptr = start;
                mmnew->next = scan;
                while (purge != scan)
                {	// free the purgable block
                    next = purge->next;
                    FREEBLOCK(purge);
                    purge = next;		// purge another if not at scan
                }
                mmrover = mmnew;
                return;	// good allocation!
            }

            //
            // if this block is purge level zero or locked, skip past it
            //
            if ((scan->attributes & LOCKBIT)
                || !(scan->attributes & PURGEBITS))
            {
                lastscan = scan;
                start = lastscan->start + lastscan->length;
            }


            scan = scan->next;		// look at next line
        }
    }

    Quit("MM_GetPtr: Out of memory!");
}

//==========================================================================

/*
====================
=
= MM_FreePtr
=
= Deallocates an unlocked, purgable block
=
====================
*/

void MM_FreePtr(memptr* baseptr)
{
    mmblocktype* scan, * last;

    last = mmhead;
    scan = last->next;

    if (baseptr == mmrover->useptr)	// removed the last allocated block
        mmrover = mmhead;

    while (scan->useptr != baseptr && scan)
    {
        last = scan;
        scan = scan->next;
    }

    if (!scan)
        Quit("MM_FreePtr: Block not found!");

    last->next = scan->next;

    FREEBLOCK(scan);
}
//==========================================================================

/*
=====================
=
= MM_SetPurge
=
= Sets the purge level for a block (locked blocks cannot be made purgable)
=
=====================
*/

void MM_SetPurge(memptr* baseptr, int32_t purge)
{
    mmblocktype* start;

    start = mmrover;

    do
    {
        if (mmrover->useptr == baseptr)
            break;

        mmrover = mmrover->next;

        if (!mmrover)
            mmrover = mmhead;
        else if (mmrover == start)
            Quit("MM_SetPurge: Block not found!");

    } while (1);

    mmrover->attributes &= ~PURGEBITS;
    mmrover->attributes |= purge;
}

//==========================================================================

/*
=====================
=
= MM_SetLock
=
= Locks / unlocks the block
=
=====================
*/

void MM_SetLock(memptr* baseptr, boolean locked)
{
    mmblocktype* start;

    start = mmrover;

    do
    {
        if (mmrover->useptr == baseptr)
            break;

        mmrover = mmrover->next;

        if (!mmrover)
            mmrover = mmhead;
        else if (mmrover == start)
            Quit("MM_SetLock: Block not found!");

    } while (1);

    mmrover->attributes &= ~LOCKBIT;
    mmrover->attributes |= locked * LOCKBIT;
}

//==========================================================================

/*
=====================
=
= MM_SortMem
=
= Throws out all purgable stuff and compresses movable blocks
=
=====================
*/

void MM_SortMem(void)
{
    mmblocktype* scan, * last, * next;
    uintptr_t start;// , length, source, dest;
    int16_t playing;

    //
    // lock down a currently playing sound
    //
    playing = SD_SoundPlaying();
    if (playing)
    {
        switch (SoundMode)
        {
        case sdm_PC:
            playing += STARTPCSOUNDS;
            break;
        case sdm_AdLib:
            playing += STARTADLIBSOUNDS;
            break;
        }
        MM_SetLock(&(memptr)audiosegs[playing], true);
    }


    SD_StopSound();

    if (beforesort)
        beforesort();

    scan = mmhead;

    last = NULL;		// shut up compiler warning

    while (scan)
    {
        if (scan->attributes & LOCKBIT)
        {
            //
            // block is locked, so try to pile later blocks right after it
            //
            start = scan->start + scan->length;
        }
        else
        {
            if (scan->attributes & PURGEBITS)
            {
                //
                // throw out the purgable block
                //
                next = scan->next;
                FREEBLOCK(scan);
                last->next = next;
                scan = next;
                continue;
            }
            else
            {
                //
                // push the non purgable block on top of the last moved block
                //
                if (scan->start != start)
                {
                    memmove((void*)start, (void*)scan->start, scan->length);

                    scan->start = start;
                    *(uintptr_t*)scan->useptr = start;
                }
                start = scan->start + scan->length;
            }
        }

        last = scan;
        scan = scan->next;		// go to next block
    }

    mmrover = mmhead;

    if (aftersort)
        aftersort();

    if (playing)
        MM_SetLock(&(memptr)audiosegs[playing], false);
}


//==========================================================================

/*
=====================
=
= MM_ShowMemory
=
=====================
*/

void MM_ShowMemory(void)
{
    mmblocktype* scan;
    uint16_t color, temp, x, y;
    uintptr_t end;

    temp = bufferofs;
    bufferofs = displayofs;
    scan = mmhead;

    end = UINTPTR_MAX;

    while (scan)
    {
        if (scan->attributes & PURGEBITS)
            color = 5;		// dark purple = purgable
        else
            color = 9;		// medium blue = non purgable
        if (scan->attributes & LOCKBIT)
            color = 12;		// red = locked
        if (scan->start <= end)
            Quit("MM_ShowMemory: Memory block order currupted!");
        end = scan->length - 1;
        y = scan->start / 320;
        x = scan->start % 320;
        VW_Hlin(x, x + end, y, color);
        VW_Plot(x, y, 15);
        if (scan->next && scan->next->start > end + 1)
            VW_Hlin(x + end + 1, x + (scan->next->start - scan->start), y, 0);	// black = free

        scan = scan->next;
    }

    VW_FadeIn();
    IN_Ack();

    bufferofs = temp;
}

//==========================================================================

/*
=====================
=
= MM_DumpData
=
=====================
*/

void MM_DumpData(void)
{
    mmblocktype* scan, * best;
    uintptr_t lowest, oldlowest;
    uintptr_t owner;
    char lock, purge;
    FILE* dumpfile;


    dumpfile = fopen("MMDUMP.TXT", "w");
    if (!dumpfile)
        Quit("MM_DumpData: Couldn't open MMDUMP.TXT!");

    lowest = UINTPTR_MAX;
    do
    {
        oldlowest = lowest;
        lowest = UINTPTR_MAX;

        scan = mmhead;
        best = NULL;
        while (scan)
        {
            owner = (uintptr_t)scan->useptr;

            if (owner && owner < lowest && owner > oldlowest)
            {
                best = scan;
                lowest = owner;
            }

            scan = scan->next;
        }

        if (lowest != UINTPTR_MAX)
        {
            if (best->attributes & PURGEBITS)
                purge = 'P';
            else
                purge = '-';
            if (best->attributes & LOCKBIT)
                lock = 'L';
            else
                lock = '-';
            fprintf(dumpfile, "0x%p (%c%c) = %u\n"
                , (void*)lowest, lock, purge, best->length);
        }

    } while (lowest != UINTPTR_MAX);

    fclose(dumpfile);
    Quit("MMDUMP.TXT created.");
}

//==========================================================================


/*
======================
=
= MM_UnusedMemory
=
= Returns the total free space without purging
=
======================
*/

int32_t MM_UnusedMemory(void)
{
    uintptr_t free;
    mmblocktype* scan;

    free = 0;
    scan = mmhead;

    while (scan->next)
    {
        free += scan->next->start - (scan->start + scan->length);
        scan = scan->next;
    }

    return free;
}

//==========================================================================


/*
======================
=
= MM_TotalFree
=
= Returns the total free space with purging
=
======================
*/

int32_t MM_TotalFree(void)
{
    uintptr_t free;
    mmblocktype* scan;

    free = 0;
    scan = mmhead;

    while (scan->next)
    {
        if ((scan->attributes & PURGEBITS) && !(scan->attributes & LOCKBIT))
            free += scan->length;
        free += scan->next->start - (scan->start + scan->length);
        scan = scan->next;
    }

    return free;
}

//==========================================================================
