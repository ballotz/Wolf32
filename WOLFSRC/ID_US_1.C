//
//	ID Engine
//	ID_US_1.c - User Manager - General routines
//	v1.1d1
//	By Jason Blochowiak
//	Hacked up for Catacomb 3D
//

//
//	This module handles dealing with user input & feedback
//
//	Depends on: Input Mgr, View Mgr, some variables from the Sound, Caching,
//		and Refresh Mgrs, Memory Mgr for background save/restore
//
//	Globals:
//		ingame - Flag set by game indicating if a game is in progress
//      abortgame - Flag set if the current game should be aborted (if a load
//			game fails)
//		loadedgame - Flag set if a game was loaded
//		abortprogram - Normally nil, this points to a terminal error message
//			if the program needs to abort
//		restartgame - Normally set to gd_Continue, this is set to one of the
//			difficulty levels if a new game should be started
//		PrintX, PrintY - Where the User Mgr will print (global coords)
//		WindowX,WindowY,WindowW,WindowH - The dimensions of the current
//			window
//

#include "ID_HEADS.H"

#pragma	hdrstop

extern int _argc;
extern char** _argv;

//	Global variables
char*       abortprogram;
boolean		NoWait;
word		PrintX, PrintY;
word		WindowX, WindowY, WindowW, WindowH;

//	Internal variables
#define	ConfigVersion	1

static	char    *ParmStrings[] = { "TEDLEVEL","NOWAIT",nil },
                *ParmStrings2[] = { "COMP","NOCOMP",nil };
static	boolean US_Started;

boolean         Button0, Button1,
                CursorBad;
int16_t         CursorX, CursorY;

void        (*USL_MeasureString)(char*, word*, word*) = VW_MeasurePropString,
            (*USL_DrawString)(char*) = VWB_DrawPropString;

SaveGame	Games[MaxSaveGames];
HighScore	Scores[MaxScores] =
{
    {"id software-'92",10000,1},
    {"Adrian Carmack",10000,1},
    {"John Carmack",10000,1},
    {"Kevin Cloud",10000,1},
    {"Tom Hall",10000,1},
    {"John Romero",10000,1},
    {"Jay Wilbur",10000,1},
};

//	Internal routines

unsigned char rndtable[] =
{
    0x00, 0x08, 0x6D, 0xDC, 0xDE, 0xF1, 0x95, 0x6B, 0x4B, 0xF8, 0xFE, 0x8C, 0x10, 0x42,
    0x4A, 0x15, 0xD3, 0x2F, 0x50, 0xF2, 0x9A, 0x1B, 0xCD, 0x80, 0xA1, 0x59, 0x4D, 0x24,
    0x5F, 0x6E, 0x55, 0x30, 0xD4, 0x8C, 0xD3, 0xF9, 0x16, 0x4F, 0xC8, 0x32, 0x1C, 0xBC,
    0x34, 0x8C, 0xCA, 0x78, 0x44, 0x91, 0x3E, 0x46, 0xB8, 0xBE, 0x5B, 0xC5, 0x98, 0xE0,
    0x95, 0x68, 0x19, 0xB2, 0xFC, 0xB6, 0xCA, 0xB6, 0x8D, 0xC5, 0x04, 0x51, 0xB5, 0xF2,
    0x91, 0x2A, 0x27, 0xE3, 0x9C, 0xC6, 0xE1, 0xC1, 0xDB, 0x5D, 0x7A, 0xAF, 0xF9, 0x00,
    0xAF, 0x8F, 0x46, 0xEF, 0x2E, 0xF6, 0xA3, 0x35, 0xA3, 0x6D, 0xA8, 0x87, 0x02, 0xEB,
    0x19, 0x5C, 0x14, 0x91, 0x8A, 0x4D, 0x45, 0xA6, 0x4E, 0xB0, 0xAD, 0xD4, 0xA6, 0x71,
    0x5E, 0xA1, 0x29, 0x32, 0xEF, 0x31, 0x6F, 0xA4, 0x46, 0x3C, 0x02, 0x25, 0xAB, 0x4B,
    0x88, 0x9C, 0x0B, 0x38, 0x2A, 0x92, 0x8A, 0xE5, 0x49, 0x92, 0x4D, 0x3D, 0x62, 0xC4,
    0x87, 0x6A, 0x3F, 0xC5, 0xC3, 0x56, 0x60, 0xCB, 0x71, 0x65, 0xAA, 0xF7, 0xB5, 0x71,
    0x50, 0xFA, 0x6C, 0x07, 0xFF, 0xED, 0x81, 0xE2, 0x4F, 0x6B, 0x70, 0xA6, 0x67, 0xF1,
    0x18, 0xDF, 0xEF, 0x78, 0xC6, 0x3A, 0x3C, 0x52, 0x80, 0x03, 0xB8, 0x42, 0x8F, 0xE0,
    0x91, 0xE0, 0x51, 0xCE, 0xA3, 0x2D, 0x3F, 0x5A, 0xA8, 0x72, 0x3B, 0x21, 0x9F, 0x5F,
    0x1C, 0x8B, 0x7B, 0x62, 0x7D, 0xC4, 0x0F, 0x46, 0xC2, 0xFD, 0x36, 0x0E, 0x6D, 0xE2,
    0x47, 0x11, 0xA1, 0x5D, 0xBA, 0x57, 0xF4, 0x8A, 0x14, 0x34, 0x7B, 0xFB, 0x1A, 0x24,
    0x11, 0x2E, 0x34, 0xE7, 0xE8, 0x4C, 0x1F, 0xDD, 0x54, 0x25, 0xD8, 0xA5, 0xD4, 0x6A,
    0xC5, 0xF2, 0x62, 0x2B, 0x27, 0xAF, 0xFE, 0x91, 0xBE, 0x54, 0x76, 0xDE, 0xBB, 0x88,
    0x78, 0xA3, 0xEC, 0xF9
};

