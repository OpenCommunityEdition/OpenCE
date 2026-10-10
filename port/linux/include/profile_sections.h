/*
PROFILE_SECTIONS.H

The profiling build's CPU sections around the game's steps (configure.py
--profile, port/linux/src/profile_trace.c): in a normal build each wraps
its statement and leaves nothing else.
*/

#ifndef __PROFILE_SECTIONS_H
#define __PROFILE_SECTIONS_H

#ifdef HALO_PROFILE

#include "cseries/profile.h"

/* these macros leave no tokens in a normal build: call sites have no trailing
semicolon. a call site may be an unbraced if-branch: in a normal build
profile_scope is the statement alone. */
#define PROFILE_SECTION(variable, name) static struct profile_section variable = { name, NONE, TRUE };
#define profile_scope_enter(variable) profile_enter(variable)
#define profile_scope_exit(variable) profile_exit(variable)
#define profile_scope(variable, statement) { profile_enter(variable) statement profile_exit(variable) }

#else

#define PROFILE_SECTION(variable, name)
#define profile_scope_enter(variable)
#define profile_scope_exit(variable)
#define profile_scope(variable, statement) statement

#endif

#endif
