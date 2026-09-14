# encounters.obj — opus5 150K house-clean lane, wave w3 (2026-09-14)

Unit `source/ai/encounters.c` (January 2002 Xbox debug, VC7 13.00.9254). Second-pass wave: every w2 law (scratch/w2/laws_w2.md A1-A22) run as a detector over the six non-exact rows and the three parks.

## Baseline and final

- Baseline real-file gate (`--all --forbid-emitted-symbol _point_from_line3d`): `== exact 55 residual 6 unwritten 0`; guard passed; parks 3/3 equal to `measurements.base`.
- Final real-file gate: identical listing, `== exact 55 residual 6 unwritten 0`. Guard passed, parkcheck `parks 3 drift 0`, owner census vs build/base: no new or lost owners, fake scan: 0 leads.
- `git diff --stat -- source/ai/encounters.c` is empty. **Nothing landed.**
- Worker notes: `scratch/workers/w3_encounters.md`. Scratch candidates: `scratch/workers/w3_encounters/`.
- Prior ledgers read: `encounters_obj_opus5_150k_w1_20260914.md`, `encounters_obj_opus5_fresh_graphs_20260914.md`, the platoon_rule_pair and initialize_cluster ledgers with their reviews, and `scratch/workers/encounters.md` (w1).
- This wave ran in three sessions. Two were cut off. Every unlogged scratch probe was re-read and is reported here.

## Per-function outcomes

| Function | Target | Real file | Outcome | Best candidate |
| --- | --- | --- | --- | --- |
| _encounter_update_status | 1136/38 | 1152/39 | NOT-LANDED (tie) | us1.c 1136/38 sha |
| _encounters_test_activation | 976/50 | 944/45 | NOT-LANDED (tie) | t3.c 960/50, 52 blocks |
| _encounter_create | 752/38 | 736/38 | NOT-LANDED (tie) | cr1.c 752/38 sha |
| _encounter_compute_activation_cluster_bit_vector (park) | 1200/46 | 1232/46 | NOT-LANDED (scratch) | cv6.c 1216/46 by evidence; cv5.c 1200/46 by bytes |
| _encounter_update_respawn (park) | 704/27 | 704/27 | NOT-LANDED (scratch) | ur2.c 704/27 sha |
| _encounter_test_rule (park) | 656/46 | 656/45 | OWNER-BLOCKED (exact scratch body) | tr8.c strict EXACT |

### _encounter_update_status

**Laws tested:**
- Single-exit / result flag (U1 `stay_active`): applied. It fixes the size and relocations: 1136/38.
- A10: no conditional init in the loop.
- A5: the squad and prop homes are proven.
- A17: the discarded `actor_in_combat` call is already present.
- Store/call order: the fighting local is proven.
- FP-intermediate type.

**Shapes:**
- us1 (U1): 1136/38 sha, 40 blocks.
- us2 (us1 plus `(real)` on the division operands): inert.
- us3 (us1 plus `body_count = 1` order): 41 blocks.

**Residual:**
- January splits the ebx zero web at the loop tail. Its roles are actor=edi and body_count/prop=ebx.
- us1 re-zeroes at the loop head, with actor=ebx and body_count=edi. This is the zero-register pinning family.

**Reopen:** a source form that moves the zero-web split. Land U1 only together with it.

### _encounters_test_activation

**Evidence:**
- January loads force_active into bl before `call game_in_editor` in both loops.
- The ors are 8-bit.
- The grouping is (force|editor) | (X|debug).
- HCEA PPC also reads force_active before the call.

**Shapes:**
- t2: one reason per statement, as an accumulator. 960/50, 52 blocks. The second loop is now byte-identical from the element fetch through `or bl,al; je`.
- t3: t2 plus reuse of the link flag. Identical to t2.
- t4: loop-scoped. Identical to t2.
- Earlier waves: A1, tv1-tv5, te1, tx, tx2.

**Residual:**
- One allocation decision per region. In the actor loop January has actor=edi and force=bl; ours has actor=ebx with force spilled to [ebp-1].
- The link loop differs the same way: January has i=ebx and the flag at [ebp-1].
- The 16 B size gap is the spill stores plus the loop-entry `jmp`.

**Reopen:** a source difference that lowers the actor pointer's priority for ebx in the actor loop.

### _encounter_create

**Evidence:**
- January frame 0x14 holds all_squads, actor_count, encounter_definition (ebx, spilled around the normal case), squad_index and actor_type (cached in ecx).
- create_leader is in bl.

**Shapes:**
- cr1 (C1+C2): 752/38 sha, 54 blocks.
- cr4 (A10 if/else assignment form): 58 blocks, worse.
- Earlier waves: scope sweeps were inert.

**Residual:** a register-priority tie with a byte-register anchor.

**Reopen:** a source difference that gives encounter_definition ebx. Apply C1+C2 only with it.

### _encounter_compute_activation_cluster_bit_vector (park)

**New evidence this wave:**
- January's dormancy select is branchy and 8-bit:

  ```
  je; xor al,al; jmp; mov al,[ebp+0xb]; mov ecx,[ebp-8]; mov [ecx+0x12],al
  ```

- **cv23** spells the ternary with a `(boolean)` cast. It is diagnostic only and inadmissible under R7. Its bytes equal the plain ternary cv5 (`xor eax,eax` / `movzx eax`).
  - A VC7 `?:` is int-typed whatever its arm types, so an 8-bit select is the phi of a char variable.
  - **The ternary spelling is refuted.** January's statement is:

    ```c
    if (squad->disable_dormant)
        dormant_desire = FALSE;
    actor->meta.dormant_desire = dormant_desire;
    ```

    This is cv6. Its actor loop is byte-identical to January.
  - The if/else-stores form (cv13) goes branchless and 32-bit instead.
