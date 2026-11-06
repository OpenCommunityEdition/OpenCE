/*
musl's internal features.h, with weak_alias spelled as assembler
directives for a guest compiled for a Darwin target (HALO_GUEST_MACHO),
which rejects alias attributes, and whose assembly is converted to ELF
afterwards (tools/android_asm_convert.py), where the alias is ordinary. A
guest that clang compiles to ELF itself (x32) keeps musl's own.
*/

#ifndef GUEST_FEATURES_H
#define GUEST_FEATURES_H

#include_next "features.h"

#ifdef HALO_GUEST_MACHO
#undef weak_alias
#define weak_alias(old, new) \
	extern __typeof(old) new; \
	static void *const __guest_alias_##new __attribute__((used)) = (void *)&old; \
	__asm__(".globl _" #new "\n.weak_definition _" #new "\n.set _" #new ", _" #old "\n")
#endif

#endif
