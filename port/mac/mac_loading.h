#ifndef HALO_MAC_LOADING_H
#define HALO_MAC_LOADING_H
/* The loading screen: the Xbox game's (interface/progress_bar.c, which the Linux and
   Windows ports run) drawn in OpenGL. Its frame is the last picture shown (the menu's),
   blurred and faded a little more each frame, with a band of light sweeping across as the
   load goes on; its brightness swells and fades with the progress, and the picture is
   gone at its end. It is drawn into the window's framebuffer inside one of the window's
   frames (mac_device_gpu_frame), as mac_movie.c's movies are. */

/* Begins a loading screen over the picture the pixels hold (A8R8G8B8, rows from the top,
   pitch_pixels pixels apart: a copy is kept), or over black if there are none. */
void mac_loading_start(const unsigned *pixels,unsigned width,unsigned height,unsigned pitch_pixels);
/* One frame, progress 0 to 1 (smoothed here, as the Xbox's is). shot_path, if not
   NULL, has the frame written to it as a PPM. */
void mac_loading_draw(unsigned width,unsigned height,float progress,const char *shot_path);
/* The progress as drawn (smoothed): what the chant that plays with it follows. */
float mac_loading_progress(void);
/* Ends it: its textures are let go (inside a frame, like the draws). */
void mac_loading_stop(void);
#endif
