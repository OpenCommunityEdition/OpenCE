# Spec: Sound

## Overview
The engine plays few sounds today. This track plays every sound of the game as the original does: sound tags with their permutations and pitch ranges, looping sounds and music, dialogue, sound environments, positional audio, and the scripts that wait for sounds. It is done last, so that every animation, effect and physics event that triggers a sound exists.

## Functional Requirements
1. **Sounds (`snd!`):** the classes and their gains, the pitch ranges, the permutations and their choice (skip fractions, the "split long sounds" blocks), the distance bounds, the cone, the gain and pitch modifiers, the scale functions, the promotion to other sounds.
2. **Formats:** the Xbox ADPCM of the stock maps and the Ogg Vorbis and PCM of Custom Edition's `sounds.map`, decoded at import into the cache.
3. **Looping sounds (`lsnd`):** tracks and detail sounds, their start, loop and end parts, their alternates, and the scale functions from objects and scripts.
4. **Music:** the scripts' `sound_looping_start`, `_stop` and `_set_alternate`, with the tracks' fades.
5. **Dialogue:** the AI's dialogue (`udlg`) chosen by their events, with the vocalisation rules (one speaker at a time, the cooldowns, the importance); the scripts' `sound_impulse_start` and `ai_conversation`; the subtitles where the UI has them.
6. **The world:** positional sound with distance attenuation as Halo's, the occlusion by the BSP, the Doppler shift for projectiles and vehicles, the sound environments (`snde`) from the BSP's clusters as reverb, the background sounds (`lsnd`) of each cluster.
7. **Events:** footsteps by material, impacts by material, weapons (fire, reload, empty, overheat), vehicles' engines with their scale functions, the HUD's sounds, the UI's sounds, the animation events' sounds.
8. **Scripts:** `sound_impulse_time` and the waits on sounds, so missions that wait for dialogue keep their timing.
9. **Output:** the mixer at 48 kHz, with the classes' volume controls from the settings (master, effects, music, dialogue), and spatialisation with Apple's spatial audio where available.

## Non-Functional Requirements
- 128 voices mixed in under 2 ms on the M1 Max, off the render and simulation threads.
- No sound from the game's data is committed or shipped.

## Acceptance Criteria
- Every sound tag of every stock map decodes (a test counts failures and expects none, marked, needs `assets/maps`).
- The first two missions' dialogue and music play at the original's moments; the waits on sounds keep the scripts' timing against the reference game.
- A recorded set of scenes sounds as in the reference app (manual check).

## Out of Scope
- Voice chat.
