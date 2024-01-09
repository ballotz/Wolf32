//
//	ID_PM.C
//	Id Engine's Page Manager v1.0
//	Primary coder: Jason Blochowiak
//

#include "ID_HEADS.H"
#pragma hdrstop

//	Main Mem specific variables
boolean         MainPresent;
memptr          MainMemPages[PMMaxMainMem];
PMBlockAttr     MainMemUsed[PMMaxMainMem];
int16_t         MainPagesAvail;

//	File specific variables
char                PageFileName[13] = { "VSWAP." };
FileSystemHandle    PageFile;
word                ChunksInFile;
word                PMSpriteStart, PMSoundStart;

//	General usage variables
boolean         PMStarted, PMPanicMode, PMThrashing;
word            MainPagesUsed, PMNumBlocks;
int32_t         PMFrameCount;
PageListStruct* PMPages, * PMSegPages;

/////////////////////////////////////////////////////////////////////////////
//
//	Main memory code
//
/////////////////////////////////////////////////////////////////////////////

//
//	PM_SetMainMemPurge() - Sets the purge level for all allocated main memory
//		blocks. This shouldn't be called directly - the PM_LockMainMem() and
//		PM_UnlockMainMem() macros should be used instead.
//
void
PM_SetMainMemPurge(int16_t level)
{
    int16_t	i;

    for (i = 0; i < PMMaxMainMem; i++)
        if (MainMemPages[i])
            MM_SetPurge(&MainMemPages[i], level);
}

//
//	PM_CheckMainMem() - If something besides the Page Mgr makes requests of
//		the Memory Mgr, some of the Page Mgr's blocks may have been purged,
//		so this function runs through the block list and checks to see if
//		any of the blocks have been purged. If so, it marks the corresponding
//		page as purged & unlocked, then goes through the block list and
//		tries to reallocate any blocks that have been purged.
//	This routine now calls PM_LockMainMem() to make sure that any allocation
//		attempts made during the block reallocation sweep don't purge any
//		of the other blocks. Because PM_LockMainMem() is called,
//		PM_UnlockMainMem() needs to be called before any other part of the
//		program makes allocation requests of the Memory Mgr.
//
void
PM_CheckMainMem(void)
{
    boolean allocfailed;
    int16_t i, n;
    memptr* p;
    PMBlockAttr* used;
    PageListStruct* page;

    if (!MainPresent)
        return;

    for (i = 0, page = PMPages; i < ChunksInFile; i++, page++)
    {
        n = page->mainPage;
        if (n != -1)						// Is the page using main memory?
        {
            if (!MainMemPages[n])			// Yep, was the block purged?
            {
                page->mainPage = -1;		// Yes, mark page as purged & unlocked
                page->locked = pml_Unlocked;
            }
        }
    }

    // Prevent allocation attempts from purging any of our other blocks
    PM_LockMainMem();
    allocfailed = false;
    for (i = 0, p = MainMemPages, used = MainMemUsed; i < PMMaxMainMem; i++, p++, used++)
    {
        if (!*p)							// If the page got purged
        {
            if (*used & pmba_Allocated)		// If it was allocated
            {
                *used &= ~pmba_Allocated;	// Mark as unallocated
                MainPagesAvail--;			// and decrease available count
            }

            if (*used & pmba_Used)			// If it was used
            {
                *used &= ~pmba_Used;		// Mark as unused
                MainPagesUsed--;			// and decrease used count
            }

            if (!allocfailed)
            {
                MM_BombOnError(false);
                MM_GetPtr(p, PMPageSize);		// Try to reallocate
                if (mmerror)					// If it failed,
                    allocfailed = true;			//  don't try any more allocations
                else							// If it worked,
                {
                    *used |= pmba_Allocated;	// Mark as allocated
                    MainPagesAvail++;			// and increase available count
                }
                MM_BombOnError(true);
            }
        }
    }
    if (mmerror)
        mmerror = false;
}

