#include "xgpu.h"
#include "mcc_environment_shader.h"

static int mcc_environment_bump;

void mcc_environment_shader_set(int enabled) { mcc_environment_bump=enabled!=0; }
int mcc_environment_shader_active(void) { return mcc_environment_bump; }

void mcc_environment_shader_emit(struct xgpu_text *text)
{
    /* The lightmap already contains the flat surface's diffuse response.
     * Apply the normal's change from that response, preserving a flat bump
     * at one even for grazing/back-facing incident vectors. This bounded
     * reconstruction follows the documented intent of MCC's alternate
     * flag; its exact proprietary shader equation is not used or known. */
    xgpu_text_append(text,
        "\t\tvec3 cAB = vec3(max(0.0, 1.0 + dot(cA, cB) - cB.z));\n");
}
