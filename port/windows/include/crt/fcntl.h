/* Windows <fcntl.h>, then the game's file-name wrappers
(halo_windows_file_names.h) */

#include_next <fcntl.h>

/* the platform layer opens its files close-on-exec on Linux; the Windows
equivalent is not letting child processes inherit them */
#ifndef O_ACCMODE
#define O_ACCMODE (_O_RDONLY | _O_WRONLY | _O_RDWR)
#endif
#ifndef O_CLOEXEC
#define O_CLOEXEC _O_NOINHERIT
#endif

#include "halo_windows_file_names.h"