- **cv25** is cv6 without the named `ultimate_parent_index` locals: 1232 B, with `add esp,8; push -1` before the parent call. The w1 locals are re-proven.
- **cv24** uses the sibling for-loop form from encounters_test_activation. It moves `mov byte [ebp+0xb],1` before datum_get, so it is refuted. January's order is actor_get, then `dormant_desire = TRUE`.
- **cv22** applies A17: a bare `unit_get(actor->meta.unit_index);` in the non-swarm branch, where January discards EAX. Its sha equals cv6, so it is byte-inert.

**Residual (cv6, 1216/46):**
- Only the firing-position pass differs.
- January releases bit_vector's ebx to firing_position_index and reloads bit_vector from [ebp+0x18].
- cv6 keeps bit_vector in ebx, spills the index to [ebp+8] and pins a zero in eax. That is January's own squad-pass treatment.
- cv5 (1200/46, 3 blocks) happens to get this allocation, but its dormancy spelling is refuted.

**Classification:** an allocation tie (a live-range split of the bit_vector parameter) after every structural element is byte-proven.

**Reopen:** a source difference elsewhere in the function that lets VC7 split bit_vector's ebx web at the firing-position pass. Apply it together with cv6.

### _encounter_update_respawn (park)

**Re-measured probes:**
- ur2 (w1 `while (left > 0 && current < min)` loop) on the current base: 704/27 ff3e16e6, 50 blocks.
- ud1/ud2 (single-exit nested if/else instead of the two early returns): alndiff byte-identical to ur2, so inert.

**January (ur_t.txt):**
- encounter_definition is in ebx for the whole function.
- The zero web is in edi and then holds squad_definition.
- The (long)squad_index CSE is homed at [ebp-8]; the short index is at [ebp-4].

**ur2:**
- The zero web is coalesced with the long-index CSE in ebx.
- encounter_definition is homed at [ebp-4].
- Statement order, loop shape and threading are identical to January.

**Diagnostic, never for landing:** enumerator padding flips ff3e16e6 <-> 26cd086d. Both states keep the same ebx allocation.

**Laws tested:**
- A3: inert.
- A16: applied.
- A12 and A20: consequence or identical.
- A5 and accessor re-fetch: identical.

No law explains the web choice, so no new shapes were spent.

**Reopen:** unchanged. A form that gives encounter_definition the ebx web, applied with ur2.

### _encounter_test_rule (park)

**New evidence this wave:**
- **tr6** removes the function-entry `result = FALSE` so that every path assigns exactly once (never arm, default arm, `else result = FALSE`). Result: 656/46 sha, 45 blocks. The target's `xor bl,bl` at 0x1c disappears, because VC7 does not hoist it. **January's initialiser is proven.**
- **tr7** has no init, a merged `case never: default:`, and an else arm. Result: 656/45 with a compressed table (`dec eax; cmp eax,8`).
- Together with the 10-slot jump table, this proves January's source. Slot 0 and the `ja` both go to the 0x106 `xor bl,bl` block, which is cross-jumped with the float false exits. So January assigns FALSE again in a distinct never case and in a distinct default, both over the FALSE initialiser.
- **tr8** is the w1 tr2 body re-gated on the current real file, with else-branch load order original_count, current_count, strength. It is **strict EXACT** 656/46 ba62edfc:
  - whole-TU gate `== exact 56 residual 5`, with only this row changed;
  - parkcheck shows only the intended drift.
- **Not proposed as a park reopen.** It needs redundant stores, which are R12 and the house-rule prohibition, even though the bytes now prove them authentic. It is filed as an owner-ruling item next to the w2 C3 redundant-store packet.

## Rejected / inert shapes this wave (do not repeat)

| Function | Shape | Result |
| --- | --- | --- |
| compute_activation | cv22: A17 bare unit_get plus swarm-scoped unit on cv6 | sha == cv6 (inert) |
| compute_activation | cv23: `(boolean)FALSE` ternary arm | == cv5 (width unchanged; R7 anyway) |
| compute_activation | cv24: sibling for-loop form with function-scope actor | TRUE store before datum_get (refuted) |
| compute_activation | cv25: cv6 without the ultimate-parent locals | 1232 (refuted) |
| compute_activation | cv13: if/else stores | branchless 32-bit select, 33 blocks |
| update_respawn | ud1/ud2: nested if/else single exit | == ur2 |
| test_rule | tr6: no initialiser, every path assigns | loses 0x1c `xor bl,bl` |
| test_rule | tr7: no initialiser, merged never/default | table compressed, 45 relocs |
| update_status | us2: `(real)` casts; us3: body_count order | inert / worse |
| test_activation | t4: loop-scoped accumulator | == t3 |
| create | cr4: A10 if/else assignment | 58 blocks, worse |

## Proposals

- **Owner ruling (no header or config text).** Admit byte-proven original redundant assignments for `_encounter_test_rule`:
  - the change is an explicit `case _platoon_rule_never: result = FALSE; break;` plus a distinct `default: result = FALSE; break;` over the `boolean result = FALSE;` initialiser;
  - it also reorders the encounter-wide else branch to original_count, current_count, current_strength_fraction;
  - candidate: `scratch/workers/w3_encounters/tr8.c` (current real file plus this body only), strict EXACT, no sibling or park loss;
  - the only consumer is `source/ai/encounters.c`.
- No header or config proposals.
