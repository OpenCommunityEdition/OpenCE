# Independent review: source/physics/collisions (Lane B)

Verdict: `_collision_move_point` ACCEPT_FUZZY_ZERO_CREDIT (NOT exact, zero credit). Overall LAND_PARTIAL.
Landing file: `scratch/lane_b/w/collisions/review/landing.c` (= candidate.c minus one byte-inert declaration-order swap).

## Re-gate (from scratch, reviewer-built objects under review/)
- unitcheck candidate.c: baseline `exact 19 residual 1`, candidate `exact 19 residual 1`, gains=0 regressions=0.
- unitcheck landing.c: identical result, gains=0 regressions=0.
- Guard `--forbid-emitted-symbol _point_from_line3d`: passed (candidate and landing).
- newsyms: 0 newly defined, 0 dropped. OWNERSHIP PASS.
- fake_match_scan: 0 leads on candidate, landing and real source (no new findings).
- Whole-object section sweep base.obj vs landing.obj (86 sections): only `.debug$S` (scratch path) and section 44
  (`_collision_move_point`) differ; defined-symbol delta is internal `$L` label numbering only. No data section moves.
- Not parked (`config/parked.json` has no entry) -> no park entry to refresh.

## Byte ledger (reviewer tool review/bytecmp.py, relocation sites zeroed, 4752 vs 4752, relocs 226/226)
- baseline: 3847 differing bytes / 43 filtered regions.
- candidate = landing: 6 differing bytes (0x595-0x597, 0x59b-0x59d) = ONE transposition of independent instructions
  (`mov edx,[edx+0xc]` vs `mov [ebp-0x50],eax` around `fmul [ebp-0x30]`). No dependency between them -> semantics equal on all paths.

## Strip tests (each hunk reverted ALONE from candidate; differing bytes vs January)
| variant | bytes | result |
|---|---:|---|
| candidate | 6 | - |
| s1 position back at block scope | 12 | scope move buys the 17b/17f preheader load order |
| s2 original decl order (indices, count) | 6 | BYTE-IDENTICAL to candidate -> inert -> plain order lands |
| s3 `= 0` + `[new_clip_count++]` | 18 | explicit constants needed |
| s4 site D one `&&` chain | 3798 | nested `if` needed |
| s5 `last_collision` local restored | 34 | removal needed |
| s6 minimum_k/steepest initialised at declaration | 1469 | late assignment needed |
| s7 no `clip_collision` pointer | 1514 | pointer local needed |
| s8 /Od initialiser chain (`position = *old_position; clipped_position = position; clipped_velocity = velocity`) | 6 | byte-identical to candidate: inert, correctly NOT adopted |

## Double attestation checked by the reviewer (/Od regenerated: review/od_move_point.txt)
- `position`: RTC descriptor names `position` at [ebp-0x18], second local, initialised at entry 0x7A52DC -> function scope is first-party.
  January side is weak (one 2-insn load-order transposition; worker's own filler probe says the key is ~50/50). Accepted because it is a
  scope correction of an EXISTING local, not a new local; no uninitialised read (assigned at the top of the success arm before every use).
- `clip_collision` pointer local: /Od slot [ebp-0xf0] written 0x7A659E, read twice; January forms the struct pointer (`add ecx,eax; fld [ecx+0x18]`),
  and s7 proves the bytes need it. Double-attested.
- no `last_collision`: /Od re-evaluates `clip_collisions[clip_count-1]` twice (0x7A64CA, 0x7A64E4), no slot between -0xe0 and -0xe4. s5 confirms.
- late `minimum_k = 0; steepest = NONE`: /Od 0x7A654B/0x7A6556 after the 0x7A6547 store. s6 confirms.
- `new_clip_count = 1/2/3` + constant subscripts: /Od 0x7A58F6, 0x7A5B0B, 0x7A5CA1 (`imul ..,0` / `shl ..,0` / `shl ..,1` constant indices, no `=0` store). s3 confirms.
- Site D nested `if`: /Od cannot distinguish (all three false jumps -> 0x7A5D0B). Natural C mirroring the enclosing `if (clip_count > 0) { if (...) }`;
  not a decoration; s4 shows it is load-bearing.

## Hunk-by-hunk admissibility
All hunks are plain C inside the one non-exact function; no casts, pragmas, volatile, barriers, raw offsets, dummy locals, self-assignments or
impossible conditions; no baseline-exact function touched; CRLF preserved (1827 CRLF, 0 bare LF); no header/config edits.
Only objection: the `new_clip_count` / `new_clip_indices` declaration swap is byte-inert and cannot be confirmed on January -> stripped in landing.c.

## Notes for the orchestrator
- First-party names are `clip_collisions` / `new_clip_collisions` (RTC + HCEA PDB); the source says `clip_indices` / `new_clip_indices`. Pre-existing, not changed here.
- /Od zeroes `clipped_velocity` via `set_real_vector3d` (0x7A5CE8); worker measured it inert and left the three stores. Unchanged from baseline.
