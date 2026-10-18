#ifndef MAC_BONE_GATHER_H
#define MAC_BONE_GATHER_H

#include <stddef.h>

/* A zero-version plan is disabled. Nonzero versions are returned only after
 * the source recognizer proves the exact bounded two-address pattern. */
struct mac_bone_gather_plan {
    unsigned version;
    unsigned address_attribute;
    unsigned address_constant;
    unsigned first_gather_attribute;
    unsigned gather_attribute_count;
    unsigned row_base;
    unsigned row_limit;
};

/* Return 1 when a supported source pattern was rewritten, 0 when it should
 * use the ordinary shader path, or -1 for allocation/argument failure.
 * On return 1, *rewritten_source is caller-owned. */
int mac_bone_gather_rewrite(const char *modern_source,
    struct mac_bone_gather_plan *plan, char **rewritten_source,
    char *error, size_t error_size);

/* Apply a recognized plan to native float4 vertex records. The input array
 * remains unchanged if validation fails. Each record is exactly 16 float4s.
 * Records and constants must be separate owned arrays. */
int mac_bone_gather_apply(const struct mac_bone_gather_plan *plan,
    float *records, size_t vertex_count, size_t stride_vec4,
    const float constants[192][4], char *error, size_t error_size);

/* The same gathered rows (attributes 10..15, 24 floats per vertex) from each
 * vertex's decoded address attribute alone (x, y: 2 floats per vertex). On
 * failure the output is incomplete and must not be drawn. */
int mac_bone_gather_rows(const struct mac_bone_gather_plan *plan,const float *addresses,
    size_t vertex_count,const float constants[192][4],float *rows_out,
    char *error,size_t error_size);

#endif