//
//	PML_StartupMainMem() - Allocates as much main memory as is possible for
//		the Page Mgr. The memory is allocated as non-purgeable, so if it's
//		necessary to make requests of the Memory Mgr, PM_UnlockMainMem()
//		needs to be called.
//
void
PML_StartupMainMem(void)
{
    int16_t i;
    memptr* p;

    MainPagesAvail = 0;
    MM_BombOnError(false);
    for (i = 0, p = MainMemPages; i < PMMaxMainMem; i++, p++)
    {
        MM_GetPtr(p, PMPageSize);
        if (mmerror)
            break;

        MainPagesAvail++;
        MainMemUsed[i] = pmba_Allocated;
    }
    MM_BombOnError(true);
    if (mmerror)
        mmerror = false;
    if (MainPagesAvail < PMMinMainMem)
        Quit("PM_SetupMainMem: Not enough main memory");
    MainPresent = true;
}

//
//	PML_ShutdownMainMem() - Frees all of the main memory blocks used by the
//		Page Mgr.
//
void
PML_ShutdownMainMem(void)
{
    int16_t i;
    memptr* p;

    // DEBUG - mark pages as unallocated & decrease page count as appropriate
    for (i = 0, p = MainMemPages; i < PMMaxMainMem; i++, p++)
        if (*p)
            MM_FreePtr(p);
}

/////////////////////////////////////////////////////////////////////////////
//
//	File management code
//
/////////////////////////////////////////////////////////////////////////////

//
//	PML_ReadFromFile() - Reads some data in from the page file
//
void
PML_ReadFromFile(byte* buf, int32_t offset, word length)
{
    if (!buf)
        Quit("PML_ReadFromFile: Null pointer");
    if (!offset)
        Quit("PML_ReadFromFile: Zero offset");
    if (FileSystem_Seek(PageFile, offset) != offset)
        Quit("PML_ReadFromFile: Seek failed");
    if (!CA_FarRead(PageFile, buf, length))
        Quit("PML_ReadFromFile: Read failed");
}

//
//	PML_OpenPageFile() - Opens the page file and sets up the page info
//
void
PML_OpenPageFile(void)
{
    int16_t i;
    int32_t size;
    void* buf;
    longword* offsetptr;
    word* lengthptr;
    PageListStruct* page;

    PageFile = FileSystem_Open(PageFileName, FileSystemRead + FileSystemBinary);
    if (FileSystem_ValidHandle(PageFile) == false)
        Quit("PML_OpenPageFile: Unable to open page file");

    // Read in header variables
    FileSystem_Read(PageFile, &ChunksInFile, sizeof(ChunksInFile));
    FileSystem_Read(PageFile, &PMSpriteStart, sizeof(PMSpriteStart));
    FileSystem_Read(PageFile, &PMSoundStart, sizeof(PMSoundStart));

    // Allocate and clear the page list
    PMNumBlocks = ChunksInFile;
    MM_GetPtr(&(memptr)PMSegPages, sizeof(PageListStruct) * PMNumBlocks);
    MM_SetLock(&(memptr)PMSegPages, true);
    PMPages = (PageListStruct*)PMSegPages;
    memset(PMPages, 0, sizeof(PageListStruct) * PMNumBlocks);

    // Read in the chunk offsets
    size = sizeof(longword) * ChunksInFile;
    MM_GetPtr(&buf, size);
    if (!CA_FarRead(PageFile, (byte*)buf, size))
        Quit("PML_OpenPageFile: Offset read failed");
    offsetptr = (longword*)buf;
    for (i = 0, page = PMPages; i < ChunksInFile; i++, page++)
        page->offset = *offsetptr++;
    MM_FreePtr(&buf);

    // Read in the chunk lengths
    size = sizeof(word) * ChunksInFile;
    MM_GetPtr(&buf, size);
    if (!CA_FarRead(PageFile, (byte*)buf, size))
        Quit("PML_OpenPageFile: Length read failed");
    lengthptr = (word*)buf;
    for (i = 0, page = PMPages; i < ChunksInFile; i++, page++)
        page->length = *lengthptr++;
    MM_FreePtr(&buf);
}

