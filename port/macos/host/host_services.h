/*
HOST_SERVICES.H

The macOS host's services around the guest: the first start's game data
(host_launcher.c), SDL for the guest (host_sdl.c) and OpenGL for it
(host_gl.c). host_main.c and the loader call these; the guest reaches the
rest through its imports (port/android/host_imports.list and
port/macos/host_imports_macos.list).
*/

#ifndef __HALO_MACOS_HOST_SERVICES_H
#define __HALO_MACOS_HOST_SERVICES_H

/* ---------- the game data (host_launcher.c)

Works out where the game data and saves live and makes the folders: the
data root is ~/Library/Application Support/OpenCE (or HALO_DATA_ROOT, if
set), the saves its save/ subfolder (or HALO_SAVE_ROOT). If the data root
has no maps/ yet, offers to extract it from a Halo disc image the player
picks, showing its progress; the main thread's, before the guest starts
(SDL's dialogs and windows need it).

Returns 0 to go on starting the game: maps/ is there, or the run is an
unattended one (debug.hidden_window, debug.exit_after or their HALO_*
variables), for which nobody can be asked, and the game reports what is
missing itself. Nonzero if the player chose to quit instead; the caller
then exits (an extraction may still be running on its thread). */
int host_launcher_prepare(char *data_root, unsigned data_root_size, char *save_root, unsigned save_root_size);
/* the data root host_launcher_prepare chose, or "" before it */
const char *host_launcher_data_root(void);
/* whether this run is one nobody watches (HALO_NO_DIALOGS, or
debug.hidden_window or debug.exit_after from the environment or the
data root's config.toml): no dialog is shown in it */
int host_unattended(const char *data_root);

/* xiso.c's log (it has no platform layer here): the host's log */
void platform_log(const char *format, ...) __attribute__((format(printf, 1, 2)));

/* ---------- SDL (host_sdl.c) */

union SDL_Event;

/* takes the events that are the host's rather than the guest's: a link
macOS opened the app with, which SDL delivers as a dropped file (an
internet play invite, halo://..., is written to <data root>/join_link.txt
for the game to pick up: p2p.c's poll_invite_file), and anything else
dropped on the window. Returns 1 if it took the event, which is then not
the guest's */
int host_sdl_take_event(const union SDL_Event *event);

/* ---------- OpenGL (host_gl.c) */

/* the OpenGL framework's function of that name, or NULL; works before any
context exists (the loader fills the guest's imports with these) */
void *host_gl_resolve(const char *name);

#endif
