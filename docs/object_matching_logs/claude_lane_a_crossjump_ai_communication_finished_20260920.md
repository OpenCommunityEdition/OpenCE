# Lane A - the `_ai_communication_finished` cross-jump, chased to the mechanism

Worktree `C:\halo-worktrees\claude-lane-a-ai-core-20260920`, branch
`claude/lane-a-ai-core-20260920`, baseline
`6130661c9585fdb2785b3ff6bdb2ad0cfef0520c`.

**OWNER DIRECTIVE, 2026-09-20:** *"chase the cross-jump on
`_ai_communication_finished`."*

## Verdict

**The cross-jump is not reachable from source, and the current reconstruction is
the floor.** Nineteen source shapes were compiled and gated across five
independent hypotheses; every one is equal to or worse than the shipped body on
the ordered key (padded size -> 1584, relocations -> 87, instructions -> 484).
Nothing is landed. `source/ai/ai_communication.c` is unchanged by this chase.

What the chase did produce is a **decoded mechanism and a corrected arithmetic**,
both of which contradict what this lane previously recorded. Those corrections
matter more than the failed shapes, because the old numbers would have sent the
next lane after a target that does not exist.

## Correction 1 - the gap is 18 bytes and 4 instructions, not 16 and 2

`gate.py` and `divcount.py` both count trailing padding. January's real code ends
at 0x62f (1583 bytes) with one pad NOP; ours ends at 0x61d (1565 bytes) with
three. The section sizes 1584 and 1568 differ by 16, and the instruction counts
484 and 482 differ by 2, but the **real** gap is **+18 bytes and +4
instructions**.

**Consequence, and this is a trap worth stating loudly: a bare `1584` from this
function is not a size match.** Closing only the register half below adds 6 real
bytes, giving 1571 real bytes, which pads up to a section size of exactly 1584
while twelve code bytes are still missing - and `divcount` would simultaneously
report roughly 495 instructions because the padding grew to 13 NOPs. Read real
code length, not the section size, on this function.

## Correction 2 - the gap has FOUR parts, not two

This lane previously recorded the gap as two coupled decisions (the cross-jump
and an EBX/EDI swap) worth 12 and 6 bytes. That is wrong and, taken alone, it
**overshoots**: an agent closing only those two lands at 1587 real bytes.

Measured January-minus-ours, region by region, by relocation-site offset deltas
(`scratch/orch/relsites.py`) cross-checked against `alndiff.py`:

| region | delta | what |
|---|---:|---|
| 0x000 .. 0x1e9 | 0 | identical, instruction for instruction |
| 0x1e9 .. `call _datum_get` | **+21 B, +6 insn, +1 reloc** | (a) the cross-jump **and** (b) the EBX/EDI assignment |
| `call _datum_get` .. `call eax` | **+4 B, 0 insn** | (d) one conditional jump that is near in January and short in ours |
| `call eax` .. end | **-7 B, -2 insn** | (c) a debug-string tail-merge January has and we do not |

`0 + 21 + 4 - 7 = +18` bytes and `0 + 6 + 0 - 2 = +4` instructions. The books
close exactly. Two of the four parts were previously unrecorded, and one of them
runs the **opposite** way: in the `"filter "` block **our** code is seven bytes
longer than January's.

Anchor offsets, independently re-derived here: `_object_get_and_verify_type`
sits at January 0x2f3 / ours 0x2de (delta -0x15 = -21);
`_get_global_random_seed_address` at 0x32e / 0x315 (delta -0x19 = -25, so four
more bytes are lost between them); the final `_csstrcat` at 0x623 / 0x611
(delta -0x12 = -18, so seven are regained).

## The single relocation

Across the whole function the relocation multisets are identical except for one
row:

    _ai_communication_find_global_actor_to_talk    January 2 (0x216, 0x2c3)   ours 1 (0x2a5)

`_actor_data` is 3 on both sides, `_datum_get` is 2 on both sides, and every
other symbol matches exactly. **The entire relocation deficit is one missing
call site**, and both builds emit exactly two resolution blocks - they just
choose different pairs to merge.

## (a) + (b) - what actually happens, from the bytes

January and we both lower the dispatch to the same compare chain
(`movsx eax,[esi+4] / sub eax,2 / je / dec / je / dec / jne`), and both lay the
arms out enemy-first, then target, then friend. The two builds then diverge only
in **which two blocks VC7's cross-jumper merges**:

- **January** merges the *friend* and *preselected* resolutions into one block at
  0x2d3 (`mov ecx,[_actor_data]; push eax; push ecx; call _datum_get`), entered
  by fall-through from the friend arm and by `jne 0x2d3` from the preselected
  test. The *enemy* arm keeps a private copy in a different scratch register
  (`push eax; mov eax,[_actor_data]; push eax; jmp 0x2db`) and cross-jumps only
  into the shared `call _datum_get`. Because the two arms' code after
  `call find_global` therefore differs, the two `call` sites survive.
