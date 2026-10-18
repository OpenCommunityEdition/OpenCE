#ifndef HALO_MAC_BINK_MOVIE_H
#define HALO_MAC_BINK_MOVIE_H
/* The Xbox disc's Bink movies (bink/intro.bik, credits.bik) in the game's own
   window, decoded by the port's decoder (port/linux/src/bink_decode.c) and
   drawn as a texture, with the sound through the audio unit. The decoder's
   tables are read from the disc's default.xbe (bink_tables.c); the movie
   files are external input. Movies of Xbox data only: the Mac version's
   own movies (MacSoft's) are mac_movie.c's. */
#include "mac_movie.h"

/* Plays bink/<name> of the Xbox data (the HALO_MAC_XBOX_DATA folder the
   launcher names, else data_root): a key, a click or a pad button skips it,
   Escape or the pad's Start skips what follows too. Returns 1 when it played
   or was skipped, 0 when the window was closed meanwhile, and -1 when it
   cannot be played here (the Xbox data has no such movie or no default.xbe,
   the tables are not the Rev 2 disc's, or this is a hidden run): the caller
   then plays the Mac version's movie, if the data has it. */
int mac_bink_movie_play(const char *data_root,const char *name,mac_movie_frame_runner runner,int visible);
#endif