//
//  PML_ClosePageFile() - Closes the page file
//
void
PML_ClosePageFile(void)
{
    if (FileSystem_ValidHandle(PageFile))
        FileSystem_Close(PageFile);
    if (PMSegPages)
    {
        MM_SetLock(&(memptr)PMSegPages, false);
        MM_FreePtr(&(void*)PMSegPages);
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//	Allocation, etc., code
//
/////////////////////////////////////////////////////////////////////////////

//
//	PM_GetPageAddress() - Returns the address of a given page
//		Returns nil if block isn't cached into Main Memory
//
//
memptr
PM_GetPageAddress(int16_t pagenum)
{
    PageListStruct* page;

    page = &PMPages[pagenum];
    if (page->mainPage != -1)
        return(MainMemPages[page->mainPage]);
    else
        return(nil);
}

//
//	PML_GiveLRUPage() - Returns the page # of the least recently used
//		present & unlocked main page
//
int16_t
PML_GiveLRUPage()
{
    int16_t i, lru;
    uint32_t last;
    PageListStruct* page;

    for (i = 0, page = PMPages, lru = -1, last = UINT32_MAX; i < ChunksInFile; i++, page++)
    {
        if (
            (page->lastHit < last)
            && (page->mainPage != -1)
            && (page->locked == pml_Unlocked)
            )
        {
            last = page->lastHit;
            lru = i;
        }
    }

    if (lru == -1)
        Quit("PML_GiveLRUPage: LRU Search failed");
    return(lru);
}

//
//	PML_TransferPageSpace() - A page is being replaced, so give the new page
//		the old one's address space. Returns the address of the new page.
//
memptr
PML_TransferPageSpace(int orig, int new)
{
    memptr			addr;
    PageListStruct	*origpage, *newpage;

    if (orig == new)
        Quit("PML_TransferPageSpace: Identity replacement");

    origpage = &PMPages[orig];
    newpage = &PMPages[new];

    if (origpage->locked != pml_Unlocked)
        Quit("PML_TransferPageSpace: Killing locked page");

    if (origpage->mainPage == -1)
        Quit("PML_TransferPageSpace: Reusing non-existent page");

    // Get the address
    addr = PM_GetPageAddress(orig);

    // Steal the address
    newpage->mainPage = origpage->mainPage;

    // Mark replaced page as purged
    origpage->mainPage = -1;

    if (!addr)
        Quit("PML_TransferPageSpace: Zero replacement");

    return(addr);
}

//
//	PML_GetAPageBuffer() - A page buffer is needed. Either get it from the
//		main free pool, or use PML_GiveLRUPage() to find which page to
//		steal the buffer from. Returns a pointer to the page buffer, and
//		sets the fields inside the given page structure appropriately.
//
byte*
PML_GetAPageBuffer(int16_t pagenum)
{
    byte* addr = nil;
    int16_t i, n;
    PMBlockAttr* used;
    PageListStruct* page;

    page = &PMPages[pagenum];
    if (MainPagesUsed < MainPagesAvail)
    {
        // There's remaining main memory - use it
        for (i = 0, n = -1, used = MainMemUsed; i < PMMaxMainMem; i++, used++)
        {
            if ((*used & pmba_Allocated) && !(*used & pmba_Used))
            {
                n = i;
                *used |= pmba_Used;
                break;
            }
        }
        if (n == -1)
            Quit("PML_GetPageBuffer: MainPagesAvail lied");
        addr = MainMemPages[n];
        if (!addr)
            Quit("PML_GetPageBuffer: Purged main block");
        page->mainPage = n;
        MainPagesUsed++;
    }
    else
        addr = PML_TransferPageSpace(PML_GiveLRUPage(), pagenum);

    if (!addr)
        Quit("PML_GetPageBuffer: Search failed");
    return(addr);
}

//
//	PML_LoadPage() - A page is not in main.
//		Load it into main.
//
void
PML_LoadPage(int16_t pagenum)
{
    byte* addr;
    PageListStruct* page;

    addr = PML_GetAPageBuffer(pagenum);
    page = &PMPages[pagenum];
    PML_ReadFromFile(addr, page->offset, page->length);
}

//
//	PM_GetPage() - Returns the address of the page, loading it if necessary
//
memptr
PM_GetPage(int16_t pagenum)
{
    memptr result;

    if (pagenum >= ChunksInFile)
        Quit("PM_GetPage: Invalid page request");

    if (!(result = PM_GetPageAddress(pagenum)))
    {
        if (!PMPages[pagenum].offset)	// JDC: sparse page
            Quit("Tried to load a sparse page!");

        if (PMPages[pagenum].lastHit == PMFrameCount)
            PMThrashing++;

        PML_LoadPage(pagenum);
        result = PM_GetPageAddress(pagenum);
    }
    PMPages[pagenum].lastHit = PMFrameCount;

    return(result);
}

//
//	PM_SetPageLock() - Sets the lock type on a given page
//		pml_Unlocked: Normal, page can be purged
//		pml_Locked: Cannot be purged
//
void
PM_SetPageLock(int16_t pagenum, PMLockType lock)
{
    if (pagenum < PMSoundStart)
        Quit("PM_SetPageLock: Locking/unlocking non-sound page");

    PMPages[pagenum].locked = lock;
}

//
//	PM_Preload() - Loads as many pages as possible into memory.
//		Calls the update function after each load, indicating the current
//		page, and the total pages that need to be loaded (for thermometer).
//
void
PM_Preload(boolean(*update)(word current, word total))
{
    int16_t i, page;
    word current, total,
        mainfree, maintotal;

    maintotal = 0;
    mainfree = MainPagesAvail - MainPagesUsed;

    for (i = 0; i < ChunksInFile; i++)
    {
        if (!PMPages[i].offset)
            continue; // sparse

        if (PMPages[i].mainPage != -1)
            continue; // already in main mem

        if (mainfree)
        {
            maintotal++;
            mainfree--;
        }
    }


    total = maintotal;

    if (!total)
        return;

    page = 0;
    current = 0;

    //
    // cache main blocks
    //
    while (maintotal)
    {
        while (!PMPages[page].offset || PMPages[page].mainPage != -1)
            page++;

        if (page >= ChunksInFile)
            Quit("PM_Preload: Pages>=ChunksInFile");

        PM_GetPage(page);

        page++;
        current++;
        maintotal--;
        update(current, total);
    }

    update(total, total);
}

/////////////////////////////////////////////////////////////////////////////
//
//	General code
//
/////////////////////////////////////////////////////////////////////////////

//
//	PM_NextFrame() - Increments the frame counter and adjusts the thrash
//		avoidence variables
//
//		If currently in panic mode (to avoid thrashing), check to see if the
//			appropriate number of frames have passed since the last time that
//			we would have thrashed. If so, take us out of panic mode.
//
//
void
PM_NextFrame(void)
{
    int16_t	i;

    // Frame count overrun - kill the LRU hit entries & reset frame count
    if (++PMFrameCount >= INT32_MAX - 4)
    {
        for (i = 0; i < PMNumBlocks; i++)
            PMPages[i].lastHit = 0;
        PMFrameCount = 0;
    }

#if 0
    for (i = 0; i < PMSoundStart; i++)
    {
        if (PMPages[i].locked)
        {
            char buf[40];
            sprintf(buf, "PM_NextFrame: Page %d is locked", i);
            Quit(buf);
        }
    }
#endif

    if (PMPanicMode)
    {
        // DEBUG - set border color
        if ((!PMThrashing) && (!--PMPanicMode))
        {
            // DEBUG - reset border color
        }
    }
    if (PMThrashing >= PMThrashThreshold)
        PMPanicMode = PMUnThrashThreshold;
    PMThrashing = false;
}

//
//	PM_Reset() - Sets up caching structures
//
void
PM_Reset(void)
{
    int16_t i;
    PageListStruct* page;

    MainPagesUsed = 0;

    PMPanicMode = false;

    // Initialize page list
    for (i = 0, page = PMPages; i < PMNumBlocks; i++, page++)
    {
        page->mainPage = -1;
        page->locked = false;
    }
}

//
//	PM_Startup() - Start up the Page Mgr
//
void
PM_Startup(void)
{
    if (PMStarted)
        return;

    PML_OpenPageFile();

    PML_StartupMainMem();

    PM_Reset();

    PMStarted = true;
}

//
//	PM_Shutdown() - Shut down the Page Mgr
//
void
PM_Shutdown(void)
{
    if (!PMStarted)
        return;

    PML_ClosePageFile();

    PML_ShutdownMainMem();
}
