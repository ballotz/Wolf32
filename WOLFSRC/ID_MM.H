// ID_MM.H

#ifndef __ID_CA__

#define __ID_CA__


#define	BUFFERSIZE		0x1000		// miscelanious, allways available buffer

#define MAXBLOCKS		700


//==========================================================================

typedef void* memptr;

typedef struct
{
    int32_t	mainmem;
} mminfotype;

//==========================================================================

extern	mminfotype	mminfo;
extern	memptr		bufferseg;
extern	boolean		mmerror;

extern	void		(*beforesort) (void);
extern	void		(*aftersort) (void);

//==========================================================================

void MM_Startup(void);
void MM_Shutdown(void);

void MM_GetPtr(memptr* baseptr, size_t size);
void MM_FreePtr(memptr* baseptr);

void MM_SetPurge(memptr* baseptr, int32_t purge);
void MM_SetLock(memptr* baseptr, boolean locked);
void MM_SortMem(void);

int32_t MM_UnusedMemory(void);
int32_t MM_TotalFree(void);

void MM_BombOnError(boolean bomb);

#endif