byte rndindex = 0;

void US_InitRndT(boolean randomize)
{
    if (!randomize)
    {
        rndindex = 0;
    }
    else
    {
        rndindex = rand() & 0xFF;
    }
}

int16_t US_RndT()
{
    rndindex = (rndindex + 1) & 0xFF;
    return rndtable[rndindex];
}

//	Public routines

///////////////////////////////////////////////////////////////////////////
//
//	US_Startup() - Starts the User Mgr
//
///////////////////////////////////////////////////////////////////////////
void
US_Startup(void)
{
    int16_t	i, n;

    if (US_Started)
        return;

    US_InitRndT(true);		// Initialize the random number generator

    for (i = 1; i < _argc; i++)
    {
        switch (US_CheckParm(_argv[i], ParmStrings2))
        {
        case 0:
            compatability = true;
            break;
        case 1:
            compatability = false;
            break;
        }
    }

    // Check for TED launching here
    for (i = 1; i < _argc; i++)
    {
        n = US_CheckParm(_argv[i], ParmStrings);
        switch (n)
        {
        case 0:
            tedlevelnum = atoi(_argv[i + 1]);
            if (tedlevelnum >= 0)
                tedlevel = true;
            break;

        case 1:
            NoWait = true;
            break;
        }
    }

    US_Started = true;
}


///////////////////////////////////////////////////////////////////////////
//
//	US_Shutdown() - Shuts down the User Mgr
//
///////////////////////////////////////////////////////////////////////////
void
US_Shutdown(void)
{
    if (!US_Started)
        return;

    US_Started = false;
}

///////////////////////////////////////////////////////////////////////////
//
//	US_CheckParm() - checks to see if a string matches one of a set of
//		strings. The check is case insensitive. The routine returns the
//		index of the string that matched, or -1 if no matches were found
//
///////////////////////////////////////////////////////////////////////////
int16_t
US_CheckParm(char* parm, char** strings)
{
    char        cp, cs,
                *p, *s;
    int16_t     i;

    while (!isalpha(*parm))	// Skip non-alphas
        parm++;

    for (i = 0; *strings && **strings; i++)
    {
        for (s = *strings++, p = parm, cs = cp = 0; cs == cp;)
        {
            cs = *s++;
            if (!cs)
                return(i);
            cp = *p++;

            if (isupper(cs))
                cs = tolower(cs);
            if (isupper(cp))
                cp = tolower(cp);
        }
    }
    return(-1);
}


