# Units carrier-body frontier and four residual mechanisms (Jonas, 2026-08-27)

Research record. **No matching credit is claimed and no production source
changed.** `units.obj` remains **175 strict-exact / 10 emitted nonexact / 4
absent** across 189 January code owners and **128 / 0 / 1** across 129 runtime
data owners, measured at integration HEAD `30658aed` (verified: `units.c`,
`config/symbols.json`, `source/objects/objects.h` and `source/math/real_math.h`
are byte-identical between the probe base `5329980c` and `30658aed`, so every
measurement below applies unchanged to the current tip).

The package described here **cannot be landed as a production wave**: it changes
the bytes of four nonexact owners without making any of them exact, and full
acceptance requires "zero inherited losses or changed nonexact owners". It is
recorded as evidence and as a starting point, not as a candidate for emission.

## 1. Headline: the reconstruction gap was scoping, not logic

The single most reusable finding. The integration lane reconstructed several
owners with **all locals hoisted to function scope**; January declares them in
their innermost block, which lets VC7 overlap frame slots. Restoring the
carrier's inner-block scoping collapses two owners almost completely:

| owner | integration body | carrier body | January target |
| --- | --- | --- | --- |
| `_unit_throw_grenade_release` | 768 / 26, **232** differing insns | **784 / 26, 4** differing insns | 784 / 26 |
| `_unit_set_or_test_seat_and_weapon_label` | 496 / 12, **141** differing insns | **544 / 12, 99** differing insns | 544 / 12 |

Both now match **padded size and relocation count exactly**. Combined with the
two field corrections in §4, **8 of the 10 nonexact owners now agree with
January on size and relocation count**; only `_unit_died` (608 vs 624) and
`_unit_update` (5984/262 vs 5936/259) still differ in shape.

Verified by full census on the combined probe object: **zero regressions**
(175/10/4 code, 128/0/1 data, unchanged), and no other owner's bytes move.

`unit_throw_grenade_release` proof that the scoping is exactly January's: the
carrier scopes `origin` inside the actor branch, so it shares frame slot
`[ebp-0x40]` with the player branch's `left`. The frame is `sub esp,0x40` = 64
bytes = `4 + 12*5`, exactly packed, and every `ebp`-relative displacement in the
784-byte body matches the target.

## 2. `_unit_throw_grenade_release` — one bijection event, two transpositions

With the carrier body the entire 784-byte function is byte-identical to January
**except two size-neutral adjacent transpositions** (8 bytes of 784), inside the
first inlined `point_from_line3d` expansion, where the scheduler hoists
`object_translate`'s argument pushes into the FP region:

```text
target    : fld [ebx+0x68] | fld [ebp-0x28] | fmul st(1) | push 0 |
            lea edx,[ebp-0x1c] | push edx | fadd [ebp-0x1c] | push edi | fstp [ebp-0x1c]
candidate : fld [ebx+0x68] | fld [ebp-0x28] | push 0 | fmul st(1) |
            lea edx,[ebp-0x1c] | push edx | push edi | fadd [ebp-0x1c] | fstp [ebp-0x1c]
```

January issues each FP op **before** the adjacent integer push; we issue the
push one slot earlier, twice. Instruction selection, register allocation and
frame layout are all identical.

### Measured inert — do not repeat

**In situ (~35 probes)**, all leaving the two transpositions untouched:
declaration order of the five inner-block locals (all 4 orderings tried,
byte-identical output — VC7 does not assign these slots by declaration order);
splitting `player_information`'s declaration from its initialiser; nesting the
third `point_from_line3d` into `object_translate`'s argument (byte-identical —
the returned pointer adds no DAG edge); `(*player_information).grenade_origin`;
`NULL` spelled `0`, `(struct location const *)NULL`, `(struct location *)0`;
making the function non-`static`; moving the whole definition elsewhere in the
TU; renaming a local in a distant function; swapping two distant function
definitions.

**Measured harmful:** `object_get_origin(grenade_index, …)` instead of the
carrier's `object_get_origin(unit->unit.grenade_object_index, …)` — the
redundant re-read is load-bearing, 768/26 and 231 differing instructions;
moving `unit_get_camera_position` earlier (19/59/65 differing insns); splitting
`normalize3d(cross_product3d(…))` into two statements (170); expanding the three
`point_from_line3d` calls by hand (170); copying `grenade_origin` to a local
first (183).

