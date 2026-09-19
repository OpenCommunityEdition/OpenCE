#ifndef BINK_RADPLATFORM_H
#define BINK_RADPLATFORM_H

/* This support layer uses Xbox kernel/XAPI services, not graphics or sound.
 * These are the XDK's documented include switches, not compiler controls.
 * Graphics/audio translation units must include their own SDK owners.
 */
#ifndef NOD3D
#define NOD3D
#endif
#ifndef NODSOUND
#define NODSOUND
#endif
#include <xtl.h>

#endif
