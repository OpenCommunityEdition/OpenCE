# VC7 13.00.9254 laws measured in the next-150K lane (2026-09-16)

Produced by a three-angle probe against `_hs_compile_and_evaluate` (source/hs/hs.obj). The probe closed no function, but
it settled a question that four earlier waves had classified as an unreachable "register-allocation tie". Evidence lives
in `scratch/hs_pin/` (angle1.md, angle2.md, angle3.md, the scan and lab scripts, and ~700 lab probes). Nothing under
`source/`, `config/`, `tools/` or `build/` was modified by the probe.

## LAW Z — the whole-function constant-zero register pin

**Statement.** VC7 keeps constant 0 in a callee-saved register for a whole function iff both conditions hold.

- **Z1 AVAILABILITY.** EBX is the only byte-addressable callee-saved register on x86 (ESI/EDI have no 8-bit alias), so a
  zero pin that serves any byte store is always a **two-way contest for EBX**, never a free choice among three
  registers. Genuine call-crossing webs are allocated esi -> edi -> ebx and the pin takes what is left; 178 of 214
  parsed pins are EBX. Measured hard ceiling across 236 both-pin functions: **no January function carries three genuine
  callee-saved webs and a pin**.
- **Z2 DEMAND.** Only zero **stores** seed and grow the web. Zero call arguments (`push 0`), NULL arguments and zero
  comparisons neither seed it nor count, though once the web exists it is spent on them (which is why January appears to
  have 25-28 "uses"). Stores of different widths unify into one web. Boundary condition, measured identically at 3 and 4
  competitors: **stores(zero) >= 2 x uses(strongest interfering competitor) + 1** — U=2 needs 5 stores, U=3 needs 7,
  U=4 needs 9, U=5 needs 11, U=6 needs 13, U=8 needs more than 16. Extra basic blocks raise the threshold.
- **Z3 INTERFERENCE, NOT ORDINAL POSITION.** What competes is the **span** from a competitor's definition to its last
  use. A competitor whose uses are entirely before or after the zero stores does not compete at all (the pin then
  appears at one store); a one-use competitor never competes. The zero variable's own seed position is **inert**
  (declaration, before the first call, mid-body and last statement are byte-identical), and whether it is the return
  value is inert. This **refutes** the "seeded flag" hypothesis carried in earlier ledgers.

**Why this matters more than one function.** Corpus cross-tab over the 572 paired objects
(`scratch/hs_pin/scan_zeropin2.py`, `zeropin_cross.json`):

| cell | >= 1 use | >= 5 uses |
| --- | --- | --- |
| January pins AND we pin | 236, of which 227 strict EXACT | 97, of which 91 EXACT |
| January pins AND we do NOT | 4, of which 0 EXACT | 5, of which 0 EXACT |
| we pin AND January does not | **0** | **0** |

Our compiler never invents a pin January lacks. The pin is therefore **fully source-determined**, and reproducing it is
sufficient for the whole function in 227 of 236 cases. **Consequence for triage: a function where January pins and we do
not is not a class-D register tie. It is a SOURCE defect** — a surplus long-lived value (an extra local, alias, carried
copy, or a statement lowered into the pinned register), i.e. the A5/R16 family — and Z1 proves January's own source had
at most two long-lived call-crossing values.

**The complete negative set** (January pins, we do not) is five functions, two of them in protected files:

| function | January | ours | bytes |
| --- | --- | --- | --- |
| `hs::_hs_compile_and_evaluate` | ebx pin (25) + edi, esi | no pin + ebx, edi, esi | 592 / 608 |
| `game_engine_king::_king_calculate_hill_state` | ebx pin (15) + edi, esi | no pin + ebx, edi, esi | 448 / 448 |
| `bink_playback::_bink_alloc@4` | esi pin (5) + edi | no pin + ebx, edi, esi | 496 / 512 |
| `transport_endpoint_set_winsock::_transport_initialize` (PROTECTED, evidence only) | ebx pin (9) + edi, esi | no pin + edi, esi (EBX free) | 416 / 400 |
| `transport_endpoint_winsock::_endpoint_readable` (PROTECTED, evidence only) | ebx pin (5) + esi | no pin + esi (EBX free) | 192 / 176 |

`_transport_initialize` proves Z2 independently: EBX is **free** in our build and we still do not pin, because January
emits three extra stores through the pinned register (`mov byte [ebp-0xb],bl`, `[ebp-5]`, `[ebp-4]`) for fields a
preceding wide zeroing already cleared. Deleting three zero-valued source statements destroys a nine-use pin.

**Source-side exemplars of the contest being WON** (from `paramhome.py`: 649 January functions write a parameter home;
533 are strict exact in our build and all 533 reproduce the write, so the idiom is ordinary, not January-only):
`libs/libcmt/wcstok.c:31 _wcstok` (verbatim-known source: a pointer parameter conditionally reassigned and walked, which
January spills back to `mov [ebp+8],ecx` rather than giving it a callee-saved register, leaving EBX for the zero);
`antenna.c:124 _antenna_new` (VC7 **spills a genuine variable** to keep the pin when the pin has enough dword-width
uses); `cinematics.c:512 _draw_quad` (three parameter-home writes although the source never assigns the parameter).

