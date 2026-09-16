# `stack_memory_pool.obj`: opus5 next-150K lane, final wave tierB (2026-09-16)

Scope: one Tier B row from the research campaign, B6 `_stack_memory_pool_allocate` (637 meaningful bytes). The
"do-not-repeat" tag from laws_w2 D was lifted by the F5 RESULTVAR-DEFECT lens.

**Outcome: NOT LANDED. Zero strict bytes.** The production source is unchanged
(`git diff --stat -- source/memory/stack_memory_pool.c` is empty). One evidence-backed shape (S1) fixed the
structural defect that both lenses name. After S1, only register assignment differed, so work stopped under owner
requirement 3.

## Baseline (HEAD 6d6529d98)

The whole-unit gate gives exact 31, residual 2, unwritten 0. The `_point_from_line3d` guard passes. The unit has no
parks.

| row | January padded / meaningful / relocs / sha16 | real file |
| --- | --- | --- |
| `_stack_memory_pool_allocate` | 640 / 637 / 36 / `00ddbe408b35e153` | 640 / 36 / `c49ac9850cc6a1eb`, 231 vs 229 instructions |

## Census

- **Frame.** Both sides use `sub esp,8` and the same slots:
  - `[ebp-4]`: `free_space_in_pool_previous`, address-taken.
  - `[ebp-8]`: the NULL home of `free_space`.
  - `[ebp+8]`, `[ebp+0xc]`, `[ebp+0x10]`: `pool`, `file`, `line`.
  - `frameslot.py` reports FRAME-CLEAN.
- **Relocations.** 36 vs 36, with the same targets, types, addends and order. There is no missing or extra call or
  global. Addresses 0x23–0xe8 sit 2–4 bytes early in our build.
- **Instructions.** Every difference comes from one register fact:
  - **January:** EDI holds `pool` and EBX holds `size`. ESI holds X, a NULL value initialised at entry (`xor esi,esi`
    before `test edi,edi`). X is read exactly once, by `mov eax,esi` on the invalid-size exit, before `pop esi`. So
    MAXLIVE is 3 and, by LAW Z1, there is no whole-function zero pin. January instead uses `test edi,edi`,
    `mov eax,[edi+4]; test eax,eax`, a local `xor eax,eax; cmp ebx,eax`, and EAX for both NULL stores and for the
    FALSE push.
  - **Ours:** ESI is a whole-function zero pin (MAXLIVE 2) and every NULL return is a literal.
  - The NONE-failure `je` binds to January's first literal epilogue. That is a downstream effect of the same fact.
- **Both lenses re-verified at HEAD.**
  - `f5detect.py` still reports RESULTVAR-DEFECT (January `pop edi; mov eax,esi; pop esi`, ours
    `pop edi; pop esi; xor eax,eax`).
  - The instruction count is still 231 vs 229.

## Evidence

- **A42.** An entry `xor` proves a declaration initializer.
- **Exact January functions with the same fingerprint.** `scratch/workers/tB_stack_memory_pool/entryzero_scan.py`
  finds 29 strict-exact January functions with an entry zero that is returned on one exit, and no non-exact ones
  apart from this target. Examples:
  - `rasterizer_memory_pool::_rasterizer_memory_alloc`: `pointer = NULL`, whose final `else` arm falls into the
    shared `return pointer;`.
  - `stack_memory_pool::_pool_new_pointer`.
  - `ui_widget::_ui_widget_load_by_name_or_tag`: the NULL-init web (EDI) is returned by the last `else` arm while the
    success value sits in ESI. This is a split phi.
- **Liveness.** January's X is dead once the three size checks pass. So no valid-size path reaches the X-reading
  return without first redefining the variable:
  - the find-space and NONE failures return literal NULL;
  - the X-reading return is reached only by the invalid-size `else` arm and by the success path.
- **Why the w2 shape failed.** In w2 `smp_v2` (single bottom return reached by every failure path), the NULL def is
  live across the whole valid region. It is rematerialised and the zero pin comes back. This explains that shape's
  identical residual.

## Shapes

**S1.** Candidate `scratch/workers/tB_stack_memory_pool/v1.c`, body `s1_body.txt`.

- **Source changes:**
  - `struct stack_memory_pool_block *block = NULL;`
  - a positive size predicate owns the allocation body;
  - the invalid-size display moves to the final `else` arm, textually last, matching January line 0x3A4;
  - one bottom `return block;` serves both the success path and the invalid arm;
  - the find-space and NONE failures keep `return NULL;`.
- **Measured:** 640 / 36 / sha `147b10dfc82d9604`, still `[sha]`. The whole-unit gate is 31/2/0 and every row
  matches the baseline.
- **Fixed:**
  - the zero pin is gone;
  - the entry region emits January's exact forms;
  - relocations 0–20 land on January's exact addresses (0x23–0x15a).
- **Remaining, all register assignment:**

  | web | ours | January |
  | --- | --- | --- |
  | X | EBX | ESI |
  | `size` / aligned size | ESI, then `lea ebx,[esi+0x20]` | EBX, `add ebx,0x20` in place |
  | `block` | EBX | EDI |
  | extra `pool` reload | into ESI | none |
  | insertion zero | EDI | ESI |

  Our X and `block` share EBX. That is consistent with VC7 merging the NULL def and the success def at the bottom
  phi, where January keeps them apart.
- **Closeness:** aligned-equal instructions fall from 207 (v0) to 168 (v1). S1 is structurally right but further from
  January in bytes. It is not exact, and the current body is complete, so the fuzzy policy keeps it in scratch.

After S1 the values reaching the join already match January's: the NULL init arrives through the invalid arm, `block`
through the success path, and the failures return literals. By Z4, re-spelling a copy or alias does nothing. No
January-side evidence names a different join topology, so no further shape was spent.

## Verification

- The final real-file gate (with the guard) is 31/2/0, and its rows match the baseline exactly.
- `parkcheck` reports no parks for this unit.
- `fake_match_scan` finds 0 leads in both the real file and v1.c.
- `branch_sweep` found 24 unique blobs; the best is the current 31/2/0.
- The owner census is trivial because the real file is unchanged.

## Reopen criterion

Reopen when there is a measured VC7 law that decides whether a result variable's NULL declaration-initializer def
keeps its own web or merges with the success def at a shared bottom return:
- January keeps them apart: X in ESI, `block` in EDI. The exact donor `_ui_widget_load_by_name_or_tag` shows the
  split.
- S1 merges them: both in EBX.

Under such a law, re-gate `scratch/workers/tB_stack_memory_pool/v1.c` (sha256 prefix `c9c67e4407a68d8e`). Do not
reopen with early-return or literal spellings, or with any form that lets the NULL def reach a valid-size failure
exit: w2 `smp_v2` and exp 076 already measure those as non-closing.
