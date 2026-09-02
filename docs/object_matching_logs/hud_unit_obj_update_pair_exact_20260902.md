# `hud_unit.obj` local-player update pair exact recovery (2026-09-02)

## Result

Starting from canonical commit `78c08edfcaf574be9165288ac9a17b36bdce2ff8`,
the authenticated private `hud_update_unit_local_player` helper and its public
`hud_update_unit` iterator are strict exact as ordinary typed C. The focused
gate advances from **15 exact / 0 residual / 7 unwritten** to **17 exact / 0
residual / 5 unwritten** without losing an inherited exact function.

| Function | Meaningful bytes | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_hud_update_unit_local_player` | 527 | 544 | 34 | `56ee6b59eb34bac353f986275d5961009dc75d1a673627c1720b6b900a301d43` |
| `_hud_update_unit` | 45 | 48 | 3 | `848d24b2b9768cdc4c4ed6c875d24bb8ca25f6a04aacbbf07a3908a8331b036e` |

For both functions, target and candidate padded bytes, normalized bytes,
relocation counts, relocation addresses, relocation types, symbolic
destinations, addends, and ordering are identical. This contributes 572
meaningful and 592 padded strict bytes. The translation unit remains correctly
partial and `NonMatching` because five unrelated target functions remain
unwritten.

## Name, call-graph, and behavior evidence

The January symbol atlas at
`research/halo-symbol-atlas/symbols/halo_ce/4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520.jsonl`
maps file offset `0x4C6B80` to `_hud_update_unit_local_player` and file offset
`0x4C7090` to `_hud_update_unit`, both in `hud_unit.obj`. The independently
parsed January PDB labels `_hud_update_unit` at RVA `0xC7090`; its SHA-256 is
`8480f0c44fc7b5acba5775c02053d1a62c6794ab8d646661106985cbe46d7bc5`.
The private helper is absent from the public PDB records, so the atlas and the
later first-party lineage are the name evidence for its private symbol.

The HCEA donors are
`research/halocea/src/hud_update_unit_local_player.c` (repository commit
`c89106c4964f2df3a98ef7b4035d6750372797cd`, Git blob
`d2f14a0048e7c8b36d1bed3720d9aecc0f0ebc84`) and
`research/halocea/src/hud_update_unit.c` (Git blob
`c4b4344e96b244c493f02223c2eadf7fac091a9a`). They independently establish
the local-player iteration, typed player-to-unit lookup, unit-HUD cached body
and shield vitalities, shield-hit fade state machine, cinematic HUD-sound
path, and authentic helper name.

January disassembly independently fixes the Xbox-specific structure and ABI:

- the public wrapper starts at `0xC7090`, iterates with
  `local_player_get_next`, and naturally passes the private helper's short
  argument in `EAX`;
- the static helper moves that argument into `ESI`, uses the canonical
  `player_get` and `unit_get` accessors, and accesses named body/shield fields;
- the target owns the historical 128-long debug stack sentinel, the two line
  `0x201` corruption assertions, and the exact `get_return_eip`, `csmemset`,
  `temporary`, and `csprintf` relocation sequence;
- target branch and x87 order prove the three shield cases: damage, recharge,
  and unchanged shield, including the fresh-hit early exit and
  `(real)elapsed * (1.0f / TICKS_PER_SECOND)` fade increment;
- the cinematic tail reads the named `hud_scripted_globals->show_hud` field
  and passes a typed `player_get` result to `hud_play_unit_sounds`.

The stack-sentinel source shape is independently corroborated by the existing
January-exact `custom_render_nav_point` and `hud_update_nav_point_local_player`
patterns in `hud_nav_points.c`. The shared external `get_return_eip` declaration
now lives in its owner `hud_draw.h`; the stale consumer-local declaration in
`hud_nav_points.c` is removed.

## Bounded source-shape experiment

The first natural reconstruction retained the HCEA decompiler's two explicit
floating-point snapshot locals. It already matched the 544-byte padded extent
and 34-relocation count, but VC7 kept both values on the x87 stack, adding
`fld`/`fxch`/`fstp` traffic around the two comparisons and producing a
different normalized hash. That source shape was rejected.

The second and retained shape expresses the same state machine directly
through the typed `hud_state` and `unit` fields. It removes only the artificial
decompiler temporaries, makes the comparison and branch topology agree with
January, and closes the complete helper plus wrapper exactly. This is a
source-recovery correction, not register tuning or byte forcing.

## Credibility and regression review

The retained implementation uses project `real`, `boolean`, and integer types;
the canonical `player_get` and `unit_get` accessors; named structure fields;
the owning public headers; a correctly `static` private helper; and explicit
terminal returns. The complete four-byte scripted-HUD globals view remains
translation-unit private to avoid perturbing unrelated header consumers.

It contains no address-named private function, raw object or tag cast, raw
structure offset, synthetic reference, fake dependency, representation pun,
undefined behavior, forced inline/noinline, assembly, `volatile`, `register`,
pragma, intrinsic barrier, compiler-flag exception, object patch, or
nonsensical condition. The conservative fake-match scan reports zero review
leads in the changed source and owner header.

The `hud_draw.h` declaration blast-radius was measured before and after the
owner-header move. The exact/residual/unwritten counts remain identical for
all five direct consumers:

- `hud_draw.obj`: 2 / 0 / 21;
- `hud_nav_points.obj`: 30 / 2 / 0;
- `hud_messaging.obj`: 30 / 2 / 4;
- `render_contrails.obj`: 6 / 0 / 0;
- `lightning.obj`: 7 / 2 / 0.

The focused final `hud_unit.obj` gate is 17 exact / 0 residual / 5 unwritten,
and its rebuilt object contains no candidate-only text symbol. The protected
`units.obj` gate remains 189 / 0 / 0, while `players.obj` remains 54 / 15 / 1.

The isolated full `all_split all_source progress semantic_progress` graph
passes. Its evidence is:

- semantic audit: 473 units, 5,260 functions evaluated, 5,103 semantic exact,
  5,115 accepted exact, and zero unit errors;
- strict board: 278/619 objects, 5,090/8,245 functions, and
  730,979/1,922,413 padded code bytes;
- ordinary progress: 669,888/2,198,102 meaningful code bytes,
  5,071/11,060 functions, and 1,872,655/4,176,062 data bytes;
- parked validation: 106 active, zero stale, and zero invalid;
- object admission: zero candidates, zero contradictions, one pre-existing
  explicit rejection, and zero revocations;
- tooling tests: 258/258 pass;
- `git diff --check`: clean, and the changed-file fake-match scan reports zero
  review leads.

The integrator must still repeat the full gate after applying this packet to
the then-current canonical head.
