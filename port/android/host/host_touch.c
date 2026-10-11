/* UI thread writes one snapshot; the guest reads it on the input thread. */
#include <jni.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "../../linux/include/halo_porting_ui.h"

static pthread_mutex_t touch_lock = PTHREAD_MUTEX_INITIALIZER;
static int32_t touch_state[7]; /* SDL axes followed by SDL button bits */
static float look_delta[2];
static int camera_pending;
static struct timespec look_time;
static uint32_t cheat_pending, cheat_busy;
static int32_t cheat_commands[16], cheat_status[16];
static int rumble_amplitude;
static struct timespec rumble_time;
static int ui_menus = 1, ui_context, ui_action, ui_value, ui_revision;
static float ui_point[6]; /* normalized x/y, click, back, scroll, scrollbar */
static struct halo_porting_menu porting_menu;
static int frame_count, frame_fps;
static struct timespec frame_time;
static float field_of_view = 70.0f;
/* Edit in-game UI: x and y of the weapons' panel, the health panel and the
   motion sensor, as fractions of the screen. */
static float hud_offsets[6];
/* The installed app's version, for the main menu's build label. */
static char app_version[HALO_APP_VERSION_SIZE];

JNIEXPORT void JNICALL Java_com_halo_decomp_TouchControls_nativeFieldOfView(
    JNIEnv *env, jclass cls, jfloat degrees)
{
    (void)env; (void)cls;
    if (!(degrees >= 55.0f && degrees <= 90.0f)) return;
    pthread_mutex_lock(&touch_lock);
    field_of_view = degrees;
    pthread_mutex_unlock(&touch_lock);
}

JNIEXPORT void JNICALL Java_com_halo_decomp_TouchControls_nativeHudLayout(
    JNIEnv *env, jclass cls, jfloatArray values)
{
    jfloat next[6];
    int i;
    (void)cls;
    if (!values || (*env)->GetArrayLength(env, values) != 6) return;
    (*env)->GetFloatArrayRegion(env, values, 0, 6, next);
    for (i = 0; i < 6; ++i)
        if (!(next[i] >= -1.0f && next[i] <= 1.0f)) return;
    pthread_mutex_lock(&touch_lock);
    memcpy(hud_offsets, next, sizeof(hud_offsets));
    pthread_mutex_unlock(&touch_lock);
}

JNIEXPORT void JNICALL Java_com_halo_decomp_TouchControls_nativeAppVersion(
    JNIEnv *env, jclass cls, jstring version)
{
    const char *text;
    (void)cls;
    if (!version) return;
    text = (*env)->GetStringUTFChars(env, version, NULL);
    if (!text) return;
    pthread_mutex_lock(&touch_lock);
    strncpy(app_version, text, sizeof(app_version)-1);
    app_version[sizeof(app_version)-1] = 0;
    pthread_mutex_unlock(&touch_lock);
    (*env)->ReleaseStringUTFChars(env, version, text);
}

void host_touch_app_version(char *buffer, unsigned int size)
{
    if (!size) return;
    pthread_mutex_lock(&touch_lock);
    strncpy(buffer, app_version, size-1);
    buffer[size-1] = 0;
    pthread_mutex_unlock(&touch_lock);
}

void host_touch_hud_read(float *offsets)
{
    pthread_mutex_lock(&touch_lock);
    memcpy(offsets, hud_offsets, sizeof(hud_offsets));
    pthread_mutex_unlock(&touch_lock);
}

float host_touch_field_of_view(void)
{
    float degrees;
    pthread_mutex_lock(&touch_lock);
    degrees = field_of_view;
    pthread_mutex_unlock(&touch_lock);
    return degrees;
}

void host_touch_ui_context(int menus, int context)
{
    pthread_mutex_lock(&touch_lock);
    if (ui_menus != menus) {
        camera_pending = 0;
        memset(touch_state, 0, sizeof(touch_state));
        memset(look_delta, 0, sizeof(look_delta));
        memset(ui_point, 0, sizeof(ui_point));
    }
    ui_menus = menus; if (context >= 0) ui_context = context;
    pthread_mutex_unlock(&touch_lock);
}

void host_touch_pointer_read(float *point)
{
    pthread_mutex_lock(&touch_lock);
    memcpy(point, ui_point, sizeof(ui_point));
    ui_point[2] = ui_point[3] = ui_point[4] = ui_point[5] = 0;
    pthread_mutex_unlock(&touch_lock);
}

void host_porting_menu_read(struct halo_porting_menu *menu)
{
    pthread_mutex_lock(&touch_lock);
    memcpy(menu, &porting_menu, sizeof(*menu));
    pthread_mutex_unlock(&touch_lock);
}

void host_porting_action(int revision, int action, int value)
{
    pthread_mutex_lock(&touch_lock);
    int slider = action >= 40 && action <= 43;
    int valid_slider = 0;
    if (slider && porting_menu.page) {
        for (int i = 0; i < porting_menu.count; ++i)
            if (porting_menu.rows[i].action == action && porting_menu.rows[i].value >= 0)
                valid_slider = 1;
    }
    /* A slider owns its gesture across value acknowledgements. Keep the
       newest position, while still checking it belongs to the current page. */
    if ((!ui_action || (valid_slider && ui_action == action)) &&
        (revision == porting_menu.revision || valid_slider)) {
        ui_action = action; ui_value = value; ui_revision = porting_menu.revision;
    }
    pthread_mutex_unlock(&touch_lock);
}

