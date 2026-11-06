/*
HOST_HTTPS.C

The server browser's requests to the master servers (host_https_get,
port/android/guest/runtime/guest_host.h; port/linux/src/master_list.c), made
in the host with the desktop's HTTPS client (port/linux/src/posix_update.c's
update_https_request, Mbed TLS, the system's certificate authorities): the
guest has no TLS of its own, and its memory is the host's to read and write.
*/

#include "host.h"
#include "../../linux/src/update.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* master_list.c's struct https_get: the guest's addresses as 32-bit values
(x86-64 would pass a 7th pointer argument on the stack) */
struct host_https_get
{
	uint32_t url;
	int32_t status;
	uint32_t headers;
	int32_t headers_size;
	uint32_t body;
	int32_t body_capacity;
	int32_t body_size;
	uint32_t error;
	int32_t error_size;
};

typedef char host_https_get_size_check[sizeof(struct host_https_get) == 36 ? 1 : -1];

int host_https_get(struct host_https_get *request)
{
	struct update_response response;
	int body_capacity = request->body_capacity;

	request->status = 0;
	request->body_size = 0;
	if (!update_https_request("GET", (const char *)(uintptr_t)request->url, NULL, NULL, 0,
		(unsigned long)body_capacity, &response, (char *)(uintptr_t)request->error, request->error_size))
		return 0;
	request->status = response.status;
	snprintf((char *)(uintptr_t)request->headers, (size_t)request->headers_size, "%s", response.headers);
	if (response.body_size > (unsigned long)body_capacity)
		response.body_size = (unsigned long)body_capacity;
	memcpy((void *)(uintptr_t)request->body, response.body, response.body_size);
	request->body_size = (int32_t)response.body_size;
	free(response.body);
	return 1;
}
