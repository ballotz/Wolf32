//
//	ID Engine
//	ID_IN.c - Input Manager
//	v1.0d1
//	By Jason Blochowiak
//

//
//	This module handles dealing with the various input devices
//
//	Depends on: Memory Mgr (for demo recording), Sound Mgr (for timing stuff),
//				User Mgr (for command line parms)
//
//	Globals:
//		LastScan - The keyboard scan code of the last key pressed
//		LastASCII - The ASCII value of the last key pressed
//	DEBUG - there are more globals
//

#include "ID_HEADS.H"
#pragma	hdrstop

extern int _argc;
extern char** _argv;

//
// joystick constants
//
#define	JoyScaleMax		32768
#define	JoyScaleShift	8
#define	MaxJoyValue		5000

/*
=============================================================================

                    GLOBAL VARIABLES

=============================================================================
*/

//
// configuration variables
//
boolean		MousePresent;
boolean		JoysPresent[MaxJoys];
boolean		JoyPadPresent;


// 	Global variables
boolean		Keyboard[NumCodes];
boolean		Paused;
char		LastASCII;
ScanCode	LastScan;

KeyboardDef	KbdDefs = { 0x1d,0x38,0x47,0x48,0x49,0x4b,0x4d,0x4f,0x50,0x51 };
JoystickDef	JoyDefs[MaxJoys];
ControlType	Controls[MaxPlayers];

longword	MouseDownCount;

Demo        DemoMode = demo_Off;
byte*       DemoBuffer;
word        DemoOffset, DemoSize;

/*
=============================================================================

                    LOCAL VARIABLES

=============================================================================
*/
static byte ASCIINames[] =		// Unshifted ASCII for scan codes
{
//	 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    0  ,27 ,'1','2','3','4','5','6','7','8','9','0','-','=',8  ,9  ,	// 0
    'q','w','e','r','t','y','u','i','o','p','[',']',13 ,0  ,'a','s',	// 1
    'd','f','g','h','j','k','l',';',39 ,'`',0  ,92 ,'z','x','c','v',	// 2
    'b','n','m',',','.','/',0  ,'*',0  ,' ',0  ,0  ,0  ,0  ,0  ,0  ,	// 3
    0  ,0  ,0  ,0  ,0  ,0  ,0  ,'7','8','9','-','4','5','6','+','1',	// 4
    '2','3','0',127,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,	// 5
    0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,	// 6
    0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0		// 7
},
ShiftNames[] =		// Shifted ASCII for scan codes
{
//	 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    0  ,27 ,'!','@','#','$','%','^','&','*','(',')','_','+',8  ,9  ,	// 0
    'Q','W','E','R','T','Y','U','I','O','P','{','}',13 ,0  ,'A','S',	// 1
    'D','F','G','H','J','K','L',':',34 ,'~',0  ,'|','Z','X','C','V',	// 2
    'B','N','M','<','>','?',0  ,'*',0  ,' ',0  ,0  ,0  ,0  ,0  ,0  ,	// 3
    0  ,0  ,0  ,0  ,0  ,0  ,0  ,'7','8','9','-','4','5','6','+','1',	// 4
    '2','3','0',127,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,	// 5
    0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,	// 6
    0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0   	// 7
},
SpecialNames[] =	// ASCII for 0xe0 prefixed codes
{
//	 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,	// 0
    0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,13 ,0  ,0  ,0  ,	// 1
    0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,	// 2
    0  ,0  ,0  ,0  ,0  ,'/',0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,	// 3
    0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,	// 4
    0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,	// 5
    0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,	// 6
    0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0   	// 7
};


static	boolean		IN_Started;
static	boolean		CapsLock;
static	ScanCode	CurCode, LastCode;

static	Direction	DirTable[] =		// Quick lookup for total direction
{
    dir_NorthWest,	dir_North,	dir_NorthEast,
    dir_West,		dir_None,	dir_East,
    dir_SouthWest,	dir_South,	dir_SouthEast
};

static	char* ParmStrings[] = { "nojoys","nomouse",nil };

//	Internal routines

