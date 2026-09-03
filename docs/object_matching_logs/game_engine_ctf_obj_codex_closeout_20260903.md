# `game_engine_ctf.obj` weapon-update closeout (2026-09-03)

## Result

This pass starts from canonical commit `3be7d0261` and preserves all 38
strict-exact functions in `source/game/game_engine_ctf.obj`. The sole remaining
576-byte function, `_ctf_engine_weapon_update`, is complete meaningful C but
does not naturally become byte-exact. Its retained reconstruction improves
objdiff from **77.044945%** to **90.87079%** and is honestly parked rather than
crediting a compiler-scheduling coincidence.

The implementation change is committed separately as `e1e1bc627`. It retains
the ordinary typed engine logic and improves the code shape by:

- saving the game tick once before the idle-return test;
- spelling the connected-map state with the established `TEST_FLAG` macro;
- isolating the authentic short owner-team value and long modulo result inside
  the idle-return block;
- preserving the reset outside the handled-message condition; and
- using the carrier-present path as the goal-marker fallthrough before the
  no-carrier clear.

No shared header, owner prototype, public ABI, symbol name, or structure layout
is changed by the CTF implementation.

## Source and donor audit

The target disassembly and the existing CTF assertions remain the primary
January evidence. The following independent material was read in full for this
function:

- `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/_research_cache/halocea/src/blam/game/ctf_engine_weapon_update.c`;
- `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research-cache/stian-halo-complete-history-20260828/src/halo/game/game_engine.c`;
- `docs/object_matching_logs/game_engine_ctf_obj_fable_salvage_reconciliation_20260902.md`;
- `docs/object_matching_logs/game_engine_ctf_obj_fable_small_families_20260901.md`; and
- `docs/object_matching_logs/game_engine_ctf_obj_jonas_public_leaves_20260828.md`.

All reachable local Claude, Opus, and Fable refs/worktrees were searched for
the object and function. None contains a stronger implementation. The
target-shaped Halo CE reconstruction is especially useful evidence for the
local widths: a signed 16-bit flag team, 32-bit carrier and tick, and 32-bit
opposing-team result. HaloCEA independently corroborates the three semantic
phases: single-flag swapping, unattended-flag return, and carrier goal-marker
maintenance.

## Fixed-point evidence

| Measurement | January | Retained candidate |
|---|---:|---:|
| Padded section bytes | 576 | 576 |
| Meaningful bytes | 566 | 566 |
| Instructions | 188 | 188 |
| Relocations | 40 | 40 |
| Normalized SHA-256 | `0c8e2649895a583167f4b4c8d53d9d29c7e4c0edbad2488e717d4517ce66d20b` | `bff168bc4a51e834f7be79383114d53173377a1ba2c60404ff287930f5e5ecd5` |
| Objdiff | 100% | 90.87079% |

The ordered relocation identities are equal. Relocation-normalized byte
comparison finds only 78 differing byte positions, all between offsets
`0x13A` and `0x194`. The alignment-aware instruction stream is identical
before the idle-return message block and resynchronizes at `0x19A`, remaining
identical through the end of the function.

January promotes the signed owner-team value into EDI and calculates the
opposing team in EBX before testing `_ctf_weapon_handled_bit`. Natural VC7 C
tests the handled flag first, retains the owner team in BX until its uses, and
then calculates/promotes the message arguments. This is a coherent register
and instruction-scheduling residual, not missing gameplay behavior.

The following bounded variants were measured and rejected when they did not
close or improve the residual:

- Fable W3/W4 block-local team declarations, W5 connected-state local, W6
  carrier branch polarity, and their combinations;
- whole-function versus return-block short/long team types;
- signed and unsigned owner/opposing-team declarations;
- declaration and initialization order permutations;
- a handled-state boolean and warning-state temporaries;
- explicit promotion/recomputation temporaries, which VC7 optimized away or
  scheduled less faithfully; and
- direct connected-bit lowering, which violates the macro rule and did not
  close the function.

The fail-closed codegen classifier reports `UNKNOWN`, class `unclassified`,
confidence `NONE`, while proving equal sizes, equal relocation counts, and
equal ordered relocation identities. The park should reopen only for
authoritative January source/local records or a natural compatible-compiler
donor explaining the remaining promotion schedule.

## Authenticity and house-rule audit

The retained source uses `long`, `short`, `real`, `real_point3d`, named enum
constants, `TEST_FLAG`, and the existing typed `WEAPON_GET` path. The private
function already carries its semantic `_ctf_engine_weapon_update` symbol; no
`code_<address>` or `bss_<address>` identifier is introduced. It ends with an
explicit `return;`, does not add an inline helper, and emits no
`point_from_line3d` symbol or COMDAT.

The fake-match scan reports zero review leads. There is no forced
inline/noinline, volatile or register steering, barrier, pragma, inline
assembly, raw offset, manual flag-mask substitute, fake dependency,
representation pun, or nonsensical branch in the retained source.

## Validation

- focused default gate: CTF **38 exact / 1 residual / 0 unwritten**;
- focused `/W3 /WX` gate: CTF **38 / 1 / 0**;
- candidate: 576 padded bytes, 566 meaningful bytes, 188 instructions, and 40
  ordered relocations;
- fail-closed classifier: `UNKNOWN`, no match credit;
- fake-match scan: zero review leads;
- full `ninja`: successful compile, split, objdiff report, semantic report, and
  progress verification;
- full `python -m pytest -q`: **261 passed**;
- full parked-function manifest: **161 active / 0 stale / 0 invalid**;
- cumulative verified board: **777,351 meaningful code bytes** and 5,589
  functions;
- `git diff --check`: clean;
- candidate symbol census: no `point_from_line3d` symbol;
- touched-file census: only `source/game/game_engine_ctf.c` plus this evidence
  ledger and the matching park record; no shared header blast.

The protected Units gate at assigned base `3be7d0261` is **188/189 before and
after this CTF-only change**: its sole residual is
`_unit_preprocess_node_orientations`. A bisect-quality check identifies the
already-present `profile_rasterizer_stats` declaration added to broadly
included `source/cseries/profile.h` by `beb469942`: removing only that
declaration restores Units 189/189, while the Xbox rasterizer-profile caller
then correctly fails `/W3 /WX` for a missing owner declaration. Merely moving
the declaration to the footer does not restore Units. This CTF branch neither
retains such a trial nor changes that shared header; the base integration issue
requires a separate owner-interface reconciliation rather than hiding it in
this object closeout.
