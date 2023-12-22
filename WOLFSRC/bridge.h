#pragma once
#include <stdint.h>

extern uint32_t TimeCount;

// update request by application
// calls added to update app state that won't change without DOS ISR

// request keyboard update trough INL_KeyService_ISR()
void Update_Key();

// request time update (TimeCount)
void Update_Time();

// poll (called by application)

uint8_t IN_HasMouse();

///////////////////////////////////////////////////////////////////////////
//
//	INL_GetMouseDelta() - Gets the amount that the mouse has moved from the
//		mouse driver
//
///////////////////////////////////////////////////////////////////////////
void INL_GetMouseDelta(int16_t* x, int16_t* y);

///////////////////////////////////////////////////////////////////////////
//
//	INL_GetMouseButtons() - Gets the status of the mouse buttons from the
//		mouse driver
//
///////////////////////////////////////////////////////////////////////////
uint16_t INL_GetMouseButtons(void);

///////////////////////////////////////////////////////////////////////////
//
//	IN_GetJoyAbs() - Reads the absolute position of the specified joystick
//
///////////////////////////////////////////////////////////////////////////
void IN_GetJoyAbs(uint16_t joy, uint16_t* xp, uint16_t* yp);

///////////////////////////////////////////////////////////////////////////
//
//	INL_GetJoyButtons() - Returns the button status of the specified
//		joystick
//
///////////////////////////////////////////////////////////////////////////
uint16_t INL_GetJoyButtons(uint16_t joy);

// events (implemented by application)

void INL_KeyService_ISR(uint8_t key);

