# Campaign throughput lessons (2026-08-30 / 2026-08-31)

Operational lessons from a run that took the board from **669,879 to 714,807
matched bytes (+44,928)** and **4,829 to 4,973 credited functions (+144)** with
**zero regressions**, verified by a full build plus a per-function sweep of all
569 built units.

This document is about *throughput and hazards* - how to close many functions
in parallel without breaking what already works. For per-function matching
technique see `matching_methodology.md` and
`exact_match_acceleration_playbook.md`.

---

## 1. Write unwritten functions before fighting residuals

This is the single highest-leverage finding. Measured across four waves:

| wave | scope | approach | bytes gained | yield |
| --- | --- | --- | --- | --- |
| 1 | 71,112 B residual | close residuals | 11,040 | 16% |
| 3 | 27,920 B "sha-only" residual | close residuals | 3,168 | 11% |
| 4 | ~160,000 B unwritten | write missing functions | **30,720** | far larger scope |

Wave 3 deliberately picked the *closest* residuals on the board - functions
whose padded size **and** relocation count already matched, so only instruction
selection differed - and still returned only 11%. Those residuals are typically
a single register-allocation or x87 stack-discipline decision, and they resist
source-level control.

An **unwritten** function has no wrong prior to undo. You read the target
disassembly and write natural C89. That is a fundamentally easier problem.

The board currently holds roughly **1,000,000 bytes of unwritten functions**
against **100,000 bytes of residual**. Work the unwritten pool first.

Within a unit, start with the **small** functions. Eight 300-byte closures beat
one failed heroic attempt at a 3,000-byte function.

## 2. Parallel fan-out architecture

`tools/campaign/gate.py <unit>` compiles **one** translation unit to a
PID-named object under `scratch/` and never writes to `build/`. That makes it
safe to run from many workers simultaneously.

The architecture that worked:

- **One worker per `.c` file, files strictly disjoint.** Never two workers in
  one file.
- Every worker measures **only** with `gate.py`. Useful flags: `--fn <FUNC>`,
  `--disas <FUNC>`, `--source <FILE>` (compile a variant without touching the
  real file), `--edits <json>`.
- **No worker runs `ninja` or `configure.py`.** A shared build directory cannot
  take concurrent writers.
- **No worker commits.** The orchestrator does one full build, one board
  measurement, and one commit at the end.
- Workers record `gate.py`'s exact count *before* starting and revert anything
  that lowers it.

### `--disas` hides insert/delete holes

`gate.py --disas` is an **index-aligned zip**, not an alignment diff. Once
instruction counts diverge, every later row reads as different and a genuinely
inserted or deleted block is invisible. Use `scratch/alndiff.py` (difflib
SequenceMatcher over normalized capstone streams, branch targets normalized):

    python scratch/build1.py <unit>.c scratch/cur.obj
    python scratch/alndiff.py <unit> <FUNC> scratch/cur.obj

A 48-instruction hole in `_ai_debug_render_actor` was only visible this way.

Ignore `.rdata` / `__real@...` annotation mismatches in that output - those are
symbol-naming artifacts, not code differences.

## 3. Header edits perturb objects that never name the added types

**"Edit only your own object's header" is not a safe rule.**

`source/items/weapons.h` is included by ten other `.c` files. Adding two
interface-state structs to it silently dropped `units.obj` from **189/189 to
188/189**, breaking `_unit_preprocess_node_orientations` - a function that
names none of those types and calls none of the new functions.

Bisecting the three added blocks:

| added to weapons.h | units.obj |
| --- | --- |
| enum only | 189 ok |
| structs only | **188 BREAKS** |
| protos only | 189 ok |
| enum + structs | 189 ok |
| structs + protos | **188 BREAKS** |
| all three | **188 BREAKS** |

Adding the enum *in front of* the structs repairs it. The effect is not
semantic - it is the C2 allocator's **definition-position** sensitivity. What
perturbs codegen is where declarations sit, not what they mean.

### Rules that follow

1. Before touching any `.h`, run
   `grep -rl "<header filename>" source/ --include=*.c`. If anything else
   includes it, default to putting the declaration in the `.c`.
2. Definitions private to one TU belong in that TU. The weapons fix: structs
   moved into `weapons.c`; the header kept the enum, the prototypes, and a
   forward declaration so the prototype's struct tag has file scope. `units`
   returned to 189/189 with all 50 of weapons' exact functions intact.
3. **Never** "fix" this by relying on a coincidental ordering that happens to
   restore the victim.
4. If a header edit is genuinely required, prove it: build **shadow header
   trees** (patched and pristine), compile every dependent unit against both,
   and compare per-function size, relocation count and normalized hash. This
   was done to drop `__declspec(align(4))` from `struct guard_state_data` in
   `actions.h` (24 includers) and it held up under independent re-check.

### Some cases have no local remedy

Adding two trailing fields to `struct _projectile_datum` in `projectiles.h` -
fields January demonstrably uses at composite offsets 0x1FC and 0x200 - drops
`units.obj` to 188/189. The type is public, so internalising it in the `.c`
does not apply. About 1,152 bytes of projectile work is gated on this. That is
an owner decision; do not trade away a complete object for it quietly.

