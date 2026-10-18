/* Actual engine header layouts, plus a correctly typed aggregate-return check.
 * The latter models the scripting ABI; it is not the scripting implementation. */
#include "tools/mac_compile_probe_prefix.h"
#include <wchar.h>
/* Only after libc headers: do not rewrite their inline declarations. */
#define __inline static __inline__
#include "cseries.h"
#include "real_math.h"
#include "tag_files/tag_groups.h"

union probe_value { long bits; float real; };
extern union probe_value mac_probe_return(long value);
struct wide_alignment { char byte; double value; };

int main(void)
{
    union probe_value result;
    real_vector3d vector;
    int ok;
    vector.i = 1; vector.j = 2; vector.k = 3;
    result = mac_probe_return(0x12345678);
    ok = sizeof(void *) == 4 && sizeof(long) == 4 && sizeof(wchar_t) == 2 &&
        sizeof(real_vector3d) == 12 && sizeof(real_matrix4x3) == 52 &&
        sizeof(struct tag_block) == 12 && sizeof(struct tag_data) == 20 &&
        sizeof(struct tag_reference) == 16 &&
        offsetof(struct tag_block, address) == 4 &&
        offsetof(struct tag_data, address) == 12 &&
        vector.n[0] == 1 && vector.n[1] == 2 && vector.n[2] == 3 &&
        result.bits == 0x12345678;
    printf("engine_layouts=%s pointer=%lu long=%lu wchar=%lu vector=%lu matrix=%lu block=%lu data=%lu reference=%lu double_offset=%lu\n",
        ok ? "pass" : "FAIL", (unsigned long)sizeof(void *),
        (unsigned long)sizeof(long), (unsigned long)sizeof(wchar_t),
        (unsigned long)sizeof(real_vector3d), (unsigned long)sizeof(real_matrix4x3),
        (unsigned long)sizeof(struct tag_block), (unsigned long)sizeof(struct tag_data),
        (unsigned long)sizeof(struct tag_reference),
        (unsigned long)offsetof(struct wide_alignment, value));
    printf("typed_union_return=%08lx\n", (unsigned long)result.bits);
    return ok ? 0 : 1;
}