**Standalone repro, 85 variants, none reached January's phase.** A faithful
63-line model (`rp.c` in the lab) reproduces our phase exactly. A 72-cell
combinatorial sweep over trailing-call arity (1–4 args) and argument order,
parameter shapes, preamble length and scratch declarations never produced
`fld | fld | fmul | push`.

### Two facts that bound the problem

- **The phase is reachable and ordinary.** 45 functions elsewhere in the split
  tree contain an FP arithmetic op immediately followed by an integer `push`,
  and our build reproduces **all 45 byte-exactly**. This is not a compiler
  version or option difference.
- **But this exact signature is unique.** Scanning every split object for
  `fld | fld | fmul st(…) | push`, the only match in the entire tree is
  `_unit_throw_grenade_release` itself. There is no analogue to learn from.

Conclusion: a genuine VC7 list-scheduler tie whose input is not reachable from
this function's source. This is now the campaign's closest residual — 1
bijection event, 8 bytes — and the best possible target for a `dag.c` decode
(`0x107304d1`, see `c2_backend_attribution_20260826.md`), far better than
`unit_place`: size-identical, register-identical, a clean binary FP/integer
choice, and it occurs in a real `units.c` compile.

## 3. `_unit_set_or_test_seat_and_weapon_label` — root cause isolated

The carrier body is right and the integration body carries a **real semantic
bug**: it omits `unit->unit.animation.seat_index = seat_index;`. January stores
four animation fields (`0x250` seat_index, `0x252` weapon_type_index, `0x257`
base_seat_index, `0x251` weapon_index); the integration body stores three, which
is why it is 48 bytes short.

Remaining 24 events reduce to **one cause**. Our build hoists the inlined
`seat_label_to_base_seat_index` initialiser `result = NONE` from just before its
lookup loop (January: `or ebx,0xffffffff` at `0x173`) all the way up above the
three animation-index comparisons (`0x120`), which then lets VC7 narrow those
comparisons to 16-bit `cmp word ptr […], bx`. January cannot do that and emits
`movsx reg, word ptr […]` + `cmp reg,-1` three times. The store-order divergence
later in the function is a downstream consequence, not an independent event.

Measured inert: **all 24 permutations** of the four field assignments (88–99
differing insns, never resolving the narrowing); `(long)` cast present/absent;
naming the lookup result through a `short` or `long` local; doing the lookup
before the state check. `_seat_label_to_base_seat_index` is itself an exact
owner (64/2) and its source shape is already correct — do not touch it.

## 4. Two genuine field-selection bugs found and fixed

Both are the *same* reconstruction error, in two functions. Our source tests
`unit->object.functions_active_flags` (`+0xd3`) with
`_unit_debug_function_active_bit`; January tests the low byte of
`unit->object.damage_flags` (`+0xb6`) with mask 4, i.e. **`_object_dead_bit`**.
The mask coincides because both bits are bit 2, which is why the error survived.

- `_code_001a0cf0` — 32 → 31 differing instructions, 21 → 20 bijection events.
- `_unit_update_animation` — 240 → 239 differing instructions.

Semantically the January form reads correctly: *if this damage did not kill the
unit but the unit is already dead, do not apply the ping animation.*

Offsets were derived from `struct object_datum` in `source/objects/objects.h`
(`definition_index` + `_object_datum`): `damage_flags` at `0xb6`,
`functions_active_flags` at `0xd3`, `parent_object_index` at `0xcc` — the last
confirmed independently by `cmp dword ptr [ebx+0xcc],-1` in the same block.

## 5. `_code_001a0cf0` — what is left after the field fix

All 20 remaining events are one cluster (plus three artefacts from
disassembling the jump table as code) at the alignment-vector switch,
`0x50a-0x5cc`. It is a single register-role swap: January holds the
`alignment_vector` pointer in `ecx` and the `damage_direction` switch index in
`eax`, reserving `al` for the `hard_ping` / `killed` byte tests; we take `eax`
for the pointer. The source order of the five-term guard already matches
January's emitted order exactly, and all four switch arms match.

Measured inert: naming the pointer through a local inside the block; casting the
switch selector; `hard_ping!=FALSE || killed!=FALSE`. This is the "first-scratch
choice at a post-join definition" already predicted for this owner.

## 6. `_code_00198fd0` — the factor.c tail-merge, sharpened

The divergence is exactly one decision, in the `else` (no parent object) arm.
January propagates the constant into a **private epilogue** and declines to
merge; we materialise the value and tail-merge into the shared epilogue:

