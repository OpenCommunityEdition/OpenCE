/* Native file boundary for the unchanged shared settings implementation. */
#include "engine_config.h"
#include "port_config.h"
#include <mach-o/dyld.h>
#include <limits.h>
#include <stdint.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
static char base_path[PATH_MAX];
static pthread_once_t path_once = PTHREAD_ONCE_INIT;
/* Read the shared setting on every query: config_boolean already owns its
   parsed settings, including environment overrides and menu updates. */
/* Interpolation draws the state between two ticks, which needs more frames
   than ticks: while the frames take as long as a tick (30 Hz) or longer it
   shows nothing the ticks do not, and costs about 1.5% of a G4. So it stops
   by itself while the frame time averages 32 ms or more, and starts again
   under 25 ms (HALO_MAC_AUTO_INTERPOLATION=0 keeps display.interpolation's
   answer); the loop reports each frame's time. */
static double interpolation_frame_ms;
static unsigned interpolation_frames;
static int interpolation_slow;
static int interpolation_auto = -1;
void mac_interpolation_note_frame(double seconds)
{
    double ms = seconds * 1000.0;
    if (ms < 0.0) return;
    if (ms > 250.0) ms = 250.0;
    if (interpolation_frames < 1000) ++interpolation_frames;
    interpolation_frame_ms = interpolation_frames == 1 ? ms :
        interpolation_frame_ms * 0.9 + ms * 0.1;
    if (interpolation_frames >= 10) {
        if (interpolation_frame_ms >= 32.0) interpolation_slow = 1;
        else if (interpolation_frame_ms <= 25.0) interpolation_slow = 0;
    }
}
int halo_interpolation_enabled(void)
{
    if (!config_boolean("display.interpolation")) return 0;
    if (interpolation_auto < 0) {
        const char *value = getenv("HALO_MAC_AUTO_INTERPOLATION");
        interpolation_auto = !(value && value[0] == '0' && !value[1]);
    }
    return !(interpolation_auto && interpolation_slow);
}
static void find_base_path(void)
{
    char executable[PATH_MAX], resolved[PATH_MAX], *separator;
    uint32_t size = sizeof(executable);
    /* the app (port/mac/app) keeps config.toml in Application Support, not in
       the bundle: HALO_MAC_CONFIG_DIR names that folder */
    const char *folder = getenv("HALO_MAC_CONFIG_DIR");
    if (folder && *folder && strlen(folder) + 2 <= sizeof(base_path)) {
        size_t length = strlen(folder);
        memcpy(base_path, folder, length);
        if (base_path[length - 1] != '/') base_path[length++] = '/';
        base_path[length] = 0;
        return;
    }
    if (_NSGetExecutablePath(executable, &size) != 0) return;
    if (!realpath(executable, resolved)) return;
    separator = strrchr(resolved, '/');
    if (!separator) return;
    separator[1] = 0;
    memcpy(base_path, resolved, strlen(resolved) + 1);
}
const char *mac_config_base_path(void)
{
    pthread_once(&path_once, find_base_path);
    return *base_path ? base_path : NULL;
}
void *mac_config_load_file(const char *path, size_t *size)
{
    FILE *file = fopen(path, "rb");
    long length;
    char *data = NULL;
    if (!file) return NULL;
    if (fseek(file, 0, SEEK_END) == 0 && (length = ftell(file)) >= 0 &&
        fseek(file, 0, SEEK_SET) == 0) {
        data = malloc((size_t)length + 1);
        if (data && fread(data, 1, (size_t)length, file) == (size_t)length) {
            data[length] = 0;
            *size = (size_t)length;
        } else { free(data); data = NULL; }
    }
    if (fclose(file) != 0) { free(data); data = NULL; }
    return data;
}
int mac_config_save_file(const char *path, const void *data, size_t size)
{
    FILE *file;
    int written;
    /* port: nothing is written into an app bundle (the app names a folder in Application Support
       for its config with HALO_MAC_CONFIG_DIR) */
    if (path && strstr(path, ".app/")) {
        fprintf(stderr, "settings: %s is inside an app and is not written (set HALO_MAC_CONFIG_DIR)\n", path);
        return 0;
    }
    file = fopen(path, "wb");
    if (!file) return 0;
    written = fwrite(data, 1, size, file) == size;
    return fclose(file) == 0 && written;
}
void mac_config_log(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    fputs("halo-mac settings: ", stderr);
    vfprintf(stderr, format, arguments);
    fputc('\n', stderr);
    va_end(arguments);
}