void host_touch_frame(void)
{
    struct timespec now;
    double elapsed;
    clock_gettime(CLOCK_MONOTONIC, &now);
    pthread_mutex_lock(&touch_lock);
    if (!frame_time.tv_sec) frame_time = now;
    ++frame_count;
    elapsed = now.tv_sec-frame_time.tv_sec + (now.tv_nsec-frame_time.tv_nsec)/1e9;
    if (elapsed >= 0.5) {
        frame_fps = (int)(frame_count/elapsed + 0.5);
        frame_count = 0; frame_time = now;
    }
    pthread_mutex_unlock(&touch_lock);
}

JNIEXPORT jintArray JNICALL Java_com_halo_decomp_TouchControls_nativeMenuPoll(JNIEnv *env, jclass cls)
{
    jint values[6]; jintArray result;
    (void)cls;
    pthread_mutex_lock(&touch_lock);
    values[0] = ui_menus; values[1] = ui_context; values[2] = ui_action;
    values[3] = ui_value; values[4] = ui_revision; values[5] = frame_fps;
    ui_action = 0;
    pthread_mutex_unlock(&touch_lock);
    result = (*env)->NewIntArray(env, 6);
    if (result) (*env)->SetIntArrayRegion(env, result, 0, 6, values);
    return result;
}

JNIEXPORT void JNICALL Java_com_halo_decomp_TouchControls_nativeMenuPointer(
    JNIEnv *env, jclass cls, jfloat x, jfloat y, jboolean click, jboolean back,
    jfloat scroll, jboolean scrollbar)
{
    (void)env; (void)cls;
    pthread_mutex_lock(&touch_lock);
    if (ui_menus) {
        ui_point[0] = x; ui_point[1] = y;
        if (click) ui_point[2] = 1;
        if (back) ui_point[3] = 1;
        ui_point[4] += scroll;
        if (scrollbar) ui_point[5] = 1;
    }
    pthread_mutex_unlock(&touch_lock);
}

JNIEXPORT void JNICALL Java_com_halo_decomp_TouchControls_nativeMenuPublish(
    JNIEnv *env, jclass cls, jint revision, jint page, jboolean editing,
    jstring title, jobjectArray labels, jintArray actions, jintArray values)
{
    struct halo_porting_menu next = {0};
    const char *text; int i; jint ids[HALO_PORTING_ROWS], progress[HALO_PORTING_ROWS];
    (void)cls;
    next.revision = revision; next.page = page; next.editing = editing;
    next.count = (*env)->GetArrayLength(env, labels);
    if (next.count > HALO_PORTING_ROWS || (*env)->GetArrayLength(env, actions) != next.count ||
        (*env)->GetArrayLength(env, values) != next.count) return;
    text = (*env)->GetStringUTFChars(env, title, NULL);
    if (!text) return;
    strncpy(next.title, text, sizeof(next.title)-1);
    (*env)->ReleaseStringUTFChars(env, title, text);
    (*env)->GetIntArrayRegion(env, actions, 0, next.count, ids);
    (*env)->GetIntArrayRegion(env, values, 0, next.count, progress);
    for (i = 0; i < next.count; ++i) {
        jstring label = (jstring)(*env)->GetObjectArrayElement(env, labels, i);
        text = (*env)->GetStringUTFChars(env, label, NULL);
        if (!text) return;
        strncpy(next.rows[i].text, text, sizeof(next.rows[i].text)-1);
        (*env)->ReleaseStringUTFChars(env, label, text);
        (*env)->DeleteLocalRef(env, label);
        next.rows[i].action = ids[i]; next.rows[i].value = progress[i];
    }
    pthread_mutex_lock(&touch_lock);
    porting_menu = next;
    pthread_mutex_unlock(&touch_lock);
}

void host_touch_rumble(unsigned int low, unsigned int high)
{
    unsigned int strength = low > high ? low : high;
    pthread_mutex_lock(&touch_lock);
    rumble_amplitude = strength ? 64 + (strength * 191u / 65535u) : 0;
    clock_gettime(CLOCK_MONOTONIC, &rumble_time);
    pthread_mutex_unlock(&touch_lock);
}

JNIEXPORT jint JNICALL Java_com_halo_decomp_TouchControls_nativeRumble(JNIEnv *env, jclass cls)
{
    struct timespec now;
    int amplitude;
    (void)env; (void)cls;
    clock_gettime(CLOCK_MONOTONIC, &now);
    pthread_mutex_lock(&touch_lock);
    amplitude = rumble_amplitude;
    if ((now.tv_sec-rumble_time.tv_sec)*1000000000LL + now.tv_nsec-rumble_time.tv_nsec > 150000000LL)
        amplitude = 0;
    pthread_mutex_unlock(&touch_lock);
    return amplitude;
}

