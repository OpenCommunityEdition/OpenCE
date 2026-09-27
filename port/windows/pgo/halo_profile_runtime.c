/*
HALO_PROFILE_RUNTIME.C

The start-up of LLVM's profile runtime (compiler-rt's lib/profile), for the
instrumented build that records the Windows build's profile
(tools/windows_build.py, configure.py --pgo=train). LLVM writes it in C++,
as a static initializer; here it is a C function in the C runtime's
initializer table. It reads LLVM_PROFILE_FILE and has the profile written
when the game exits.
*/

#include "InstrProfiling.h"

/* instrumented code refers to this, so that the runtime gets linked */
COMPILER_RT_VISIBILITY int INSTR_PROF_PROFILE_RUNTIME_VAR;

static void __cdecl register_runtime(void)
{
	__llvm_profile_initialize();
}

#pragma section(".CRT$XCU", read)
__declspec(allocate(".CRT$XCU")) __attribute__((used)) static void (__cdecl *const halo_profile_runtime)(void) =
	register_runtime;
