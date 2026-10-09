/*
HOST_VK_DRIVER.H

Opens the Vulkan driver the renderer runs on: the phone's own, or one the player left in the data folder
as an adrenotools archive, loaded with libadrenotools.
*/

#ifndef HOST_VK_DRIVER_H
#define HOST_VK_DRIVER_H

#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif
#include <vulkan/vulkan.h>

#include <stddef.h>

/* Opens the driver named by setting (display.vk_driver): empty is the phone's
own; otherwise the name of an archive in the app's external files folder. It
returns that driver's vkGetInstanceProcAddr, or NULL when not even the phone's
driver could be opened (after logging why). description says which driver was
asked for and, if it fell back, why, in one line, for the log.

What falls back here, and what does not. An archive that cannot be used - no
such file, a broken zip, no meta.json, a library that is not an ELF file, a
loader libadrenotools refuses - is logged and the phone's own driver is opened
instead. But once libadrenotools has accepted the library, the driver itself
is loaded only by vkCreateInstance, through its hook, and two failures show up
only then and cannot be undone in this process:

- the hook cannot load the library and quietly loads the phone's driver: the
  instance works, on the wrong driver; host_vk_driver_verify() says so;
- the library loads but is no Vulkan driver: the instance has no physical
  device.

So the caller makes the instance, then calls host_vk_driver_verify(), and
treats a failed check or no physical device as "Vulkan on this driver is not
available": the backend starts the GL ES image instead, and says why in the log.

Only one driver is open in the process: a second call returns the first call's
result, and a call after host_vk_driver_close() returns NULL. Nothing in the
host may load the phone's Vulkan driver once a custom one is open (no
SDL_Vulkan_* calls: SDL would dlopen the system loader). Not thread safe; call
it from the thread that sets Vulkan up. */
PFN_vkGetInstanceProcAddr host_vk_driver_open(const char *setting, char *description, size_t size);

/* After the instance is made (the system loader loads a driver only then): whether the driver asked for is the one
loaded, in text. libadrenotools hands back the system loader whatever it is given, and its hook falls back to the phone's
own driver without a word when the library will not load, so a custom driver's library must be looked for in the
process: returns 0 and says so if it is not there. True for the phone's own driver. */
int host_vk_driver_verify(char *text, size_t size);

/* closes the driver, once, when the process is done with Vulkan: every Vulkan
object made through it must be destroyed first, and nothing may call into it
afterwards. The driver cannot be opened again in the same process (libadrenotools'
hooks stay installed), so host_vk_driver_open() returns NULL from then on. */
void host_vk_driver_close(void);

#endif