///////////////////////////////////////////////////////////////////////////
//
//	INL_KeyService_ISR() - Handles a keyboard interrupt (key up/down)
//
///////////////////////////////////////////////////////////////////////////
void INL_KeyService_ISR(byte key)
{
    static	boolean	special;
    byte	c;

    if (key == 0xe0)		// Special key prefix
        special = true;
    else if (key == 0xe1)	// Handle Pause key
        Paused = true;
    else
    {
        if (key & 0x80)	// Break code
        {
            key &= 0x7f;

            // DEBUG - handle special keys: ctl-alt-delete, print scrn

            Keyboard[key] = false;
        }
        else			// Make code
        {
            LastCode = CurCode;
            CurCode = LastScan = key;
            Keyboard[key] = true;

            if (special)
                c = SpecialNames[key];
            else
            {
                if (key == sc_CapsLock)
                {
                    CapsLock ^= true;
                    // DEBUG - make caps lock light work
                }

                if (Keyboard[sc_LShift] || Keyboard[sc_RShift])	// If shifted
                {
                    c = ShiftNames[key];
                    if ((c >= 'A') && (c <= 'Z') && CapsLock)
                        c += 'a' - 'A';
                }
                else
                {
                    c = ASCIINames[key];
                    if ((c >= 'a') && (c <= 'z') && CapsLock)
                        c -= 'a' - 'A';
                }
            }
            if (c)
                LastASCII = c;
        }

        special = false;
    }
}

///////////////////////////////////////////////////////////////////////////
//
//	INL_GetJoyDelta() - Returns the relative movement of the specified
//		joystick (from +/-127)
//
///////////////////////////////////////////////////////////////////////////
void INL_GetJoyDelta(word joy, int16_t* dx, int16_t* dy)
{
    word x, y;
    JoystickDef* def;
    static longword lasttime;

    IN_GetJoyAbs(joy, &x, &y);
    def = JoyDefs + joy;

    if (x < def->threshMinX)
    {
        if (x < def->joyMinX)
            x = def->joyMinX;

        x = -(x - def->threshMinX);
        x *= def->joyMultXL;
        x >>= JoyScaleShift;
        *dx = (x > 127) ? -127 : -x;
    }
    else if (x > def->threshMaxX)
    {
        if (x > def->joyMaxX)
            x = def->joyMaxX;

        x = x - def->threshMaxX;
        x *= def->joyMultXH;
        x >>= JoyScaleShift;
        *dx = (x > 127) ? 127 : x;
    }
    else
        *dx = 0;

    if (y < def->threshMinY)
    {
        if (y < def->joyMinY)
            y = def->joyMinY;

        y = -(y - def->threshMinY);
        y *= def->joyMultYL;
        y >>= JoyScaleShift;
        *dy = (y > 127) ? -127 : -y;
    }
    else if (y > def->threshMaxY)
    {
        if (y > def->joyMaxY)
            y = def->joyMaxY;

        y = y - def->threshMaxY;
        y *= def->joyMultYH;
        y >>= JoyScaleShift;
        *dy = (y > 127) ? 127 : y;
    }
    else
        *dy = 0;

    lasttime = TimeCount_Get();
}

///////////////////////////////////////////////////////////////////////////
//
//	IN_GetJoyButtonsDB() - Returns the de-bounced button status of the
//		specified joystick
//
///////////////////////////////////////////////////////////////////////////
word
IN_GetJoyButtonsDB(word joy)
{
    longword	lasttime;
    word		result1, result2;

    do
    {
        result1 = INL_GetJoyButtons(joy);
        lasttime = TimeCount_Get();
        while (TimeCount_Get() == lasttime)
            ;
        result2 = INL_GetJoyButtons(joy);
    } while (result1 != result2);
    return(result1);
}

///////////////////////////////////////////////////////////////////////////
//
//	INL_StartKbd() - Sets up my keyboard stuff for use
//
///////////////////////////////////////////////////////////////////////////
static void
INL_StartKbd(void)
{
    IN_ClearKeysDown();
}

///////////////////////////////////////////////////////////////////////////
//
//	INL_ShutKbd() - Restores keyboard control to the BIOS
//
///////////////////////////////////////////////////////////////////////////
static void
INL_ShutKbd(void)
{
}

