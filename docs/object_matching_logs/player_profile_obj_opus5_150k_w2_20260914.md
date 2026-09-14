# `player_profile.obj` — opus5 150K house-clean lane, wave w2 (2026-09-14)

## Baseline

Gate: 18 exact / 4 residual / 0 unwritten, with the point guard passing. All four residuals are parked, and parkcheck on the real file shows no drift. The real file is unchanged.

## Outcomes

### Blocked by the owner and by original-bug policy

**`_player_profile_write_thread_proc@4`** (target 352/27, real 384/27)
- alndiff: the size gap is exactly the `= {0}` clear of the 512-byte file block. January serialises an uninitialised 444-byte tail.
- Owner rule: the batch owner repaired this ("Do NOT remove initialization to regain byte credit"), and the brief forbids uninitialised reads.
- Scratch measurement without the initializer (`pp_probe1.c`): 352/27, but the sha still differs.

**`_player_profile_create_default_profiles_on_disk`** (target 288/14, real 336/14)
- Same uninitialised-tail repair.
- Without the initializer (`pp_probe2.c`): 288/14, sha still residual.

**`_player_profile_new`** (464, relocs 19 vs 18)
- January's failure path sets the index to NONE and then calls `saved_game_file_close(&file, NONE)`. That API asserts on the index fields (`saved_game_files.c` lines 603–609).
- Owner rule: the owner repaired it.
- Exactness would require reproducing that fatal path, so no attempt was made.

### Not landed

**`_player_profile_get_argb_color`** (80/1, sha only)
- January reads the rgb fields through the returned pointer, never writes them back to the local, and stores alpha after the call.
- Three shapes were tried: the current body, alpha before the call, and a named result pointer. All stay residual; this is a store-forwarding tie.

## Reopen criteria

- **The three blocked functions:** an owner ruling on reproducing January's uninitialised file tail and its close-with-NONE failure path. The probes show that the tail alone is not enough for exactness.
- **ARGB:** authentic evidence of the copy statement's shape.
