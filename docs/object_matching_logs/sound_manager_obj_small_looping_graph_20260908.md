# `sound_manager.obj` small looping graph reconstruction (2026-09-08)

## Result

This packet advances `source/sound/sound_manager` from **27 strict-exact,
6 residual, and 32 unwritten** January function owners to **32 strict-exact,
8 residual, and 25 unwritten** owners.  It adds five genuine exact functions,
**981 meaningful / 1,008 padded exact code bytes**. Every inherited
strict-exact function remains exact. The object is still incomplete. The
source-authentic `loop` local also restores January's 41-byte stringized
assertion owner; final whole-tree data accounting is reported separately.

| January owner | meaningful | padded | relocations | normalized SHA-256 |
|---|---:|---:|---:|---|
| `_looping_sound_find` | 79 | 80 | 6 | `16110281b6483bbf1caf61d136b12c4fe3d392379c62827d979ade60d4386b39` |
| `_render_debug_looping_sound` | 349 | 352 | 18 | `290dba10e1e81030cc5171bb40fada36e02a7e2ec980fa225de754b7d1d17cac` |
| `_sound_set_definition_begin` | 30 | 32 | 2 | `a1062aea5fdd02d4e1bc856e96565659d38067d5be69dbaa7bb7be67ace61a7b` |
| `_track_loop_track_sound` | 51 | 64 | 2 | `0573b931827c886e0e1392764d91aeafa5511de84761e2f1847a4d424849ff04` |
| `_update_potentially_audible_looping_sound` | 472 | 480 | 20 | `eadd707ebf7a7b7d38a15d53361eb6cecff3650b93d506fa1d3a1656a515d0cc` |

The meaningful sizes are the January function sizes recorded in
`build/report.json`; padded sizes, relocation counts, and normalized hashes are
from direct strict COFF comparison.  Code bytes and semantic relocation
identities agree for every row.

## Reconstruction evidence

January target disassembly, January relocation order, and January object layout
remain authoritative.  The independently reconstructed later Xbox build in
[surreptitiousresearch/halocea](https://github.com/surreptitiousresearch/halocea)
at commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c` supplied a semantic and naming
map, chiefly under `src/blam`:

- `sound/looping_sound_new.c`;
- `sound/looping_sound_new_sound.c`;
- `sound/looping_sound_find.c`;
- `sound/track_loop_track_sound.c`;
- `render/render_debug_looping_sound.c`; and
- `cache/sound_refresh_looping.c`.

The build-family mapping matters: January
`_update_potentially_audible_looping_sound` is the component-sound allocator
called `looping_sound_new_sound` in the later reconstruction.  January's own
`_looping_sound_new_sound` is a different transition helper and receives no
credit in this packet.

`_render_debug_looping_sound` deliberately fetches tag-block element zero on
each pass through both bounded loops.  The January instruction stream requires
the non-advancing record pointer, and the independent later-build
reconstruction records the same original defect.  This is therefore preserved
source behavior, not a coincidental or nonsensical matching trick.

## Best-known fuzzy source retained with zero credit

Two authentic, useful reconstructions are retained in the C file so future work
does not restart from an unwritten body.  Neither is counted as exact, complete,
or matching:

| owner | January | candidate | verdict |
|---|---|---|---|
| `_looping_sound_new` | 304 padded, 14 relocations, `c611da5f...` | 320 padded, 14 relocations, `33527f3f...` | Credible control flow and exact relocation cardinality, but different code size and instruction hash; **zero credit**. |
| `_sound_refresh_looping` | 1,136 padded, 43 relocations, `22e6f765...` | 1,088 padded, 42 relocations, `4820b25c...` | Substantial January/later-build semantic reconstruction, but one relocation and 48 padded bytes remain missing; **zero credit**. |

The latter's standalone `sound_get` validation calls are evidence-backed:
January has the corresponding `_sound_data` and `_datum_get` relocations. The
candidate uses the existing named `sound_inaudible_fade_out_time` constant and
the source-attested local name `loop`, which restores the exact stringized
assertion. It contains no unused-local dependency. The complete routine remains
parked here only as the best-known fuzzy source.

## Validation

The final isolated production command was:

```text
python -B tools/campaign/gate.py source/sound/sound_manager --all --out scratch/sound_manager_final_packet.obj --forbid-emitted-symbol _point_from_line3d
```

It reported **32 exact / 8 residual / 25 unwritten of 65**, and the emitted
symbol guard passed. Direct `tools/coff_compare.py` comparison independently
reported `all_equal: true` for all five credited owners.  Additionally:

- `python -B tools/fake_match_scan.py source/sound/sound_manager.c --fail-on-findings`
  passed with zero review leads;
- `git diff --check -- source/sound/sound_manager.c` passed (apart from Git's
  existing LF-to-CRLF advisory);
- no shared header, configuration, symbol map, or parked-function registry was
  edited; and
- no full build, configure step, commit, or push was performed by this isolated
  packet worker.

The new functions use project `real`/boolean types, typed tag/data access
macros, named enum constants, canonical private names, one-parameter-per-line
formatting, and explicit terminal returns where applicable.  No inlining or
assembly was introduced.