///////////////////////////////////////////////////////////////////////////
//
//	INL_StartMouse() - Detects and sets up the mouse
//
///////////////////////////////////////////////////////////////////////////
static boolean
INL_StartMouse(void)
{
    return Mouse_Detect();
}

///////////////////////////////////////////////////////////////////////////
//
//	INL_ShutMouse() - Cleans up after the mouse
//
///////////////////////////////////////////////////////////////////////////
static void
INL_ShutMouse(void)
{
}

//
//	INL_SetJoyScale() - Sets up scaling values for the specified joystick
//
static void
INL_SetJoyScale(word joy)
{
    JoystickDef* def;

    def = &JoyDefs[joy];
    def->joyMultXL = JoyScaleMax / (def->threshMinX - def->joyMinX);
    def->joyMultXH = JoyScaleMax / (def->joyMaxX - def->threshMaxX);
    def->joyMultYL = JoyScaleMax / (def->threshMinY - def->joyMinY);
    def->joyMultYH = JoyScaleMax / (def->joyMaxY - def->threshMaxY);
}

///////////////////////////////////////////////////////////////////////////
//
//	IN_SetupJoy() - Sets up thresholding values and calls INL_SetJoyScale()
//		to set up scaling values
//
///////////////////////////////////////////////////////////////////////////
void
IN_SetupJoy(word joy, word minx, word maxx, word miny, word maxy)
{
    word d, r;
    JoystickDef* def;

    def = &JoyDefs[joy];

    def->joyMinX = minx;
    def->joyMaxX = maxx;
    r = maxx - minx;
    d = r / 3;
    def->threshMinX = ((r / 2) - d) + minx;
    def->threshMaxX = ((r / 2) + d) + minx;

    def->joyMinY = miny;
    def->joyMaxY = maxy;
    r = maxy - miny;
    d = r / 3;
    def->threshMinY = ((r / 2) - d) + miny;
    def->threshMaxY = ((r / 2) + d) + miny;

    INL_SetJoyScale(joy);
}

///////////////////////////////////////////////////////////////////////////
//
//	INL_StartJoy() - Detects & auto-configures the specified joystick
//					The auto-config assumes the joystick is centered
//
///////////////////////////////////////////////////////////////////////////
static boolean
INL_StartJoy(word joy)
{
    word x, y;

    IN_GetJoyAbs(joy, &x, &y);

    if (
        ((x == 0) || (x > MaxJoyValue - 10))
        || ((y == 0) || (y > MaxJoyValue - 10))
    )
        return(false);
    else
    {
        IN_SetupJoy(joy, 0, x * 2, 0, y * 2);
        return(true);
    }
}

///////////////////////////////////////////////////////////////////////////
//
//	INL_ShutJoy() - Cleans up the joystick stuff
//
///////////////////////////////////////////////////////////////////////////
static void
INL_ShutJoy(word joy)
{
    JoysPresent[joy] = false;
}


///////////////////////////////////////////////////////////////////////////
//
//	IN_Startup() - Starts up the Input Mgr
//
///////////////////////////////////////////////////////////////////////////
void
IN_Startup(void)
{
    boolean	checkjoys, checkmouse;
    word	i;

    if (IN_Started)
        return;

    checkjoys = true;
    checkmouse = true;
    for (i = 1; i < _argc; i++)
    {
        switch (US_CheckParm(_argv[i], ParmStrings))
        {
        case 0:
            checkjoys = false;
            break;
        case 1:
            checkmouse = false;
            break;
        }
    }

    INL_StartKbd();
    MousePresent = checkmouse ? INL_StartMouse() : false;

    for (i = 0; i < MaxJoys; i++)
        JoysPresent[i] = checkjoys ? INL_StartJoy(i) : false;

    IN_Started = true;
}

///////////////////////////////////////////////////////////////////////////
//
//	IN_Default() - Sets up default conditions for the Input Mgr
//
///////////////////////////////////////////////////////////////////////////
void
IN_Default(boolean gotit, ControlType in)
{
    if (
        (!gotit)
        || ((in == ctrl_Joystick1) && !JoysPresent[0])
        || ((in == ctrl_Joystick2) && !JoysPresent[1])
        || ((in == ctrl_Mouse) && !MousePresent)
    )
        in = ctrl_Keyboard1;
    IN_SetControlType(0, in);
}

