# `thread_win32.obj` — opus5 next-150K wave n1 (2026-09-15)

## Scope and result

Worker n1, lane `C:\halo-worktrees\opus5-150k-house-clean-20260914`, frozen baseline `ae10935da`.
Target row: the PARKED `_get_mutex_from_pool` (64 padded / 59 meaningful / 3 relocations, gate tag
`[reloc-identity]`, park class `csplit-relocation-alias`). Verdict: **SKIPPED-EXHAUSTED**, zero shapes spent.
`source/bungie_net/common/thread_win32.c` was **not modified**.

Gate before and after (with `--forbid-emitted-symbol _point_from_line3d`, guard passed):
`== exact 8  residual 1  unwritten 0`. Park drift: `parks 1 drift 0`. Fake-match scan: 0 leads.
Newly exact: **none**.

## Duplicate prevention

Read in full: `thread_win32_obj.md` (the superseded 9/9 closeout and its rejected four-file port),
`thread_win32_obj_house_rule_correction_20260831.md`,
`legacy_small_donor_exact_reaudit_20260913.md` (line 29), `opus5_250k_house_clean_checkpoint_20260915.md`
section 6.3, the `config/parked.json` entry, and `scratch/w3/laws_w3.md` A45.
`git log --all --oneline -- source/bungie_net/common/thread_win32.c` = 20 commits.

## Census

- **Frame.** None — 22 instructions, leaf, no `sub esp`. Identical on both sides.
- **Instructions.** 22 target / 22 candidate. The normalized section hashes are **already equal**
  (`3b67f561243fc178…` on both sides; objdiff 100.0). Exactly one line differs, and only in its relocation
  owner:

      T  0x19  cmp ecx, 0   rel@0x1b -> _transport_address_string + 36
      O  0x19  cmp ecx, 0   rel@0x1b -> _thread_globals + 1580

- **Relocations.** 3 target / 3 candidate, at the same addresses (5, 27, 40) and of the same type (0x0006).
  Rows 1 and 3 (`_thread_globals+300`, `_thread_globals+264`) are identical. Row 2 differs only in the owner
  spelling; both spellings resolve to image address 3263828, because `_thread_globals` is 1544 bytes at
  3262248 and +1580 is 36 bytes past its end, inside `_transport_address_string` at 3263792.

## Why strict exact is unreachable

The loop bound is the natural one-past-the-end expression
`&thread_globals.mutex_references[MAXIMUM_MUTEX_REFERENCES].in_use`. csplit anchors a relocation to the image
symbol that **contains** the destination, so on the target side it must name the following object; VC7 names
the object the C expression actually refers to. laws_w3 **A45** states this class directly: a one-past-the-end
bound spelled as the neighbouring symbol "makes strict exact unreachable and cannot land as an already-written
residual", and only the resolved-equal **reclass** is admissible — which this park already carries, for zero
strict credit.

The only source form that emits January's spelling is the rejected
`extern struct mutex_reference transport_address_string;` — a false declaration giving another translation
unit's 256-byte `char[256]` string buffer a mutex type, then walking `(long)&transport_address_string.in_use`
as the pool bound. `branch_sweep.py thread_win32` still surfaces that historical blob as
`>>> BEST: history:253d8146  exact 9 residual 0`; `git cat-file -p 253d8146` confirms line 100 is that extern
and line 297 is `while ((long)in_use<(long)&transport_address_string.in_use);`. It has been rejected three
times — the house-rule correction of 2026-08-31, the legacy small-donor re-audit of 2026-09-13, and the 250K
checkpoint history sweep of 2026-09-15. Re-landing it would be a fake match, so it was not built.

## Disposition and reopen criterion

`_get_mutex_from_pool` remains parked at 64 / 3 / `3b67f561243fc178`, with bytes already equal to January.
The park is at its truthful terminal state and should stay `csplit-relocation-alias`.

There is **no** reopen criterion this campaign can satisfy from source. The only route by which this row could
ever be credited is a tooling/admission change — scoring the resolved destination rather than the owner
spelling, as `tools/parked_functions.py` already does for the accepted A45 reclass — which is an
orchestrator/owner decision and is worth 0 strict bytes either way. Recommendation: do not assign this row to
another wave.

Worker notes: `scratch/workers/n1_key_agreement_prime_numbers_etc.md`.