- **We** merge the *enemy* and *friend* arms instead, all the way back through
  `call find_global` (our enemy arm is just `... push 2; jmp 0x2a4`), and leave
  the preselected resolution private.

Two call sites whose entire suffix is shared cannot emit different suffixes, so
the merge choice and the missing call site are **one decision, not two**. The
scratch-register choice that decides it - `push eax; mov eax,[_actor_data];
push eax` (7 bytes, using the 5-byte `A1` moffs32 form) versus
`mov ecx,[_actor_data]; push eax; push ecx` (8 bytes) - is assigned per block by
the backend and was not reachable from any spelling tried.

**The EBX/EDI swap is the same decision, not a second one.** When one probe
(`scratch/xj/enemy-private/cand_1.c`) gave the enemy arm its own resolution, the
register assignment flipped to January's for free: `mov ebx,[ebp+8]` at 0x9f,
`xor edi,edi; mov di,[_communication_speech_priorities+eax*2]` at 0xdb,
`mov [ebp-0x14],edi` at 0xf0, and all three `mov edi,ebx` repairs at
0x20d / 0x280 / 0x2bb against January's 0x20d / 0x27f / 0x2ba. Over 0x0..0x226
that shape is 165 instructions against January's 165, same opcodes throughout.
It still fails, by one surplus resolution block.

### Why our register assignment is the one that makes sense

Our build binds `unit_index` to EDI because **EDI is where the callees want it**.
`_ai_communication_find_global_actor_to_talk` reads its subject unit index from
EDI (`cmp edi,esi` at its 0xa, `push edi` at its 0x1c);
`_ai_communication_find_specific_actor_to_talk` reads it from EDI (`cmp edi,-1`
at its 0x1f, `push edi` at its 0x28); `_ai_communication_look_secondary_at_unit`
takes it in EDI as well. All three callees are **byte-exact in our build**, so
their private conventions are fixed and identical to January's. Our allocator
honours those three coalescing edges; January's did not, and paid three
`mov edi, ebx` repairs to fix it up afterwards. Nothing in C expresses "decline
this coalescing", and the only constructs that would are exactly the artificial
perturbations this campaign refuses.

## (c) and (d) - the two newly found parts, same mechanism

Both are also scratch-register-driven merge decisions, and both are recorded
here so nobody spends a wave calling them source bugs.

**(c) the `"filter "` append, -7 bytes for January.** January emits
`push offset "filter "; jmp 0x4e2` and cross-jumps into the `"nobody "` block's
`lea edx,[ebp-0x450]; push edx; call _csstrcat` trampoline. We emit
`push offset "filter "; lea eax,[ebp-0x450]; push eax; jmp 0x4d7` and merge only
the `call`, because we chose EAX where the shared block uses EDX. The
`"playercant "`, `"nobody "` and `"rand-failed "` blocks are byte-identical
between the builds; only this one differs, and only in the `lea` register.

**(d) the 0-chance / rand-failed placement, +4 bytes for January.** Same source,
different block placement. We emit `then(rand print) -> jmp -> else(0-chance) ->
join(rand-failed)`; January emits `then(rand print) -> join(rand-failed) -> ...
-> else(0-chance) -> jmp back`. Both pay exactly one 2-byte jump, so the
placement itself is byte-neutral. The four bytes come from the consequence:
January's `chance > 0.0f` test must reach an else arm 0x95 bytes away and so
needs a 6-byte near `jne 0x3bc`, where ours reaches it in 0x75 and uses a 2-byte
short `jne 0x387`. The decoded else-arm sink rule in the campaign notes does not
apply - neither of its triggers is present.

## Hypotheses tested and refuted

Five hypotheses, nineteen compiled and gated shapes. Floor = 1568 / 86 / 482,
function-section sha1 `a8fcfe6782e01ed0`, 72 differing regions. No shape in any
family lost a sibling: the unit census stayed `exact 44 residual 4 unwritten 0`
throughout, and `fake_match_scan` reported zero findings on every nominated
shape.

**H2 - give the enemy arm its own resolution (4 shapes).** PARTIALLY SUPPORTED
as a mechanism, refuted as an optimization.

- enemy arm owns a block-scoped `long enemy_actor_index` and resolves it:
  1600 / 88 / 493, 55 regions. Restores both `find_global` call sites at
  January's own offsets and flips EBX/EDI for free, but emits **three**
  resolution blocks where January emits two (`_actor_data` 4 vs 3).
- the same, plus `long reply_actor_index = preselected_reply_actor_index;`
  hoisted and the `else` deleted: 1648 / 88 / 503, 91 regions. **Worst of
  everything** - it spills two frame slots (`mov [ebp-8],0xffffffff` and
  `mov [ebp-0xc],ecx`), pushing `unit` from `[ebp-8]` to `[ebp-0x10]` and
  destroying the otherwise-exact frame.
