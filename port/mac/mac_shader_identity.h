#ifndef HALO_MAC_SHADER_IDENTITY_H
#define HALO_MAC_SHADER_IDENTITY_H

/* Issued only for immutable source-cache entries. Keep the matching result
   pinned and pass the same source pointers with this token. */
struct mac_shader_source_identity
{
    unsigned long cache_epoch;
    unsigned long vertex_entry;
    unsigned long pixel_entry;
};

#endif
