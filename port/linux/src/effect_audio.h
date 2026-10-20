/*
EFFECT_AUDIO.H

Short sound effects from WAV files, mixed into the game's output
(effect_audio.c): the headshot party's sound (headshot_party.c). Called by
port/linux/game (the game's side, so its flags are int) and voice_audio.c
(the mix).
*/

#ifndef EFFECT_AUDIO_H
#define EFFECT_AUDIO_H

/* plays a WAV file (8, 16, 24 or 32 bit PCM, or 32 bit float; mono or
stereo; any rate) once, at the gain (1: as recorded), mixed with whatever
else plays. 1 if it plays, 0 if the file is missing or cannot be read (it is
tried again in a few seconds) */
int effect_audio_play_file(const char *path, float gain);
/* (voice_audio.c, on the audio thread) the sounds added to the output
(stereo, 48 kHz) */
void effect_audio_mix(float *output, unsigned long frames);

#endif
