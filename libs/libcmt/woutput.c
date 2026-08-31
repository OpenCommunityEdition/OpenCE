/*
WOUTPUT.C

The wide-character printf formatting engine.  As in the original CRT, this is a
thin wrapper that compiles OUTPUT.C with the wide-character configuration.
*/

#define _UNICODE 1
#define UNICODE 1

#include "output.c"
