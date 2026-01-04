/*
CSERIES_WINDOWS.H
*/

#ifndef __CSERIES_WINDOWS_H
#define __CSERIES_WINDOWS_H
#pragma once

/* ---------- includes */

#define DEBUG_KEYBOARD
#include <xtl.h>
#include <xbdm.h>

/* ---------- prototypes/CSERIES_WINDOWS.C */

unsigned long system_milliseconds(void);

long generic_exception_filter(unsigned long exception_code, PEXCEPTION_POINTERS exception_information);

#endif // __CSERIES_WINDOWS_H