**Never steerable.** Z1 counts long-lived source values and Z2 counts zero-valued source statements, so a pin can only
be obtained by adding or removing real program text. A pin bought with a carrier variable, a redundant `= 0`, `volatile`
or an operand permutation is a fake match by construction.

## LOOP-HEAD ALIGNMENT LAW

A loop head is padded to the next 16-byte boundary **iff the distance from its natural (unpadded) address to that
boundary is 1..8**; at distance 9..15 VC7 emits nothing. The pad is a single nop-instruction of the exact width where
one exists (1 `nop`, 2 `mov edi,edi`, 3 `lea ecx,[ecx]`, 4 `lea esp,[esp]`, 6 `lea ebx,[ebx]`, 7-byte `lea esp,[esp]`),
and for the two widths with no single encoding it emits `jmp SHORT <head>` plus a nop (distance 5 -> jmp + `lea
ecx,[ecx]`; distance 8 -> jmp + `lea ebx,[ebx]`). Loop body size, back-edge distance and early-exit block size are all
inert. **Detector:** `(-natural_head_address) mod 16`; 1..8 pads, 9..15 does not.

In `_hs_compile_and_evaluate` our natural head is at 0x58 (distance 8) so VC7 emits `jmp` + `lea ebx,[ebx]` = 8 bytes;
January's is at 0x55 (distance 11) and emits nothing. Since the decision reads only the head address, and every byte of
our 3-byte-longer pre-loop region is a consequence of the missing pinned zero, **the pad cannot be removed by any source
change independent of the EBX contest**. Refinement the n1 ledger does not state: only ONE pre-loop byte is needed to
reach distance 9, so the first zero-register site recovered is worth **9 bytes, not 1**.

## `_hs_compile_and_evaluate`: disposition

**NOT LANDED. Still 592 padded / 601 meaningful in the real file, unchanged from the baseline.**

The best candidate (`scratch/hs_pin/v1.c`) reaches **592 padded / 585 meaningful / 40 relocations, `[sha]` only** — down
from the baseline's `[size 608!=592, sha]` — with the whole unit still at 446 exact and the guard passing. It is 8
meaningful bytes over January and therefore carries zero credit, and the lane's fuzzy policy forbids a non-exact body
replacing a complete one, so it stays in scratch. It does contain one genuine reconstruction finding the n1 ledger does
not record: January nests the global-lookup test on the `!= NONE` arm (proven by January's block order at 0xb4, where
the `if (space) type = 2;` arm is the fall-through).

Measured and refuted this probe:
- **Parameter reuse** (drop the `source` local, reassign `expression`) is January's shape and is confirmed by Ghidra,
  but it **strengthens the competing web** and loses the pin (608, no pin). It lowers the store threshold from 15 to 9.
- **`source` assigned in every switch arm** gives the local a real frame slot and displaces both error-output slots
  (608 / 41 relocations, strictly worse).
- **The residual is exactly one zero store.** With parameter reuse the threshold is 9 and our source supplies 8. A
  census of January's own pin-sourced stores (`[ebp-1]`, `[ebp-0xd]`, `[ebp-0x40c]`, `[esi]`, `[eax]`,
  `hs_syntax_data_allocated`, `hs_syntax_data`, `hs_recompile_pending`) returns **exactly the same eight stores we
  already write, at the same positions and loop depths**. There is no legitimate ninth statement to copy, and the worker
  correctly refused to buy the store with a redundant `= 0` or a carrier.
- With an instrument store standing in for that ninth zero, the body measures **577 meaningful = January's exact size**
  with 40/40 relocations, both sprintf arms cross-jumping onto a shared `lea ecx,[expanded]; push ecx; call _sprintf`
  and `mov [ebp+8],edx` homing the phi. The whole front end closes; only the net-zero-byte error-tail packet survives.
- 20+ house-rule-clean spellings on the parameter-reuse base (declaration order, error-output and `expanded` scoping,
  declare-and-initialize forms, while/for/do-while, explicit NULL comparisons, ternary vs if/else, four newline-block
  spellings) are all 608 with no pin; the same spellings on a pin-winning base are byte-identical, confirming the
  ESI/EDI caching and the slot mirror are a genuine tie.

**Reopen criterion (replaces the n1 one).** Either recover a ninth zero-valued statement that January's source
provably contains (the eight known ones are already written, so this requires new January-side evidence), or find a
source form that removes one use from the strongest interfering competitor, which by Z2's boundary drops the threshold
to 7 and wins the pin with the eight stores we already have.

## Do-not-repeat additions

- Do not re-spend on the `_hs_compile_and_evaluate` loop respellings, scoping variants, declaration order, parameter
  reuse alone, or any "seeded flag" placement: Z3 proves seed position is inert and the 20+ spelling screen is measured.
- Do not attempt to win a pin with a redundant zero store, a carrier variable, `volatile`, or an operand permutation.
- The loop-alignment pad is not independently removable; do not hunt it as a separate defect.
