# `rasterizer_frame_statistics.obj` Opus draw-only reconciliation (2026-09-06)

## Verdict

**HOLD; no production patch is recommended and no strict credit is available.**

The reconstructed `rasterizer_frame_statistics_draw` body is substantial and
semantically coherent, but the independently compiled draw-only packet is a
4,176-byte / 322-relocation residual.  The Opus object's exact draw depends on
also retaining Opus's previously rejected, fuzzy `rasterizer_frame_statistics_get_fps`
body earlier in the translation unit.  Moreover, the last source changes that
made the donor draw exact were selected through declaration-order and local-order
experiments.  This is compiler-state/matching-directed evidence, not authentic
January source topology.

The natural draw-only residual is preserved below as research.  It must not be
reported as an exact closure.

## Scope and immutable inputs

- Canonical baseline inspected: `bcc049329401607b0f5264588eff1e6ab9f6915d`
  (the frame-statistics C/header/config were unchanged while the Devices owner
  worked elsewhere).
- Opus donor: `C:/halo-worktrees/opus-30k-rasterizer-widgets-20260905`, tip
  `bd8c008e85a1543877d0843bab3f1c2dffab9a13`.
- Donor first large-body commit: `83252d44b`.
- Donor exact-claim commit: `7841c6ac306c4087af584a380b9e2aa44c45126d`.
- Donor final C SHA-256:
  `6386f4eb7c7d68997194bfca67c3615399acab3b41aaea01370cced79aa41d44`.
- January target object:
  `build/split/source/rasterizer/rasterizer_frame_statistics.obj`, SHA-256
  `26bd4b1483b12e1c15673041dd25f030fdf02423b21541c776a773965fe1e736`.
- Current canonical base object:
  `build/base/source/rasterizer/rasterizer_frame_statistics.obj`, SHA-256
  `ed458dd0e40159c92e8ba15ee94ba076333c2fed41eb2be09b32db601f0d02ed`.

No production source, header, config, build file, or donor file was changed.
No Ninja or full build was run in this audit.

## Scratch candidates and results

| Purpose | Source/object SHA-256 | Draw result |
| --- | --- | --- |
| Draw-only donor body, with the complete fuzzy `get_fps` definition removed | `scratch/rasterizer_frame_statistics_draw_only_candidate_20260906.c` = `9e4cc8359c92f6fb21a14761bdf0572f9a6ee12a754cf3d307af19866807a208`; `scratch/rasterizer-frame-statistics-draw-only-20260906.obj` = `abf99ef921d0ce41441fc45989ee6e4e58bf02e49d1864a8ec0de80fb3e4b515` | 4,176 B, 322 relocs, residual normalized SHA `30aab13ebd011f789202b4e8417572903afbf2f4fbdaa1b52553357362c8b111` |
| Full Opus TU under current canonical flags (diagnostic only) | `scratch/rasterizer-frame-statistics-donor-full-current-flags-diagnostic-20260906.obj` = `15c2f8550d1b7c3974ab8b5689efd5aead2ec8a3d9fb3c304f729f680fe29546` | exact: 4,176 B, 322 relocs, target normalized SHA `747b937d70b288778c244bc3e426d03294da76a99ea3afd31cc23abb5edcccec` |
| Draw-only plus a redundant `get_fps` declaration | source `659915c12383ba575dfad58067a70a1e1e44747cb6c56ee66759b2382fd4ec33`; object `f12d765ab9bce49175721afacd1ec9e5f90d63f5238bf6c042fcea4860110b1a` | 4,176 B, 322 relocs, residual SHA `596dde6cc2b538e27d2ea493ad7712ce774657181e6833a9e663037b4cc8fbce` |
| Draw-only plus experimental narrow profile header | source `14375a7d98aed62a81e07865815fc9dbd60c89243ea54135cb7f52bf76140016`; header `6335207566ca5d39fc9b3827a4b0f25df92c43d45a651e04dacc8fd996665645`; object `5b3471fec9b2215b08bee481e436d7b58819c2655f6884a389e5dc2e499ad6c6` | 4,176 B, 322 relocs, residual SHA `d537a6143fa20820eae62bfc3b57d94f71a3516763979b2b1a21ad3e658fc947` |

The January target draw is 4,176 bytes, has 322 ordered relocations, and has
normalized SHA-256
`747b937d70b288778c244bc3e426d03294da76a99ea3afd31cc23abb5edcccec`.

The draw-only gate was run with the current canonical compile flags and:

```text
python -B tools/campaign/gate.py source/rasterizer/rasterizer_frame_statistics \
  --source scratch/rasterizer_frame_statistics_draw_only_candidate_20260906.c \
  --all --forbid-emitted-symbol _point_from_line3d \
  --out scratch/rasterizer-frame-statistics-draw-only-20260906.obj
```

The forbidden-owner check passed.  The object remained six exact, three
residual, one unwritten: all six inherited exact functions stayed exact;
`eat_my_shorts` and `count_static_vertices` retained their existing residuals;
the new draw was residual; and `get_fps` stayed unwritten.  Against the current
base, the only added code owner is `_rasterizer_frame_statistics_draw`.

`tools/campaign/alndiff.py` localizes the substantive draw-only mismatch to the
initial aggregate calculations.  At draw offsets `0x6e`, `0x7e`, `0x8d`,
`0xca..0xd9`, `0x10f..0x117`, and `0x197..0x1b4`, VC7 commutes independent
field loads/add destinations and therefore changes ordered relocation identities
(for example frame-statistics offsets `0xb0`/`0xa0`, `0xa8`/`0xb4`, and
`0xe4`/`0xd8`).  The remainder of `alndiff` differences are the expected
defined-in-target versus external-in-candidate descriptions of the same BSS and
literal owners.  Equal size and relocation count are not an exact match.