///////////////////////////////////////////////////////////////////////////
//
//	IN_Shutdown() - Shuts down the Input Mgr
//
///////////////////////////////////////////////////////////////////////////
void
IN_Shutdown(void)
{
    word i;

    if (!IN_Started)
        return;

    INL_ShutMouse();
    for (i = 0; i < MaxJoys; i++)
        INL_ShutJoy(i);
    INL_ShutKbd();

    IN_Started = false;
}

///////////////////////////////////////////////////////////////////////////
//
//	IN_ClearKeysDown() - Clears the keyboard array
//
///////////////////////////////////////////////////////////////////////////
void
IN_ClearKeysDown(void)
{
    LastScan = sc_None;
    LastASCII = key_None;
    memset(Keyboard, 0, sizeof(Keyboard));
}


///////////////////////////////////////////////////////////////////////////
//
//	IN_ReadControl() - Reads the device associated with the specified
//		player and fills in the control info struct
//
///////////////////////////////////////////////////////////////////////////
void
IN_ReadControl(int16_t player, ControlInfo* info)
{
    boolean		realdelta;
    byte		dbyte;
    word		buttons;
    int16_t		dx, dy;
    Motion		mx, my;
    ControlType	type;
    register	KeyboardDef* def;

    dx = dy = 0;
    mx = my = motion_None;
    buttons = 0;

    if (DemoMode == demo_Playback)
    {
        dbyte = DemoBuffer[DemoOffset + 1];
        my = (dbyte & 3) - 1;
        mx = ((dbyte >> 2) & 3) - 1;
        buttons = (dbyte >> 4) & 3;

        if (!(--DemoBuffer[DemoOffset]))
        {
            DemoOffset += 2;
            if (DemoOffset >= DemoSize)
                DemoMode = demo_PlayDone;
        }

        realdelta = false;
    }
    else if (DemoMode == demo_PlayDone)
        Quit("Demo playback exceeded");
    else
    {
        switch (type = Controls[player])
        {
        case ctrl_Keyboard:
            Keyboard_Update();

            def = &KbdDefs;

            if (Keyboard[def->upleft])
                mx = motion_Left, my = motion_Up;
            else if (Keyboard[def->upright])
                mx = motion_Right, my = motion_Up;
            else if (Keyboard[def->downleft])
                mx = motion_Left, my = motion_Down;
            else if (Keyboard[def->downright])
                mx = motion_Right, my = motion_Down;

            if (Keyboard[def->up])
                my = motion_Up;
            else if (Keyboard[def->down])
                my = motion_Down;

            if (Keyboard[def->left])
                mx = motion_Left;
            else if (Keyboard[def->right])
                mx = motion_Right;

            if (Keyboard[def->button0])
                buttons += 1 << 0;
            if (Keyboard[def->button1])
                buttons += 1 << 1;
            realdelta = false;
            break;
        case ctrl_Joystick1:
        case ctrl_Joystick2:
            INL_GetJoyDelta(type - ctrl_Joystick, &dx, &dy);
            buttons = INL_GetJoyButtons(type - ctrl_Joystick);
            realdelta = true;
            break;
        case ctrl_Mouse:
            Mouse_GetDelta(&dx, &dy);
            buttons = Mouse_GetButtons();
            realdelta = true;
            break;
        }
    }

    if (realdelta)
    {
        mx = (dx < 0) ? motion_Left : ((dx > 0) ? motion_Right : motion_None);
        my = (dy < 0) ? motion_Up : ((dy > 0) ? motion_Down : motion_None);
    }
    else
    {
        dx = mx * 127;
        dy = my * 127;
    }

    info->x = dx;
    info->xaxis = mx;
    info->y = dy;
    info->yaxis = my;
    info->button0 = buttons & (1 << 0);
    info->button1 = buttons & (1 << 1);
    info->button2 = buttons & (1 << 2);
    info->button3 = buttons & (1 << 3);
    info->dir = DirTable[((my + 1) * 3) + (mx + 1)];

    if (DemoMode == demo_Record)
    {
        // Pack the control info into a byte
        dbyte = (buttons << 4) | ((mx + 1) << 2) | (my + 1);

        if (
            (DemoBuffer[DemoOffset + 1] == dbyte)
            && (DemoBuffer[DemoOffset] < 255)
        )
            (DemoBuffer[DemoOffset])++;
        else
        {
            if (DemoOffset || DemoBuffer[DemoOffset])
                DemoOffset += 2;

            if (DemoOffset >= DemoSize)
                Quit("Demo buffer overflow");

            DemoBuffer[DemoOffset] = 1;
            DemoBuffer[DemoOffset + 1] = dbyte;
        }
    }
}