```text
target    : … jle 0x2e | pop esi | mov al,1 | pop ebx | ret |
                        pop esi | mov al,bl | pop ebx | ret
candidate : … jle 0x2e | mov bl,1 | pop esi | mov al,bl | pop ebx | ret
```

Both objects are 208/8; the padding absorbs the three-byte difference. Six new
source shapes measured, all **worse**: `return TRUE;` in place of
`result = TRUE;`, an explicit `return result;` at the end of the `else` arm, an
explicit `return result;` at the end of the parent-object arm, `return FALSE;`
for the switch arms, and the combinations — every one lets VC7 merge everything
and shrinks the function to 192 bytes. January's shape keeps the variable *and*
declines the merge; no ordinary-C form reaches it. Confirms this owner needs the
`factor.c` cross-jump decode at `0x1070c2e9`.

## 7. Carrier-body measured negatives (do not retry)

The carrier is **not** uniformly better. Measured in the current integration TU:

| owner | integration | carrier body | verdict |
| --- | --- | --- | --- |
| `_code_001a0cf0` | 21 ev | **128 ev**, 44 relocs | reject |
| `_unit_update_animation` | 94 ev | **187 ev**, 65 relocs | reject |
| `_unit_update` | 426 ev | 243 ev, still 5984/262 | better, still far |

And the bodies are **textually identical** in both trees for `_code_00198fd0`,
`_unit_died`, `_unit_euler_aiming_update`,
`_unit_preprocess_node_orientations` and `_unit_animation_set_state` — the
carrier offers those owners nothing, and their residuals are open reconstruction
problems in both trees.

## 8. Structural leads for the next lane (measured, not pursued)

- **`_unit_animation_set_state`** (294 ev): January's frame is `sub esp,0x1c`,
  ours `0x18`. January holds a **`short` local copy of the old animation state**
  (`movsx cx,al` / `mov word ptr [ebp-0x14],cx`, re-read at `0x32e` and
  `0x36e`) that our reconstruction lacks entirely, and keeps `changed_state` in
  `dl` where we spill it. That missing local is worth a lane on its own.
- **`_unit_euler_aiming_update`** (113 ev): `sub esp,0xb0` vs `0xa8` — 8 bytes
  of locals missing, with every subsequent slot displaced.
- **`_unit_preprocess_node_orientations`** (139 ev): frame slots displaced the
  same way.
- **`_unit_died`** (113 ev, 608 vs 624): decoded, and it is **block placement,
  not a missing statement**. Our source's statement order is already January's;
  the whole common tail (`flags &= ~(…)`, `control_flags = 0`, the
  `current_weapon_index` guard) is byte-for-byte correct. The difference is that
  January **sinks the `feigned` arm past the common tail**, giving
  `[!feigned arm][common tail][feigned arm → jmp back to 0xe1]`, where we emit
  the ordinary `[!feigned arm][jmp tail][feigned arm][common tail]`. The two
  arms' bodies, the `match_assert(…, 5099, …)` expansion and the two
  `SET_FLAG` tails at `0x1c0`/`0x1d0` are all identical between the builds.
  Measured: **inverting the arms to `if (feigned) … else …` reaches the target's
  624 bytes exactly but is wrong** — it moves the feigned arm inline and pushes
  the first divergence from `0x5b` all the way back to `0x1b` (110 events).
  Keep the current `if (!feigned)` order; the residual is cold-block sinking.
  `feigned==FALSE` is inert.
- **`_unit_update`** (243 ev with the carrier body): structural, lowest value.

## 9. Method note

Probing ran entirely out-of-tree at roughly **two full `units.c` translation
units per second**, which is what made ~200 measured variants affordable. The
harness (in-memory edit → real VC7 full-TU compile → `section_infos_equal` +
divergent-instruction count + bijection events, plus a whole-object census and a
cross-tree pattern scanner) is preserved in the scratch lab alongside
`probe.py`. It is a lab, not campaign tooling: no tests, not committed.

## 10. Single strongest next lever

**Decode the `dag.c` scheduler priority at `0x107304d1` against
`_unit_throw_grenade_release`, not `unit_place`.** It is one bijection event
and eight bytes from identity, size- and register-identical, the choice is a
clean binary FP-versus-integer ready-list tie, and — unlike the tiny repros
that never reach that code — it occurs inside a real `units.c` compile, which is
exactly the condition the gate-mode instrument was built for.
