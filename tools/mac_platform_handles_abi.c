/* Compile the native declarations alongside authoritative clean-XDK types. */
#include "engine_xdk_prefix.h"
#include "engine_sockets.h"
#include "engine_handles.h"
typedef char mac_socket_native_width[sizeof(SOCKET)==sizeof(unsigned int)?1:-1];
typedef char mac_handle_native_width[sizeof(HANDLE)==4?1:-1];
