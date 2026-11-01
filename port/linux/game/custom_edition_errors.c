#include "custom_edition_errors.h"
#include <stdio.h>

char const *custom_edition_status_explanation(enum cache_file_status status)
{
    switch (status) {
    case _cache_file_status_ok: return "";
    case _cache_file_status_opensauce_cache:
        return "It requires OpenSauce features that GulchCE does not support.";
    case _cache_file_status_out_of_memory:
        return "There is not enough memory to load it. Close other applications and try again.";
    case _cache_file_status_tag_cache_overflow:
        return "Its map data exceeds the Custom Edition tag-cache limit. It needs a version built within that limit.";
    case _cache_file_status_file_too_large:
        return "Its file is larger than the Custom Edition loader supports.";
    case _cache_file_status_read_failed:
        return "Its data could not be read. Check that the complete map and its resources are accessible.";
    case _cache_file_status_unsupported_version:
        return "It is not a supported Halo Custom Edition map. Install a Custom Edition version in custom_maps.";
    case _cache_file_status_compressed_cache:
        return "This compressed map format is not supported. Install an uncompressed Custom Edition version.";
    case _cache_file_status_missing_resource_map:
        return "A required Custom Edition resource is missing or unusable. Check bitmaps.map, sounds.map and loc.map in custom_maps.";
    case _cache_file_status_missing_resource_item:
        return "Its resource files do not contain the data it needs. Install the matching Custom Edition resources.";
    case _cache_file_status_wrong_resource_map_type:
    case _cache_file_status_bad_resource_map_header:
    case _cache_file_status_bad_resource_item:
    case _cache_file_status_bad_resource_layout:
    case _cache_file_status_bad_resource_data_range:
        return "A required resource file is invalid or incompatible. Reinstall the matching Custom Edition resources.";
    case _cache_file_status_file_too_small:
    case _cache_file_status_bad_file_length:
        return "Its file is incomplete or has an invalid length. Reinstall the complete map.";
    default:
        return "Its map data is invalid or unsupported. Check for an updated map; debug.txt has the failing check.";
    }
}

void custom_edition_error_message(char const *map, char const *reason, char *out, size_t size)
{
    size_t i;
    if (!out || !size) return;
    snprintf(out, size, "Custom Edition map %.63s could not load. %.320s",
        map ? map : "(unknown)", reason ? reason : "See debug.txt for details.");
    /* Map and tag names are untrusted display text, never formatting or UI controls. */
    for (i = 0; out[i]; i++)
        if ((unsigned char)out[i] < 32 || (unsigned char)out[i] >= 127) out[i] = '?';
}
