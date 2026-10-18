/*
MAC_PLATFORM_SUPPRESS_CHECK.C

The companion to mac_platform_shim_check.c, testing the other half of what
port/mac/platform.h promises.

Internet play's sources say #include "platform.h" and sit in this
directory, beside the real one, so a quoted include finds the real one
whatever -I order the Mac build uses. The Mac shim is pre-included ahead
of them instead, and suppresses the real header by defining its include
guard. If that guard is ever renamed, the suppression silently stops and
the Mac build begins reading the XDK headers — which is the one thing
port/mac/platform.h exists to prevent.

This unit reproduces that arrangement: the shim first, then the real
header by the same name and from the same directory the p2p sources are
in. PLATFORM_CONTIGUOUS_BASE is defined in the body of the real header and
nowhere else, so its absence is what proves the body was skipped.

It defines nothing and links as an empty object.
*/

/* as the Mac build pre-includes it */
#include "../../mac/platform.h"

/* as p2p.c and its three companions reach it */
#include "platform.h"

#ifdef PLATFORM_CONTIGUOUS_BASE
#error "port/mac/platform.h no longer suppresses port/linux/src/platform.h. Its include guard was probably renamed; update the #define in port/mac/platform.h to match, or the Mac build will read the XDK headers."
#endif