- the same with `long reply_actor_index;` hoisted uninitialised, or declared
  `= NONE`: both 1616 / 88 / 501, 49 regions, byte-identical to each other (the
  initializer placement is inert). These are the **first shapes ever to merge
  friend+preselected** - `_actor_data` drops to exactly 3 - but a
  `mov edi,[ebp-0xc]` reload inserted after each `datum_get` breaks the enemy
  arm's tail merge instead (`_datum_get` 3 vs 2). One relocation over, in a
  different place.

**H3 - flip the EBX/EDI assignment (5 shapes + control).** REFUTED as an
independent lever. The assignment did not move under a single probe: declaring
and initialising `speech_priority` first, a C89 declaration split, an explicit
`if/else` for `speaker_actor` instead of the ternary, and hoisting
`speech_priority` to the enclosing block were all byte-identical to the floor.
Putting `speech_priority` textually first made VC7 hoist `mov edi,[ebp+8]`
*into the middle of* the speech-priority computation so that `unit_index` still
got EDI - the compiler actively works to keep it there.

**H4 - restructure the friend arm's guard (5 shapes).** REFUTED. `!= NULL`
spelled out, a named local for the absolute encounter index, and the full
`?:`-hoisted `speaker_encounter_index` form all produced a function section
**byte-for-byte identical** to the floor (same sha1). VC7 normalizes
`a && a->f != NONE` and `(a ? a->f : NONE) != NONE` to the same IL, so the
recorded `&&` / `?:` layout law has no purchase here. Binding
`unit->object.owner_team_index` to a named local moved bytes the wrong way
(real code 1566 -> 1560) by promoting `unit` into EBX and making the tail merge
*tighter*. The "nested `if` instead of `&&`" probe is unwritable: every version
either duplicates the call in source or changes behaviour.

**H5 - the dispatch form and case order (5 shapes).** REFUTED, with a hard
sub-result: **case source order and case-body braces are byte-inert here.**
Production order, enemy/target/friend, friend/enemy/target and a braced variant
all emit the identical section (sha1 `a8fcfe6782e0`). In a compare-chain switch
VC7 lays the arms out in descending case value regardless of how they are
written; the campaign's "case order is a real lever" law applies only where a
jump *table* exists. Replacing the switch with an `if`/`else if` chain reaches
size 1584 - **and that is a trap, not progress**: it hits the number by
coincidental compensation (three 16-bit compares replacing January's
proven-correct `movsx/sub/dec` chain cost about the same 16 bytes), while
instructions move from 482 to 489/491 against a target of 484, relocations do
not move at all, and the file dispatches the same enum with a `switch` 300 lines
earlier at `source/ai/ai_communication.c:4557`.

**H6 - the `reply_unit_index` data flow (5 shapes).** REFUTED. Hoisting
`reply_actor_index` beside `reply_unit_index`, binding the resolved datum to a
named local symmetrically at both sites, swapping the declaration order of the
two convergent variables, and moving `reply_unit_index` up to the enclosing
reply block were **all byte-identical to the floor**. The frame is unchanged by
the last of these, confirming `reply_unit_index` never reaches a frame slot at
all - it lives in EBX on every path. Inverting the outer test
(`if (preselected != NONE) ... else ...`) is the only shape in the family that
moved a byte, and it is a regression wearing a `+1 instruction` as a disguise:
size and relocations do not move, region count goes 72 -> 73, `relocdiff`
differing rows go 58 -> 65 because the three `match_assert` string relocations
that the floor matches at January's exact addresses (0x1be / 0x1c3 / 0x1c8)
shift, and it introduces a hoist January does not have.

## What would reopen this

- Authoritative January source for `ai_communication_finished`, or a
  same-compiler donor object built from a known source shape that emits two
  `call _ai_communication_find_global_actor_to_talk` sites **without** a third
  resolution block.
- A decoded rule for VC7's cross-jump *candidate ordering* - specifically what
  makes the backend prefer merging {enemy, friend} over {friend, preselected}
  when all three blocks are structurally eligible. That rule would also settle
  (c), which is the same decision in the debug-string epilogue.

**Do not repeat**: any dispatch-form, case-order, guard-spelling,
declaration-order, declaration-position or `reply_actor_index` hoist probe on
this function. Twenty-four of those are measured above, nineteen of them
compiled in this chase, and the four that move bytes all move them the wrong
way.

## Provenance

The probes were run as five isolated agents over disjoint copies of the source
(`scratch/xj/{enemy-private,ebx-edi,friend-shape,switch-form,reply-unit-flow}/`),
all scratch-only, none touching the production tree. Their measurements were
re-derived independently here before being written down: the relocation census
(`scratch/orch/relsites.py`), the four-region byte arithmetic, the two callee
register conventions, and every disassembly quoted above were reproduced
directly from `build/split/source/ai/ai_communication.obj` and a fresh gate of
the committed source.
