#ifndef GUEST_FEATURES_H
#define GUEST_FEATURES_H

#include_next "features.h"

#undef weak_alias
#define weak_alias(old, new) \
	extern __typeof(old) new; \
	static void *const __guest_alias_##new __attribute__((used)) = (void *)&old; \
	__asm__(".globl _" #new "\n.weak_definition _" #new "\n.set _" #new ", _" #old "\n")

#endif
