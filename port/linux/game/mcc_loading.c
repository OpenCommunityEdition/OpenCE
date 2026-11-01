/* MCC conversion runs synchronously on the render thread. Present only the
 * standalone loading effect: normal frames can dereference unfinished tags. */
#include "cseries.h"
#include "errors.h"
#include "mcc_cache.h"
#include "mcc_loading.h"
#include "interface/progress_bar.h"
#include <xtl.h>
#include "rasterizer/xbox/rasterizer_xbox.h"
#include <string.h>

int config_boolean(char const *name);

static struct {
    int active, drawing, stage;
    DWORD last_frame;
    real progress;
} mcc_loading;

/* Stage weights describe completed work, not an estimate of seconds left. */
static real const mcc_loading_limits[] = {0.001f, 0.12f, 0.16f, 0.22f,
    0.30f, 0.55f, 0.88f, 0.96f, 0.99f};

void mcc_loading_pulse(void)
{
    DWORD now;
    D3DVIEWPORT8 viewport;
    IDirect3DSurface8 *surface;
    D3DSURFACE_DESC description;
    if (!mcc_loading.active || mcc_loading.drawing) return;
    now = GetTickCount();
    if ((DWORD)(now - mcc_loading.last_frame) < 50) return;
    mcc_loading.last_frame = now;
    mcc_loading.drawing = 1;
    /* No frame/window setup, widgets, console commands or network callbacks.
     * Present pumps platform window events through the existing device API. */
    surface = NULL;
    if (IDirect3DDevice8_GetBackBuffer(global_d3d_device, 0, 0, &surface) < 0 || !surface) {
        mcc_loading.drawing = 0;
        return;
    }
    if (IDirect3DSurface8_GetDesc(surface, &description) < 0) {
        IDirect3DSurface8_Release(surface);
        mcc_loading.drawing = 0;
        return;
    }
    IDirect3DSurface8_Release(surface);
    memset(&viewport, 0, sizeof(viewport));
    viewport.Width = description.Width;
    viewport.Height = description.Height;
    viewport.MaxZ = 1.f;
    IDirect3DDevice8_SetViewport(global_d3d_device, &viewport);
    IDirect3DDevice8_BeginScene(global_d3d_device);
    progress_bar_display(mcc_loading.progress);
    IDirect3DDevice8_EndScene(global_d3d_device);
    IDirect3DDevice8_Present(global_d3d_device, NULL, NULL, NULL, NULL);
    /* The next game window installs its own viewport. The loading renderer
     * already invalidates the rasterizer's cached device states. */
    mcc_loading.last_frame = GetTickCount();
    mcc_loading.drawing = 0;
}

int mcc_loading_begin(char const *name)
{
    if (mcc_loading.active || !mcc_level_name(name) || !global_d3d_device ||
        config_boolean("debug.null_renderer") || progress_bar_is_active()) return 0;
    memset(&mcc_loading, 0, sizeof(mcc_loading));
    mcc_loading.active = 1;
    mcc_loading.stage = -1;
    mcc_loading.progress = mcc_loading_limits[0];
    mcc_loading.last_frame = GetTickCount() - 50;
    progress_bar_eachframe();
    progress_bar_begin(FALSE);
    mcc_loading_stage(MCC_LOADING_RESOURCES);
    return 1;
}

void mcc_loading_stage(enum mcc_loading_stage stage)
{
    static char const *const names[] = {"checking resources", "converting tags",
        "converting scripts", "converting geometry", "converting audio",
        "converting textures", "validating tags", "preparing scenario"};
    if (!mcc_loading.active || stage < 0 || stage >= MCC_LOADING_STAGES ||
        (int)stage <= mcc_loading.stage) return;
    mcc_loading.stage = stage;
    mcc_loading.progress = mcc_loading_limits[stage];
    error(_error_silent, "mcc loading: %s", names[stage]);
    mcc_loading_pulse();
}

void mcc_loading_update(enum mcc_loading_stage stage, unsigned long done, unsigned long total)
{
    real progress;
    if (!mcc_loading.active || stage < 0 || stage >= MCC_LOADING_STAGES ||
        (int)stage != mcc_loading.stage || !total || done > total) return;
    progress = mcc_loading_limits[stage] + (mcc_loading_limits[stage + 1] -
        mcc_loading_limits[stage]) * ((real)done / (real)total);
    if (progress > mcc_loading.progress) mcc_loading.progress = progress;
    mcc_loading_pulse();
}

void mcc_loading_end(int success)
{
    if (!mcc_loading.active) return;
    if (success) {
        mcc_loading.progress = 1.f;
        mcc_loading.last_frame = GetTickCount() - 50;
        mcc_loading_pulse();
    }
    progress_bar_end();
    progress_bar_eachframe();
    memset(&mcc_loading, 0, sizeof(mcc_loading));
}