## 4. Verification discipline

**Worker self-reports are systematically optimistic.** One wave's rolled-up
"confirmed bytes" read 18,544 against 3,168 actually closed, because verifiers
counted pre-existing exact functions alongside new ones.

The only numbers that count:

1. A full `ninja` build. Its semantic gate fails closed if any previously exact
   function stops matching.
2. `python tools/campaign/board.py` for the byte and function totals.
3. A **per-function regression sweep**: snapshot the exact set of every unit
   from `build/split` vs `build/base` before the run, and re-compare after.
   Object-level counts hide one object lost against another gained.

`board.py`'s per-object listing is **truncated** - an object appearing in or
disappearing from it is not evidence of a change. Use the sweep.

## 5. Cross-cutting codegen facts confirmed this run

- **Frame size is structural.** A differing `sub esp, N` means a local's scope
  or type is wrong. It is never scheduling noise. This is what reopened
  `_generate_prime_numbers` after a prior session had declared evidence
  exhaustion on it (81.61% -> 91.15%).
- **Local declaration order is inert** for stack-slot assignment - proven
  byte-identical across four permutations. Do not spend waves permuting
  declarations.
- **The zero-register pin.** January frequently keeps a zero live in a register
  and spends it repeatedly (`mov [mem],reg`, `push reg`, `cmp reg,zeroreg`)
  where our build rematerialises immediates. Pinning a zero consumes a
  callee-saved register, so this surfaces *downstream* as stack-slot
  permutations and duplicated or merged blocks rather than as a local byte
  difference. It is not a compiler flag: on `_lra_allocate`, `/O1`, `/O2 /Os`
  and `/O1 /Ob2` all shrink the function to 319 bytes and `/Ox /Os` explodes it
  to 1255, while the campaign flags give the closest result at 368.
- **Shared-header inlines must stay non-`static`.** Converting the 89
  `__inline` definitions in `real_math.h` (plus 6 elsewhere) to
  `static __inline` costs **21 objects and 119 functions**: `static` lets VC7
  dead-code the out-of-line copy and inline more aggressively, changing call
  sites. This also kills the theory that our ~20-28 surplus COMDATs per object
  come from storage class - non-static is what reproduces January's bytes, so
  the surplus-COMDAT question remains open and is a whole-tree property, not
  something an individual object's reconstruction introduced.

## 6. Guards against fake matching

A byte match bought with source Bungie would not have written is worth less
than an honest residual. Rejected on sight: `volatile`, `register`, `#pragma`,
barriers, compiler-flag changes, raw numeric struct offsets, type punning or
representation casts, undefined behaviour, byte forcing, and logic that is
nonsensical for the function's stated job and only coincidentally matches.

Three live examples from this run, all correctly refused:

- The only shape that narrowed `_ai_debug_render_actor` required
  `real_vector3d *` to `real_point3d *` casts to reach `point_from_line3d`.
  That is representation punning, so it was reverted - and re-reading the
  numbers, it only reached 24,864 of 24,976 bytes anyway. It was never a
  closure, just a 16-byte improvement on a 128-byte gap.
- Writing an early return as `*num_primes = i` to deliberately spend a live
  zero is a coincidence generator, not a reconstruction. XDK 3911 folds it to
  identical code and it was already on a do-not-repeat list.
- A `vehicle_update` shape that forced an increment into both arms of a clamp
  using two invented file statics measured the right size only because the
  shortfall was cancelled downstream by alignment padding. The real shape is
  the `cseries.h` `MIN` macro with the constant on the left and no local
  increment at all.

Inline assembly is allowed **sparingly** and **unit-locally** in small helper
and math functions. The admitted case is `fast_ftol` (`fld` / `fistp`), because
`/QIfist` lowers every C float-to-int conversion through a 64-bit `fistp qword`
and a 32-bit inline `fistp` is never compiler output. Keep it `static __inline`
in the `.c` that needs it - never in a shared header.

## 7. Reusable scratch tooling

Left in `scratch/` by this run (note that `scratch/` is per-worktree and is not
committed, so these exist only in the worktree that produced them):

| file | purpose |
| --- | --- |
| `alndiff.py` | true alignment diff over normalized capstone streams |
| `build1.py` | compile one unit with the real build flags to a chosen `.obj` |
| `cs_build.py`, `cs_gate.py`, `cs_blast2.py` | shadow-header-tree blast-radius harness |
| `pn_score.py` | score source variants by byte divergence and first-diff offset |
| `pn_try.py` | batch-apply source variants through `gate.py` and report |

Use the committed `tools/fake_match_scan.py` for fake-match review; unlike the
scratch helpers above, it is available in every checkout.

A caution learned twice the hard way: **Python heredocs in this environment eat
one level of backslashes.** Build Windows path separators with `chr(92)` rather
than writing them literally, or you will silently emit a single-backslash
`__FILE__` string where the source needs a doubled one and break relocation
identity against the target's string literal.
