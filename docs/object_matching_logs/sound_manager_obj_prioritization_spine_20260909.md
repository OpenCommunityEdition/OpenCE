# `sound_manager.obj` prioritization spine (2026-09-09)

Base commit: `105d9acaacda589e21a4389d7e6bd69db0b42aba`

This packet reconstructs five previously unwritten private functions in
`source/sound/sound_manager.c`:

| function | meaningful bytes | padded bytes | relocations | gated status |
| --- | ---: | ---: | ---: | --- |
| `_prioritize_sounds` | 725 | 736 | 45 | strict exact |
| `_sound_find_best_channel` | 413 | 416 | 17 | strict exact |
| `_sound_find_channel` | 481 | 496 | 30 | strict exact |
| `_sound_preempts_sound` | 243 | 256 | 15 | strict exact |
| `_track_loop_impulse_sound` | 231 | 240 | 4 | strict exact |
| total | **2,093** | **2,144** | **111** | **5 strict exact** |

The first measurements used a temporary direct caller so that C2 would emit
the private graph. The temporary caller is absent from the retained source.
The reconstructed `sound_render` is now the authentic root, and a fresh gate
on the integrated source confirms that all five sections remain strict exact.

## Source evidence and corrections

The HCEA reconstructions were used only as a topology map. January's target
object supplied the authoritative control flow, assertion text, line numbers,
types, constants, and relocation identities. In particular:

- `_prioritize_sounds` treats the cache-miss policy as the explicit
  `_sound_cache_miss_mode_discard` / `_sound_cache_miss_mode_postpone` enum and
  asserts on invalid values. The HCEA map incorrectly reduced this to a
  boolean.
- `_prioritize_sounds` preserves January's delayed-or-postpone invariant for
  sounds whose start time is still in the future, plus the channel-format and
  channel-ownership assertions omitted by the HCEA map.
- `_sound_preempts_sound` preserves the distinct challenger/champion
  definitions and the original repeated `sound_class_get` expressions. It
  does not cache priorities in invented locals.
- `_sound_find_channel` validates an already-owned channel before returning
  it and keeps the speech and instance-limit fallback paths distinct. That
  natural source shape explains January's two separate best-channel calls.
- `_track_loop_impulse_sound` uses the typed tracking payload and ordinary
  aggregate copies. The point/vector representation cast is limited to the
  semantically identical three-real position offset; there is no raw byte
  emission or assembly.

## Verification

- `tools/campaign/gate.py`: all five integrated sections strict exact;
  2,144/2,144 padded bytes and all 111 relocations matched.
- `tools/campaign/gate.py --forbid-emitted-symbol _point_from_line3d`: passed.
- `tools/fake_match_scan.py source/sound/sound_manager.c`: zero review leads.
- `git diff --check`: passed.
- Final source contains no artificial gate root.
