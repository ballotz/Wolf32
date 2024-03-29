//
//	ID_PM.H
//	Header file for Id Engine's Page Manager
//

//	NOTE! PMPageSize must be >= 1024
#define	PMPageSize      4096
#define	PMPageSizeSeg   (PMPageSize >> 4)
#define	PMPageSizeKB    (PMPageSize >> 10)

#define	PMMinMainMem    10      // Min acceptable # of pages from main
#define	PMMaxMainMem    100     // Max number of pages in main memory

#define	PMThrashThreshold   1   // Number of page thrashes before panic mode
#define	PMUnThrashThreshold 5   // Number of non-thrashing frames before leaving panic mode

typedef	enum
{
    pml_Unlocked,
    pml_Locked
} PMLockType;

typedef	enum
{
    pmba_Unused = 0,
    pmba_Used = 1,
    pmba_Allocated = 2
} PMBlockAttr;

typedef	struct
{
    longword    offset;     // Offset of chunk into file
    word        length;     // Length of the chunk

    PMLockType  locked;     // If set, this page can't be purged
    int16_t     mainPage;   // If in Main, index into handle array

    longword    lastHit;    // Last frame number of hit
} PageListStruct;

extern  word            ChunksInFile, PMSpriteStart, PMSoundStart;
extern  PageListStruct* PMPages;

#define	PM_GetSoundPage(v)	PM_GetPage(PMSoundStart + (v))
#define	PM_GetSpritePage(v)	PM_GetPage(PMSpriteStart + (v))

#define	PM_LockMainMem()	PM_SetMainMemPurge(0)
#define	PM_UnlockMainMem()	PM_SetMainMemPurge(3)


extern	char	PageFileName[13];


extern	void	PM_Startup(void);
extern	void	PM_Shutdown(void);
extern	void	PM_Reset(void);
extern	void	PM_Preload(boolean(*update)(word current, word total));
extern	void	PM_NextFrame(void);
extern	void	PM_SetPageLock(int16_t pagenum, PMLockType lock);
extern	void	PM_CheckMainMem(void);
extern	memptr	PM_GetPageAddress(int16_t pagenum);
extern	memptr	PM_GetPage(int16_t pagenum);		// Use this one to cache page

void PM_SetMainMemPurge(int16_t level);