///////////////////////////////////////////////////////////////////////////
//
//	IN_SetControlType() - Sets the control type to be used by the specified
//		player
//
///////////////////////////////////////////////////////////////////////////
void
IN_SetControlType(int16_t player, ControlType type)
{
    // DEBUG - check that requested type is present?
    Controls[player] = type;
}

///////////////////////////////////////////////////////////////////////////
//
//	IN_WaitForKey() - Waits for a scan code, then clears LastScan and
//		returns the scan code
//
///////////////////////////////////////////////////////////////////////////
ScanCode
IN_WaitForKey(void)
{
    ScanCode result;

    while (!(result = LastScan))
        Keyboard_Update();
    LastScan = 0;
    return(result);
}

///////////////////////////////////////////////////////////////////////////
//
//	IN_WaitForASCII() - Waits for an ASCII char, then clears LastASCII and
//		returns the ASCII value
//
///////////////////////////////////////////////////////////////////////////
char
IN_WaitForASCII(void)
{
    char result;

    while (!(result = LastASCII))
        Keyboard_Update();
    LastASCII = '\0';
    return(result);
}

///////////////////////////////////////////////////////////////////////////
//
//	IN_Ack() - waits for a button or key press.  If a button is down, upon
// calling, it must be released for it to be recognized
//
///////////////////////////////////////////////////////////////////////////

boolean	btnstate[8];

void IN_StartAck(void)
{
    uint16_t i, buttons;

    //
    // get initial state of everything
    //
    IN_ClearKeysDown();
    memset(btnstate, 0, sizeof(btnstate));

    buttons = IN_JoyButtons() << 4;
    if (MousePresent)
        buttons |= IN_MouseButtons();

    for (i = 0; i < 8; i++, buttons >>= 1)
        if (buttons & 1)
            btnstate[i] = true;
}


boolean IN_CheckAck(void)
{
    uint16_t i, buttons;

    //
    // see if something has been pressed
    //
    Keyboard_Update();
    if (LastScan)
        return true;

    buttons = IN_JoyButtons() << 4;
    if (MousePresent)
        buttons |= IN_MouseButtons();

    for (i = 0; i < 8; i++, buttons >>= 1)
        if (buttons & 1)
        {
            if (!btnstate[i])
                return true;
        }
        else
            btnstate[i] = false;

    return false;
}


void IN_Ack(void)
{
    IN_StartAck();

    while (!IN_CheckAck())
        ;
}


///////////////////////////////////////////////////////////////////////////
//
//	IN_UserInput() - Waits for the specified delay time (in ticks) or the
//		user pressing a key or a mouse button. If the clear flag is set, it
//		then either clears the key or waits for the user to let the mouse
//		button up.
//
///////////////////////////////////////////////////////////////////////////
boolean IN_UserInput(longword delay)
{
    longword lasttime;

    lasttime = TimeCount_Get();
    IN_StartAck();
    do
    {
        if (IN_CheckAck())
            return true;
    } while (TimeCount_Get() - lasttime < delay);
    return(false);
}

//===========================================================================

/*
===================
=
= IN_MouseButtons
=
===================
*/

byte	IN_MouseButtons(void)
{
    if (MousePresent)
        return (byte)Mouse_GetButtons();
    else
        return 0;
}


/*
===================
=
= IN_JoyButtons
=
===================
*/

byte	IN_JoyButtons(void)
{
    return (byte)INL_GetJoyButtons(0);
}


