#ifndef HALO_MAC_MAPPED_FILE_H
#define HALO_MAC_MAPPED_FILE_H
#include <stddef.h>
struct mac_mapped_file { const unsigned char *data;size_t size; };
/* Read-only, private, demand-paged view of an existing nonempty regular file.
   No full-file heap copy and no retained descriptor. Mapping still consumes
   virtual address space; touched pages can consume RAM. The file must not be
   truncated while mapped. Output must be empty; failure leaves it empty. */
int mac_mapped_file_open(const char *,size_t maximum_bytes,struct mac_mapped_file *,const char **);
const void *mac_mapped_file_span(const struct mac_mapped_file *,size_t offset,size_t length);
/* Invalidates all borrowed spans. Safe for an empty or already closed view. */
void mac_mapped_file_close(struct mac_mapped_file *);
#endif
