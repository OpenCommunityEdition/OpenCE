# `game_sound.obj` Opus core reconciliation (2026-09-04)

## Scope and provenance

This packet was reconstructed in an isolated worktree from canonical
`ddb213b3ef8477d027e0ce8d2c20d48fb122a5fd`. The interrupted Opus donor was
the frozen `opus/small-families-30k-20260902` worktree at
`eeedd72e5ba0fc1761519f68cad241fe606320e7`. Its implementation was treated
as a research lead rather than copied: it introduced private shadow layouts
and placed shared ownership in the wrong translation unit.

The final source was reconciled against the January COFF and independently
corroborated by the HCEA reconstructions under
`_research_cache/halocea/src/blam/sound`. Only the four exact lifecycle
functions, one real shared-field name, and the types they directly prove are
admitted here.

## Reconciled source

- `looping_sound_definition::runtime_scripting_sound_index` now names the
  proven field at offset `0x1C` in its owning `sound_definitions.h`; the
  compile-time offset assertion remains in that header.
- `game_looping_sound_datum` names its state and last-audible-frame fields,
  and access goes through the local typed `game_looping_sound_get` macro.
- The local state enum and fake-impulse flag replace raw state and bit
  literals. Existing typed looping- and impulse-sound tag accessors are used.
- `game_sound_clear`, `game_sound_restore`, `game_looping_sound_new`, and
  `game_looping_sound_delete` are readable typed C with normal control flow.

No private shadow tag structure, raw member offset, address-shaped private
name, fabricated global, or representation pun was retained. No inline
assembly, volatile scheduling device, optimizer pragma, force-inline marker,
or byte-emission construct is present. No-argument definitions spell `void`,
parameters are formatted one per line, and all four void paths end with an
explicit `return;`.

## Focused exactness

The canonical base gate was 13 exact, 0 residual, and 18 unwritten functions.
The reconciled gate is 17 exact, 0 residual, and 14 unwritten functions.

| Function | Padded bytes | Verdict |
| --- | ---: | --- |
| `_game_sound_clear` | 176 | exact |
| `_game_sound_restore` | 208 | exact |
| `_game_looping_sound_new` | 256 | exact |
| `_game_looping_sound_delete` | 160 | exact |

The strict gain is four functions and 800 padded bytes. Campaign accounting
advances by four functions and 779 meaningful bytes, from 887,284 to 888,063
matched code bytes. The unit remains `NonMatching`; its fourteen unwritten
functions are not claimed or parked by this packet.

## Verification

- Focused gate: 17 exact / 0 residual / 14 unwritten.
- Forced full Halo and libcmt build graphs: pass.
- Rename-stable whole-tree comparison: four gains / zero regressions.
- Campaign progress: 888,063/2,198,102 meaningful bytes and 6,090/11,060
  exact functions (40.40%).
- Semantic audit: 473 units, 6,524 functions evaluated, 6,147 semantic exact,
  6,163 accepted exact, 169 hidden functions / 101,147 hidden bytes, and zero
  unit errors.
- `units.obj`: 189 exact / 0 residual / 0 unwritten after the shared-header
  edit.
- Parked audit: 234 active / 0 stale / 0 invalid.
- Admission audit: zero candidates, contradictions, or revocations; the two
  inherited rejections remain unchanged.
- Fake-match scan over the changed source and header: zero review leads.
- Tool suite: 261/261 tests pass.
- `git diff --check`: pass.
- Neither the target nor rebuilt `game_sound.obj` owns a
  `point_from_line3d` symbol or COMDAT.

No configuration status changes, frozen target objects, or prior matching
logs are modified by this packet.