JNIEXPORT void JNICALL Java_com_halo_decomp_TouchControls_nativeLook(
    JNIEnv *env, jclass cls, jfloat dx, jfloat dy)
{
    (void)env; (void)cls;
    pthread_mutex_lock(&touch_lock);
    if (!ui_menus) { look_delta[0] += dx; look_delta[1] += dy; }
    clock_gettime(CLOCK_MONOTONIC, &look_time);
    pthread_mutex_unlock(&touch_lock);
}

JNIEXPORT void JNICALL Java_com_halo_decomp_TouchControls_nativeLookReset(JNIEnv *env, jclass cls)
{
    (void)env; (void)cls;
    pthread_mutex_lock(&touch_lock);
    look_delta[0] = look_delta[1] = 0;
    pthread_mutex_unlock(&touch_lock);
}

JNIEXPORT void JNICALL Java_com_halo_decomp_TouchControls_nativeCameraMode(JNIEnv *env, jclass cls)
{
    (void)env; (void)cls;
    pthread_mutex_lock(&touch_lock);
    if (!ui_menus) camera_pending = 1;
    pthread_mutex_unlock(&touch_lock);
}

int host_touch_camera_read(void)
{
    int pending;
    pthread_mutex_lock(&touch_lock);
    pending = camera_pending; camera_pending = 0;
    pthread_mutex_unlock(&touch_lock);
    return pending;
}

void host_touch_look_read(float *delta)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    pthread_mutex_lock(&touch_lock);
    memcpy(delta, look_delta, sizeof(look_delta));
    /* Discard movement left over from menus, cutscenes or a suspended app. */
    if ((now.tv_sec-look_time.tv_sec)*1000000000LL + now.tv_nsec-look_time.tv_nsec > 150000000LL)
        delta[0] = delta[1] = 0;
    look_delta[0] = look_delta[1] = 0;
    pthread_mutex_unlock(&touch_lock);
}

JNIEXPORT jboolean JNICALL Java_com_halo_decomp_TouchControls_nativeCheatRequest(
    JNIEnv *env, jclass cls, jint id, jboolean enabled)
{
    (void)env; (void)cls;
    if (id < 0 || id >= 16) return JNI_FALSE;
    pthread_mutex_lock(&touch_lock);
    if (cheat_busy & (1u << id)) { pthread_mutex_unlock(&touch_lock); return JNI_FALSE; }
    cheat_commands[id] = enabled != 0;
    cheat_pending |= 1u << id;
    cheat_busy |= 1u << id;
    pthread_mutex_unlock(&touch_lock);
    return JNI_TRUE;
}

JNIEXPORT jint JNICALL Java_com_halo_decomp_TouchControls_nativeCheatStatus(
    JNIEnv *env, jclass cls, jint id)
{
    int status;
    (void)env; (void)cls;
    if (id < 0 || id >= 16) return -1;
    pthread_mutex_lock(&touch_lock);
    status = (cheat_busy & (1u << id)) ? -2 : cheat_status[id];
    pthread_mutex_unlock(&touch_lock);
    return status;
}

unsigned int host_touch_cheats_read(int *commands)
{
    unsigned int pending;
    pthread_mutex_lock(&touch_lock);
    pending = cheat_pending; cheat_pending = 0;
    memcpy(commands, cheat_commands, sizeof(cheat_commands));
    pthread_mutex_unlock(&touch_lock);
    return pending;
}

void host_touch_cheat_result(int id, int status)
{
    pthread_mutex_lock(&touch_lock);
    cheat_status[id] = status;
    cheat_busy &= ~(1u << id);
    pthread_mutex_unlock(&touch_lock);
}

void host_touch_cheat_sync(int id, int active)
{
    pthread_mutex_lock(&touch_lock);
    if (!(cheat_busy & (1u << id)) && cheat_status[id] >= 0) cheat_status[id] = active;
    pthread_mutex_unlock(&touch_lock);
}

JNIEXPORT void JNICALL Java_com_halo_decomp_TouchControls_nativeState(
	JNIEnv *env, jclass cls, jint lx, jint ly, jint rx, jint ry,
	jint lt, jint rt, jint buttons)
{
	int32_t next[] = { lx, ly, rx, ry, lt, rt, buttons };
	(void)env;
	(void)cls;
	pthread_mutex_lock(&touch_lock);
	memcpy(touch_state, next, sizeof(next));
	if (porting_menu.editing || (ui_menus && porting_menu.page))
		memset(touch_state, 0, sizeof(touch_state));
	pthread_mutex_unlock(&touch_lock);
}

void host_touch_read(int32_t *state)
{
	pthread_mutex_lock(&touch_lock);
	memcpy(state, touch_state, sizeof(touch_state));
	pthread_mutex_unlock(&touch_lock);
}

/* The touch controls of port/linux/src/touch_input.c publish the scene
   and the profile's bindings. This overlay tracks the menus through
   host_touch_ui_context and draws icons, so both are accepted and unused. */
void host_touch_scene(int scene)
{
    (void)scene;
}

void host_touch_bindings(const int32_t *controls)
{
    (void)controls;
}
