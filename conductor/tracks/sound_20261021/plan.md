# Plan: Sound

## Phase 1: Sounds and their formats [checkpoint: 3a736947]
- [x] Task: Sound tags and decoding 934b28e4
    - [x] Write tests: `snd!` fields; Xbox ADPCM, Ogg Vorbis and PCM decode to known samples; every sound of every stock map decodes (marked, needs `assets/maps`)
    - [x] The typed tags and the decoders, decoded into the cache
- [x] Task: The mixer ea62a97f
    - [x] Write tests: permutation choice, pitch ranges, gains by class, distance attenuation, 128 voices under 2 ms
    - [x] `opence_audio`: the mixer and voices on Core Audio
- [ ] Task: Conductor - User Manual Verification 'Phase 1: Sounds and their formats' (Protocol in workflow.md) (deferred to the end)

## Phase 2: The world's sounds [checkpoint: b1d38d4c]
- [x] Task: Events 1eb03eda
    - [ ] Write tests: footsteps, impacts, weapons, vehicles and animation events start their sounds
    - [ ] Sounds from the simulation's events in the snapshot
- [x] Task: Looping sounds, environments and occlusion a34916da
    - [ ] Write tests: `lsnd` parts and alternates; reverb by cluster; occlusion by the BSP; Doppler
    - [ ] Looping sounds, sound environments, occlusion
- [ ] Task: Conductor - User Manual Verification 'Phase 2: The world's sounds' (Protocol in workflow.md) (deferred to the end)

## Phase 3: Music, dialogue and scripts [checkpoint: 35a1e351]
- [x] Task: Music and dialogue bf9e1ea1
    - [ ] Write tests: music starts, stops and alternates; dialogue rules; conversations
    - [ ] Music and dialogue
- [x] Task: Script timing 172f552d
    - [ ] Write tests: waits on sounds against the reference game's timing in a10 and a30 (marked, needs `assets/maps`)
    - [ ] `sound_impulse_time` and the waits; the settings' volume controls; `docs/engine.md`: sound
- [ ] Task: Conductor - User Manual Verification 'Phase 3: Music, dialogue and scripts' (Protocol in workflow.md) (deferred to the end)

## Phase: Review Fixes
- [x] Task: Apply review suggestions eeb2ba13
