#ifndef HALO_MAC_MOVIE_H
#define HALO_MAC_MOVIE_H
/* The retail Mac game's start-up movies (Movies/MSWI_Logo.mov, bungie.mov,
   gearbox.mov, mgs.mov, in that order) played with QuickTime in the game's
   own window: a movie's frames reach the window's OpenGL context as textures
   (a QuickTime OpenGL texture context, on Mac OS X 10.4 to 10.6, PowerPC and
   Intel) and QuickTime plays the sound. The movie files are external input:
   one that is missing or that QuickTime cannot open is skipped. */
struct mac_input_frame;
typedef int (*mac_movie_frame_proc)(void *,const struct mac_input_frame *,unsigned,unsigned);
/* mac_device_gpu_frame's shape: calls the frame procedure once, in a frame
   of the window (events, the framebuffer bound, the frame presented after) */
typedef int (*mac_movie_frame_runner)(mac_movie_frame_proc,void *);

/* Plays the movies of data_root/Movies. visible is false for the hidden
   (offscreen) runs, which play nothing unless HALO_MAC_MOVIES_HIDDEN=1 (a
   check of this player; silent then). A key, a click or a pad button skips
   the movie; Escape skips the rest. Returns 1 when the movies are done or
   were not played, and 0 when the window was closed meanwhile.
   Diagnostics: HALO_MAC_MOVIE_LIMIT=<seconds> stops each movie after that
   long, HALO_MAC_MOVIE_CAPTURE=<prefix> writes <prefix>-<movie>-<n>.ppm
   (HALO_MAC_MOVIE_SHOT_EVERY=<seconds>, default 1).
   Timing: HALO_MAC_MOVIE_STATS=1 prints, when each movie ends, the processor
   time, the time of each stage, the gaps between images and the frames
   dropped (2 also waits for the card after each draw); HALO_MAC_MOVIE_STEP=1
   with it decodes frame by frame without a clock; HALO_MAC_MOVIE_PIXELS=
   yuv|argb|bgra asks QuickTime for those buffers (default: its choice);
   HALO_MAC_MOVIE_PRIME=0 and HALO_MAC_MOVIE_LEGACY=1 give the earlier start
   and loop back, for comparing. */
int mac_movie_play_intro(const char *data_root,mac_movie_frame_runner runner,int visible);
/* One movie of data_root/Movies by file name (the campaign's ending.mov), played
   as the intro's are: 1 when played or skipped, 0 when the window closed. */
int mac_movie_play_named(const char *data_root,const char *name,mac_movie_frame_runner runner,int visible);
#endif
