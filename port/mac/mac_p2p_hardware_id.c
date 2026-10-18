/*
MAC_P2P_HARDWARE_ID.C

What this machine is known by, for p2p_hardware_id (port/linux/src/p2p.c).

Each platform answers this differently: Windows reads the SMBIOS system
UUID or the registry's MachineGuid (port/windows/src/win32_p2p.c), Linux
reads /etc/machine-id, Android reads the ANDROID_ID its Java put in
hardware_id.txt. Mac OS X has neither of the file paths, so before this it
had no answer at all and every Mac reported no hardware id.

IOPlatformUUID, from the I/O registry's IOPlatformExpertDevice, is the
Mac's equivalent: one value for the machine, the same across reboots and
reinstalls, present on 10.4 and later on both PowerPC and Intel. The
serial number is the fallback for a machine whose UUID is absent, which
some early models are.

What is returned here is never sent anywhere. p2p.c hashes it with a key
of its own, so what leaves the machine is a digest and not a serial
number. It is a stable name, not a proof: anyone with root can change what
the machine answers.

This file needs -framework IOKit, which the session build already links
for the pads (port/mac/mac_gamepad.c).
*/

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>

/* (declared where p2p.c calls it, as win32_p2p.c's is) */
int posix_hardware_id_source(char *text, int size);

/* the first of the entry's properties that gives a non-empty string */
static int copy_registry_string(io_service_t service, const char *key,
	char *text, int size)
{
	CFStringRef name;
	CFTypeRef value;
	int copied = 0;

	name = CFStringCreateWithCString(kCFAllocatorDefault, key, kCFStringEncodingUTF8);
	if (!name)
		return 0;
	value = IORegistryEntryCreateCFProperty(service, name, kCFAllocatorDefault, 0);
	CFRelease(name);
	if (value)
	{
		if (CFGetTypeID(value) == CFStringGetTypeID() &&
			CFStringGetCString((CFStringRef)value, text, (CFIndex)size, kCFStringEncodingUTF8) &&
			text[0])
		{
			copied = 1;
		}
		CFRelease(value);
	}
	return copied;
}

int posix_hardware_id_source(char *text, int size)
{
	/* (the key names, rather than kIOPlatformUUIDKey, which the 10.4
	headers do not all declare) */
	static const char *const keys[] = { "IOPlatformUUID", "IOPlatformSerialNumber" };
	io_service_t service;
	CFMutableDictionaryRef matching;
	int index;
	int found = 0;

	if (!text || size <= 0)
		return 0;
	text[0] = 0;
	matching = IOServiceMatching("IOPlatformExpertDevice");
	if (!matching)
		return 0;
	/* (IOServiceGetMatchingService consumes the dictionary either way) */
	service = IOServiceGetMatchingService(kIOMasterPortDefault, matching);
	if (!service)
		return 0;
	for (index = 0; !found && index < (int)(sizeof(keys) / sizeof(*keys)); index++)
		found = copy_registry_string(service, keys[index], text, size);
	IOObjectRelease(service);
	if (!found)
		text[0] = 0;
	return found;
}
