#ifndef HALO_MAC_SOUND_CONVERT_H
#define HALO_MAC_SOUND_CONVERT_H
/* Sound permutations the sound manager would not play (Ogg Vorbis,
   uncompressed, mono at 44 kHz) are served to the sound cache as Xbox ADPCM
   in the cache's converted space, converted when the cache reads them (the
   Linux build converts them all at load: port/linux/game/custom_edition_sounds.c,
   whose encoder this uses). Only the session program calls this. */
void mac_sound_convert_install(void);
#endif
