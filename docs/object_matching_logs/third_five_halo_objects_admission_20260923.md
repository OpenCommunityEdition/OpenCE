# Third five Halo object admissions (2026-09-23)

## Result and accounting

Five more Halo-owned units outside the concurrent Lane D allowlist are
promoted from `NonMatching` to `Matching`: 418/833 becomes 423/833. Their 78
accepted functions, 20,828 meaningful code bytes, and 89,368 target data
bytes were **already credited before this batch**. Newly matched code and
data bytes are both **zero**. This is a reviewed whole-object/source-ownership
admission, not a claim that five functions were newly reconstructed.

| Unit | Accepted target functions | Already credited meaningful code | Target data |
| --- | ---: | ---: | ---: |
| `source/game/player_queues_new` | 23/23 | 3,404 | 84,924/84,924 |
| `source/input/input_abstraction` | 10/10 | 3,314 | 688/688 |
| `source/cseries/debug_memory` | 18/18 | 2,856 | 1,120/1,120 |
| `source/rasterizer/xbox/rasterizer_xbox_screen_effect` | 13/13 | 8,657 | 1,620/1,620 |
| `source/physics/collision_usage` | 14/14 accepted | 2,597 | 1,016/1,016 |

Fresh guarded `gate.py --all` checks find 23/23, 10/10, 18/18, and 13/13
strict rows in the first four units, with no `_point_from_line3d` emission.
The fifth is **13/14 in raw gate**: `_collision_log_get_current_user` is the
existing fail-closed, accepted `csplit-relocation-alias` exception. Candidate
and January have identical 256-byte code and 23 relocations after resolving
one relocation's two spellings to the same January address; its normalized
code hash already matches, and the semantic board credits it. Its valid
alias-class park remains active rather than being silently described as a raw
COFF relocation-identity match. `input_abstraction_update` (2,384 bytes) and
`_rasterizer_screen_flash` (1,344 bytes) are similarly existing hardened
semantic/objdiff exceptions, but both pass the current strict COFF gate.

## Section and source ownership

All five have exact target-owned named section sizes, flags, normalized
payloads, symbol storage and offsets, with the one resolved relocation alias
disclosed above rather than passed off as literal relocation identity.
`player_queues_new`'s
83,364-byte BSS has the two expected typed static owners at offsets zero and
0x4110. `collision_usage` retains its three naturally tentative, descriptive
COMMON globals; January's split pools such records in `linker_common`, so
none is miscredited to this object's target data. The historical Xbox donor
and January references support the user-stack names and accesses; pool
ordering alone is not treated as proof of original TU ownership.

Candidate-only helper COMDATs, pooled literals and stock SDK tables in the
five units each have the same bytes and relocation identities as the unique
January selected copy. Their genuine shared-header inline helpers remain
unexpanded. Ordinary VC7 **base/base selected-provider pair** links were
checked in both input orders: no `LNK2005` or `LNK1169` in 18 Input, 8 Debug,
8 Player Queues, 14 Screen Effect, or 24
Collision Usage provider pairs. They end in expected unrelated `LNK1120`
imports; this is bounded duplicate/coalescing evidence, **not** proof of a
successful full mixed January/current program link. Surplus sections get
zero January-byte credit.

`rasterizer_xbox_screen_effect.c` now uses the existing complete
`rasterizer_debug_options.h` and `rasterizer_xbox_pixel_shader.h` types
instead of two TU-local partial/duplicate structs. Its 13 target functions
and 43 named target sections remain exact. `collision_usage.h` now uses
project `boolean`, names/formats every parameter, places `void` on its own
line, and removes a declaration for a nonexistent `collision_log_dispose`.
The full consumer sweep preserves all accepted exact functions and valid
parks. No new codegen directive, asm, hand expansion, or fake-match construct
was added.

## Final verification and holds

Full `ninja` passes and reports 423/833 matching. Stable verdicts over all
8,245 rows remain 7,493 exact, gained 0, regressed 0. Park validator:
151 active, 0 stale, 0 invalid. Admission audit: 36 pending candidates,
0 contradicted, 10 rejected, 0 revoked. Fake-match scanner finds 0 leads
in changed source. Tool tests: 1,152 passed, 5 skipped, 26 subtests.
`git diff --check` passes.

Several 100%-function objects remain `NonMatching` on evidence: `bitmaps`
has a reachable, unapproved `%f`/integer vararg bug; `rasterizer_xbox_draw_primitives`
still has a partial debug-options facade and the genuine-header migration
regresses an exact 480-byte function; `bitmap_utilities`, `render_debug`,
`render_objects`, and `rasterizer_xbox_water` have ordinary selected-provider
duplicate-link conflicts. No blocked source or Lane D TU was promoted to make
the requested count.
