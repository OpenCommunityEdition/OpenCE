# `prime_numbers.obj` — opus5 next-150K wave n1 (2026-09-15)

## Scope and result

Worker n1, lane `C:\halo-worktrees\opus5-150k-house-clean-20260914`, frozen baseline `ae10935da`.
Target row: the PARKED `_primegen` (352 padded / 340 meaningful / 12 relocations,
class `tu-context-optimization`, objdiff 81.614815). Verdict: **SKIPPED-EXHAUSTED**, zero shapes spent.
`source/bungie_net/common/prime_numbers.c` was **not modified**.

Gate before and after (with `--forbid-emitted-symbol _point_from_line3d`, guard passed):
`== exact 3  residual 1  unwritten 0`. Park drift: `parks 1 drift 0`. Fake-match scan: 0 leads.
Newly exact: **none**.

## Duplicate prevention

Read in full: `prime_numbers_obj.md` (families E17–E27 plus the earlier count-model, loop-shape and
`prime-numbers-final` families, with their do-not-repeat entries),
`prime_numbers_obj_jonas_credible_fuzzy_park_20260830.md`, and
`prime_numbers_obj_jonas_stian_sieve_lifetime_fixed_point_20260830.md` (the Stian/Pastudan donor packet and
its explicit "do not repeat this cursor/reused-index family").
`git log --all --oneline -- source/bungie_net/common/prime_numbers.c` = 16 commits, all predating the ledgers.
`branch_sweep.py prime_numbers` = 12 unique blobs; the best historical state is `exact 3 residual 1`, equal to
HEAD, so there is no historical body to mine.

## Census (fresh; the prior ledgers record this only in prose)

### Frame — structural, and the gate tag `[sha]` understates it

January `sub esp,0xc` (three dword cells); ours `sub esp,8` (two).

| cell | January holds | refs | ours |
| --- | --- | ---: | --- |
| `[ebp-4]` | scan index, zeroed at **entry** (`mov [ebp-4],edi` @0x1c, before the assert) | 4 | same cell, but it also carries the sieve countdown |
| `[ebp-8]` | the fill value (`mov dword [ebp-8],3` @0x83), then the sieve countdown (@0xd4, @0x101, @0x10c) | 6 | **absent** — the fill value stays in ECX (`mov ecx,3` @0x99) and never gets a home |
| `[ebp-0xc]` | `total_count` (@0x5b store, @0x111 load) | 2 | lives at `[ebp-8]` |
| `[ebp+8]` | the `maximum` **parameter home** reused for `m` (@0xcf, @0xfe, @0x109) | — | identical in ours |

So the candidate is exactly one memory lifetime short. January makes the fill value memory-resident — it
loads it at the loop top and stores it at the bottom on every iteration — and packs the sieve countdown into
that same cell, while our build enregisters the fill value and packs the countdown with the scan index.
laws_w3 A24 (parameter-home offer) fires identically on both sides, so there is no parameter-home gap.

### Relocations

12 target / 12 candidate; every type, owner and addend identical. Eight of the twelve **addresses** drift
(target 0057, 006a, 008e, 0095, 0115, 0125, 0138, 0142 against candidate 005b, 006f, 008c, 0093, 0113, 0123,
0136, 0140) purely as a consequence of the code-size shuffle around the frame and zero-pin hunks. No call and
no global is missing or added.

### Remaining instruction hunks

The zero-register pin. January pins `EDI = 0` (`xor edi,edi` @0x18) and spends it three times — `mov [esi],edi`
for `*num_primes = 0`, `push edi` for the `match_malloc` `FALSE`, and `cmp esi,edi` for the allocation test —
where our build rematerialises `0` and uses `test esi,esi`. This is the recurring zero-register-pinning
mechanism; family E20 measured it directly (VC7 proves the index is zero and rematerialises immediates).

### Line anchors

`push 0x3d` = line 61 (assert), `push 0x47` = 71 (`match_malloc`), `push 0x75` = 117 (`match_realloc`). All
three match the literals in the retained source, so there is no reconstruction gap in the spans between them.

## Why no shape was spent

Every remaining difference is already a measured negative in the inherited corpus:

- E17 and E18 already reach January's 12-byte frame **and** the `EBX=odd_count` / `ESI=num_primes` / `EDI=i`
  register plan using the fill-value-becomes-countdown lifetime mapping. Their only residue is that the two
  cells are coloured the other way round (fill at `-4`, scan at `-8`) and that the `= 3` store lands before
  the assert instead of at +0x83.
- E19 refutes assignment-order permutations as the colouring control.
- E21–E26 refute aggregates, constant-index local arrays, field-address aliases and whole-object aliases —
  VC7 scalar-replaces all of them.
- E27 refutes signedness; the ledger records declaration, spelling and hint changes as neutral.
- The Stian donor family (pointer cursor plus reused indices) is an explicit do-not-repeat.

No w2/w3 law fires. A24 is already satisfied identically on both sides. A25 — the one new law that produces
exactly a frame `8 → 0xc` growth by spilling a variable — requires the same store at the end of both arms of
an `if/else`, and this function's fill loop contains no `if/else` at all. A6 (nested-block packing) and A38
(initializer placement) cover the ground E17–E22 already measured.

## Disposition and reopen criterion

`_primegen` remains parked at 352 / 12 / `66e1c7859e354a6b`, unchanged. The residual is frame-slot
**colouring** (two equally sized cells exchanged) on top of the zero-register pin — both non-source mechanisms
at XDK 3911 CL 13.00.9254.1.

The park's reopen criterion (authoritative local-variable provenance, an original-source donor with the same
simultaneous allocation, or a verified compatible compiler/QFE) is **not** met and is unchanged. Practical
guidance for a future attempt: start from E18, which already has the correct frame and register plan, and
attack only the `-4`/`-8` colouring.

Unverified hypothesis banked for a future wave: January's three cells are ordered `-4`, `-8`, `-0xc` by
**last use** (scan @0xc3, fill/countdown @0x10c, `total_count` @0x111). That ordering is neither reference
count (4, 6, 2) nor first-definition order (0x1c, 0x83, 0x5b). If the rule is confirmed on other units it
predicts which source lifetime has to end first, which is the missing control here.

Worker notes: `scratch/workers/n1_key_agreement_prime_numbers_etc.md`.
