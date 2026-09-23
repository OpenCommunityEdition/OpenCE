# Player profile color return-by-value correction (2026-09-24)

`player_profile_get_rgb_color` and `player_profile_get_argb_color` return color structs by value, as the first-party `/Od` build shows through hidden result pointers (0x85bf60 and 0x85b8e0). The ARGB function's RTC records name `argb_color` (16 bytes) and `rgb_color` (12 bytes), in that order. The real prototypes belong in `player_profile.h`; the sole external caller in `game_engine.c` consumes the RGB return directly. This removes a redundant pointer API invented by the reconstruction.

The smallest coherent edit changes those two declarations/definitions and the one caller. It does not bring over any other Lane D candidate, including the held original-bug and type-punning changes in `player_profile.c`. The pre-existing `_player_profile_get_rgb_color` and `_game_engine_player_get_change_color` exact functions remain exact. `_player_profile_get_argb_color` is newly strict exact at 80 padded / 68 meaningful bytes and its stale park was retired.

Isolated full build, using the same January split and VC7 flags as canonical: 8,245 target functions, 7,586 -> 7,587 strict exact, one gain and zero regressions on a stable-section board diff. Halo meaningful code credit increases 1,551,775 -> 1,551,843 of 1,770,166. Park check: 102 active / 0 stale / 0 invalid. Admission audit: 0 contradicted. Fake-match scan: 0 leads in the changed C files. Tests: 1,154 passed, 5 skipped, 26 subtests. No config or symbol ownership changed beyond the retired park.

Prior probe and first-party evidence: `docs/object_matching_logs/claude_lane_d_refresh_manifest_20260922.json`, entry for `_player_profile_get_argb_color`; isolated measurements are in `scratch/before.json` and `scratch/after.json` of the worktree (untracked).