## Why the donor exact source is not admissible

1. The canonical fixed-point ledger
   `docs/object_matching_logs/rasterizer_frame_statistics_obj_jonas_get_fps_fixed_point_20260829.md:11-32`
   records the one natural January-version `get_fps` candidate as 352 B / 21
   relocs versus 384 B / 21 relocs and explicitly says not to tune declarations,
   expressions, or control flow around that rejected shape.  The Opus TU retains
   that fuzzy body at donor source lines 378-426, immediately before the draw at
   line 532.

2. Removing only that body makes the otherwise unchanged draw residual.  A mere
   declaration does not restore the schedule.  Thus importing the rejected body
   only to perturb C2 state would be a synthetic predecessor dependency, outside
   this draw-only scope.

3. The donor has 283 `scratch/fstat*` artifacts.  In particular,
   `scratch/fstat_perm.py:7-32` enumerates every permutation of the three color
   declarations, rewrites the staged source, compiles it, gates the draw, scores
   it, and restores the file.  `scratch/fstat_try.py:8-37` applies arbitrary
   source replacements, recompiles, scores, and restores them.  There are also
   `fstat_batch.py`, `fstat_try2.py`, four comparison scripts, stage scripts,
   and at least sixteen `fstat_exp*.py` experiment drivers.  Pinned hashes:

   - `fstat_perm.py`:
     `42341e781b02311fa9f5eb279b95c5bdde428b182f074ebc60e700b0ad5fc310`
   - `fstat_try.py`:
     `03ef6820d8fa4f9e3edf3aef4df157f372bf66e9d418c8979ba70e8a0502d5a9`
   - `fstat_try2.py`:
     `5a38fa3de366e272ee4678fafb73eb3e9c1053d39ff255de13df4652f1641d46`
   - `fstat_batch.py`:
     `2bd2c905cdb89e3db7dca76012210e81c29f0d9d220ff74f66632dff5ef73433`

4. Commit `7841c6ac3` removes two layout asserts and changes only source-order
   details in the draw: it changes `default,data,header,tab_stops` to
   `tab_stops,data,header,default`, and moves three memory-mode scalars before
   the initialized report array.  Those exact kinds of changes are what the
   retained permutation/edit scripts searched.  The donor wave-5 ledger's claim
   at `docs/object_matching_logs/opus_30k_rasterizer_widgets_wave5_20260906.md:49-51`
   that the body was "driven to strict exact" therefore cannot establish
   source authenticity.

5. The only later HCEA source found is
   `research-cache/halocea-full-blobs-20260830/src/blam/rasterizer/rasterizer_frame_statistics_draw.c`,
   SHA-256
   `71ecd923ac43e3d2f7828a12efe199b2ccdc89d4dba0cb33d3de040f40d896a3`.
   Its lines 40-154 implement a different retail hotkey/graph/FPS overlay and
   do not contain January's objects, geometry, profile, memory, or profile-log
   branches.  It corroborates a few display idioms only; it does not authenticate
   the January declaration order or full body.

## Types, prototypes, data, and linkage

- The public draw prototype already lives in its proper owner header at
  `source/rasterizer/rasterizer_frame_statistics.h:15-18`; no header edit is
  needed.  The rejected `get_fps` prototype must not be used as a reason to add
  its body.
- The candidate's private layouts are local to the owner C and are pinned by
  `sizeof`/`offsetof`: frame statistics `0x170`, private prefix `0x860`, temp
  buffer `+0x740`, profile file `+0x758`, FPS samples `+0x760`, count `+0x850`,
  profile index `+0x854`, and debug `statistics_mode +2` / `profile_log +0x4c`.
  The target's 322 relocations corroborate the accessed offsets; this does not
  authenticate declaration order.
- Existing profile API declarations are in their current owning header,
  `source/rasterizer/xbox/rasterizer_xbox.h:55-80`.  The scratch narrow-header
  experiment was diagnostic only, changed the draw hash again, and supplies no
  basis for a production header migration.
- `_profile_log_path` is an ordinary external 4-byte data owner initialized to
  `"d:\\r-prof.txt"`; the name already exists at `config/symbols.json:22245`
  without `static`.  The candidate emits the expected owner and draw literals.
- The candidate intentionally declares, rather than defines,
  `_rasterizer_frame_statistics_private_globals`.  January's split owns the
  `0x860` BSS and current metadata names it with `static` at
  `config/symbols.json:22640`.  No new candidate BSS owner or duplicate is
  introduced.
- The D3D encode-table COMDATs visible in the scratch object are inherited from
  the current canonical base object; they are not draw-only candidate additions.
  Their absence from this January split object is therefore not a new regression
  caused by this packet.
- Existing function metadata already names draw at `config/symbols.json:6183`.
  No config rename, alias, storage, or emission-control change is required.

## Proposed `apply_patch` disposition

No production `apply_patch` should be applied from this handoff.  Preserve
`scratch/rasterizer_frame_statistics_draw_only_candidate_20260906.c` as the
readable semantic residual and record zero strict gain.

Reopen only with either authentic January source/source topology, or as a
separately scoped semantics-only fuzzy admission whose ledger explicitly
declines exact credit and whose full build, owner census, data/BSS/link, and
header-consumer gates are run from a clean frozen baseline.  Do not import the
fuzzy `get_fps` predecessor or permute declarations to recover this draw's bytes.
