# Native globals storage conversion

`mac_native_globals_decode` in `port/mac/native_globals_tags.h` produces an owned
native `game_globals` root using the shared checked field decoder. It converts
all 15 root blocks, nested blocks, numeric arrays, references and reference arrays;
no source pointers or block-definition pointers survive. References retain their
salted handles and copied names. Stored reference groups/handles must agree with
the cache directory. This is storage validation, not a complete semantic validator
or proof that referenced dependencies have native providers.

Layouts come from the repository's clean `game_globals.h`, material and breakable
surface headers, and `tag_schema_effects.c`. The unconsumed playlist's 148-byte
format was checked against [Invader's format definition](https://github.com/SnowyMouse/invader/blob/master/src/tag/hek/definition/globals.json):
two 32-byte strings, five 32-bit integers, then 64 reserved bytes. Only that format
information was used; no upstream implementation was copied. Playlist strings,
count and address are checked and all five numeric fields converted. Runtime
playlist execution remains outside this adapter.

On 2026-10-15, `tools/mac_native_globals_checks.sh` built with GCC 4.0 and the Tiger
SDK on the MacBook. Its Intel executable and the PPC executable on both G4s each
passed 25 checks. Blood Gulch's `e7a3062f` root owns 39,539 bytes, converts 1,358
numeric/string/reference values, and has digest `4467fdfb` on all three machines.
It contains 32 materials, one playlist entry, six look-function values and default
pitch/yaw rates 60/120. The dependency visitor reports 165 non-NONE references
(including duplicates), not 165 loaded resources.

Tests reject oversized player-control/playlist/look blocks, a NaN control value,
and invalid look/material addresses, verifying failed outputs are cleared. They
free the map source before checking owned control and curve data. The existing
biped/weapon decoder regression also passed on all three machines with unchanged digests
(37 native checks and 28 mutation checks).

Evidence: `build/mac-audit/native-globals-build.txt`,
`native-globals-powerbook.txt`, and `native-globals-ibook.txt`; the G4 object regressions are in
`native-globals-object-powerbook.txt` and `native-globals-object-ibook.txt`.

The root is now published through the native tag registry and consumed by actual
player-control map initialization in the optional `--player-control` runtime gate.
Intel and both G4s pass 123 harness checks plus 20 queue checks per map across
two cycles; see `ppc-mac-native-spawning.md` for evidence and scope. Referenced sounds, camera
tracks, interface/HUD tags, first-person models, effects and other objects still
need their own native providers before the corresponding services run. Input queues and control initialization are verified separately; camera, full
game tick, complete rendering and minimum-memory operation remain unverified.
