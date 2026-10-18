/*
MAC_PLATFORM_SHIM_CHECK.C

port/mac/platform.h mirrors two declarations and two constants out of this
directory, so that internet play's sources can be built for the Mac
without reading the XDK headers. A mirror is only useful while it still
says what the original says.

This unit reads the originals and then the mirror, and will not compile if
they have drifted apart:

  - the prototypes, because C requires two declarations of one function in
    the same scope to be compatible;
  - the two Winsock numbers, by capturing the SDK's and asserting the
    mirror's equal them. The mirror defines them only when they are not
    already defined, so they are undefined here first — otherwise the
    comparison would be of the SDK's values with themselves.

The Mac's own compiler never sees this file. That is the point: it runs on
every Linux build and in CI, which the Mac build cannot reach.

What it does not catch is a name *added* to the real platform.h that the
mirror lacks; nothing is mirrored until internet play uses it, and the Mac
build then fails plainly. mac_platform_suppress_check.c covers the other
half, that the mirror still suppresses the real header.

It defines nothing and links as an empty object.
*/

/* the originals */
#include "platform.h"
#include "xdk_winsock.h"

/* the SDK's numbers, kept where the mirror cannot overwrite them */
enum
{
	mac_shim_sdk_wouldblock = WSAEWOULDBLOCK,
	mac_shim_sdk_inprogress = WSAEINPROGRESS
};

#undef WSAEWOULDBLOCK
#undef WSAEINPROGRESS

/* the mirror, which now defines its own */
#include "../../mac/platform.h"

/* a negative array size is the error; the names say which number moved */
typedef char mac_shim_wouldblock_matches_the_sdk[
	WSAEWOULDBLOCK == mac_shim_sdk_wouldblock ? 1 : -1];
typedef char mac_shim_inprogress_matches_the_sdk[
	WSAEINPROGRESS == mac_shim_sdk_inprogress ? 1 : -1];