//	Window/Printing routines

///////////////////////////////////////////////////////////////////////////
//
//	US_SetPrintRoutines() - Sets the routines used to measure and print
//		from within the User Mgr. Primarily provided to allow switching
//		between masked and non-masked fonts
//
///////////////////////////////////////////////////////////////////////////
void
US_SetPrintRoutines(void (*measure)(char*, word*, word*), void (*print)(char*))
{
    USL_MeasureString = measure;
    USL_DrawString = print;
}

///////////////////////////////////////////////////////////////////////////
//
//	US_Print() - Prints a string in the current window. Newlines are
//		supported.
//
///////////////////////////////////////////////////////////////////////////
void
US_Print(char* s)
{
    char	buffer[64];
    char	c, *se;
    word	w, h;

    // copy string to prevent inplace modification of argument
    // that could be in a read only memory
    strncpy(buffer, s, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';
    s = buffer;

    while (*s)
    {
        se = s;
        while ((c = *se) && (c != '\n'))
            se++;
        *se = '\0';

        USL_MeasureString(s, &w, &h);
        px = PrintX;
        py = PrintY;
        USL_DrawString(s);

        s = se;
        if (c)
        {
            *se = c;
            s++;

            PrintX = WindowX;
            PrintY += h;
        }
        else
            PrintX += w;
    }
}

///////////////////////////////////////////////////////////////////////////
//
//	US_PrintUnsigned() - Prints an unsigned long
//
///////////////////////////////////////////////////////////////////////////
void
US_PrintUnsigned(longword n)
{
    char	buffer[32];

    snprintf(buffer, sizeof(buffer), "%u", n);
    US_Print(buffer);
}

///////////////////////////////////////////////////////////////////////////
//
//	US_PrintSigned() - Prints a signed long
//
///////////////////////////////////////////////////////////////////////////
void
US_PrintSigned(int32_t n)
{
    char	buffer[32];

    snprintf(buffer, sizeof(buffer), "%i", n);
    US_Print(buffer);
}

///////////////////////////////////////////////////////////////////////////
//
//	USL_PrintInCenter() - Prints a string in the center of the given rect
//
///////////////////////////////////////////////////////////////////////////
void
USL_PrintInCenter(char* s, Rect r)
{
    word    w, h,
            rw, rh;

    USL_MeasureString(s, &w, &h);
    rw = r.lr.x - r.ul.x;
    rh = r.lr.y - r.ul.y;

    px = r.ul.x + ((rw - w) / 2);
    py = r.ul.y + ((rh - h) / 2);
    USL_DrawString(s);
}

///////////////////////////////////////////////////////////////////////////
//
//	US_PrintCentered() - Prints a string centered in the current window.
//
///////////////////////////////////////////////////////////////////////////
void
US_PrintCentered(char* s)
{
    Rect	r;

    r.ul.x = WindowX;
    r.ul.y = WindowY;
    r.lr.x = r.ul.x + WindowW;
    r.lr.y = r.ul.y + WindowH;

    USL_PrintInCenter(s, r);
}

///////////////////////////////////////////////////////////////////////////
//
//	US_CPrintLine() - Prints a string centered on the current line and
//		advances to the next line. Newlines are not supported.
//
///////////////////////////////////////////////////////////////////////////
void
US_CPrintLine(char* s)
{
    word	w, h;

    USL_MeasureString(s, &w, &h);

    if (w > WindowW)
        Quit("US_CPrintLine() - String exceeds width");
    px = WindowX + ((WindowW - w) / 2);
    py = PrintY;
    USL_DrawString(s);
    PrintY += h;
}

///////////////////////////////////////////////////////////////////////////
//
//	US_CPrint() - Prints a string in the current window. Newlines are
//		supported.
//
///////////////////////////////////////////////////////////////////////////
void
US_CPrint(char* s)
{
    char	buffer[64];
    char	c, *se;

    // copy string to prevent inplace modification of argument
    // that could be in a read only memory
    strncpy(buffer, s, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';
    s = buffer;

    while (*s)
    {
        se = s;
        while ((c = *se) && (c != '\n'))
            se++;
        *se = '\0';

        US_CPrintLine(s);

        s = se;
        if (c)
        {
            *se = c;
            s++;
        }
    }
}

///////////////////////////////////////////////////////////////////////////
//
//	US_ClearWindow() - Clears the current window to white and homes the
//		cursor
//
///////////////////////////////////////////////////////////////////////////
void
US_ClearWindow(void)
{
    VWB_Bar(WindowX, WindowY, WindowW, WindowH, WHITE);
    PrintX = WindowX;
    PrintY = WindowY;
}

///////////////////////////////////////////////////////////////////////////
//
//	US_DrawWindow() - Draws a frame and sets the current window parms
//
///////////////////////////////////////////////////////////////////////////
void
US_DrawWindow(word x, word y, word w, word h)
{
    word	i,
        sx, sy, sw, sh;

    WindowX = x * 8;
    WindowY = y * 8;
    WindowW = w * 8;
    WindowH = h * 8;

    PrintX = WindowX;
    PrintY = WindowY;

    sx = (x - 1) * 8;
    sy = (y - 1) * 8;
    sw = (w + 1) * 8;
    sh = (h + 1) * 8;

    US_ClearWindow();

    VWB_DrawTile8(sx, sy, 0), VWB_DrawTile8(sx, sy + sh, 5);
    for (i = sx + 8; i <= sx + sw - 8; i += 8)
        VWB_DrawTile8(i, sy, 1), VWB_DrawTile8(i, sy + sh, 6);
    VWB_DrawTile8(i, sy, 2), VWB_DrawTile8(i, sy + sh, 7);

    for (i = sy + 8; i <= sy + sh - 8; i += 8)
        VWB_DrawTile8(sx, i, 3), VWB_DrawTile8(sx + sw, i, 4);
}

///////////////////////////////////////////////////////////////////////////
//
//	US_CenterWindow() - Generates a window of a given width & height in the
//		middle of the screen
//
///////////////////////////////////////////////////////////////////////////
void
US_CenterWindow(word w, word h)
{
    US_DrawWindow(((MaxX / 8) - w) / 2, ((MaxY / 8) - h) / 2, w, h);
}

///////////////////////////////////////////////////////////////////////////
//
//	US_SaveWindow() - Saves the current window parms into a record for
//		later restoration
//
///////////////////////////////////////////////////////////////////////////
void
US_SaveWindow(WindowRec* win)
{
    win->x = WindowX;
    win->y = WindowY;
    win->w = WindowW;
    win->h = WindowH;

    win->px = PrintX;
    win->py = PrintY;
}

///////////////////////////////////////////////////////////////////////////
//
//	US_RestoreWindow() - Sets the current window parms to those held in the
//		record
//
///////////////////////////////////////////////////////////////////////////
void
US_RestoreWindow(WindowRec* win)
{
    WindowX = win->x;
    WindowY = win->y;
    WindowW = win->w;
    WindowH = win->h;

    PrintX = win->px;
    PrintY = win->py;
}

//	Input routines

///////////////////////////////////////////////////////////////////////////
//
//	USL_XORICursor() - XORs the I-bar text cursor. Used by US_LineInput()
//
///////////////////////////////////////////////////////////////////////////
static void
USL_XORICursor(int16_t x, int16_t y, char* s, word cursor)
{
    static  boolean status;		// VGA doesn't XOR...
    char    buf[MaxString];
    byte    temp;
    word    w, h;

    strcpy(buf, s);
    buf[cursor] = '\0';
    USL_MeasureString(buf, &w, &h);

    px = x + w - 1;
    py = y;
    if (status ^= 1)
        USL_DrawString("\x80");
    else
    {
        temp = fontcolor;
        fontcolor = backcolor;
        USL_DrawString("\x80");
        fontcolor = temp;
    }

}

///////////////////////////////////////////////////////////////////////////
//
//	US_LineInput() - Gets a line of user input at (x,y), the string defaults
//		to whatever is pointed at by def. Input is restricted to maxchars
//		chars or maxwidth pixels wide. If the user hits escape (and escok is
//		true), nothing is copied into buf, and false is returned. If the
//		user hits return, the current string is copied into buf, and true is
//		returned
//
///////////////////////////////////////////////////////////////////////////
boolean
US_LineInput(int16_t x, int16_t y, char* buf, char* def, boolean escok,
    int16_t maxchars, int16_t maxwidth)
{
    boolean     redraw,
                cursorvis, cursormoved,
                done, result;
    ScanCode    sc;
    char        c,
                s[MaxString], olds[MaxString];
    word        i,
                cursor,
                w, h,
                len;
    byte        temp;
    longword    lasttime;

    if (def)
        strcpy(s, def);
    else
        *s = '\0';
    *olds = '\0';
    cursor = (word)strlen(s);
    cursormoved = redraw = true;

    cursorvis = done = false;
    lasttime = TimeCount_Get();
    LastASCII = key_None;
    LastScan = sc_None;

    while (!done)
    {
        if (cursorvis)
            USL_XORICursor(x, y, s, cursor);

        Keyboard_Update();

        sc = LastScan;
        LastScan = sc_None;
        c = LastASCII;
        LastASCII = key_None;

        switch (sc)
        {
        case sc_LeftArrow:
            if (cursor)
                cursor--;
            c = key_None;
            cursormoved = true;
            break;
        case sc_RightArrow:
            if (s[cursor])
                cursor++;
            c = key_None;
            cursormoved = true;
            break;
        case sc_Home:
            cursor = 0;
            c = key_None;
            cursormoved = true;
            break;
        case sc_End:
            cursor = (word)strlen(s);
            c = key_None;
            cursormoved = true;
            break;

        case sc_Return:
            strcpy(buf, s);
            done = true;
            result = true;
            c = key_None;
            break;
        case sc_Escape:
            if (escok)
            {
                done = true;
                result = false;
            }
            c = key_None;
            break;

        case sc_BackSpace:
            if (cursor)
            {
                strcpy(s + cursor - 1, s + cursor);
                cursor--;
                redraw = true;
            }
            c = key_None;
            cursormoved = true;
            break;
        case sc_Delete:
            if (s[cursor])
            {
                strcpy(s + cursor, s + cursor + 1);
                redraw = true;
            }
            c = key_None;
            cursormoved = true;
            break;

        case 0x4c:	// Keypad 5
        case sc_UpArrow:
        case sc_DownArrow:
        case sc_PgUp:
        case sc_PgDn:
        case sc_Insert:
            c = key_None;
            break;
        }

        if (c)
        {
            len = (word)strlen(s);
            USL_MeasureString(s, &w, &h);

            if
                (
                    isprint(c)
                    && (len < MaxString - 1)
                    && ((!maxchars) || (len < maxchars))
                    && ((!maxwidth) || (w < maxwidth))
                    )
            {
                for (i = len + 1; i > cursor; i--)
                    s[i] = s[i - 1];
                s[cursor++] = c;
                redraw = true;
            }
        }

        if (redraw)
        {
            px = x;
            py = y;
            temp = fontcolor;
            fontcolor = backcolor;
            USL_DrawString(olds);
            fontcolor = temp;
            strcpy(olds, s);

            px = x;
            py = y;
            USL_DrawString(s);

            redraw = false;
        }

        if (cursormoved)
        {
            cursorvis = false;
            lasttime = TimeCount_Get() - TickBase;

            cursormoved = false;
        }
        if (TimeCount_Get() - lasttime > TickBase / 2)
        {
            lasttime = TimeCount_Get();

            cursorvis ^= true;
        }
        if (cursorvis)
            USL_XORICursor(x, y, s, cursor);

        VW_UpdateScreen();
    }

    if (cursorvis)
        USL_XORICursor(x, y, s, cursor);
    if (!result)
    {
        px = x;
        py = y;
        USL_DrawString(olds);
    }
    VW_UpdateScreen();

    IN_ClearKeysDown();
    return(result);
}
