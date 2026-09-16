# ai_debug.obj — next-150K house-clean lane, wave n2 (2026-09-15)

Unit `source/ai/ai_debug`. Worker scope: `source/ai/ai_debug.c` only (the only file changed).
Worker notes: `scratch/workers/n2_ai_debug.md`; probes: `scratch/workers/n2_ai_debug/`.

## Result

**`_code_00041220` closed strict EXACT — 2,336 padded bytes / 169 relocations.**
Unit gate moves `== exact 56  residual 4  unwritten 0` → `== exact 57  residual 3  unwritten 0`, with **zero**
regressions: the full `--all` listings differ by exactly one line. The function is the one this object's ledger
has carried open since 2026-08-10, when the "tu-context inliner tie" adjudication was withdrawn under the
byte-match-only rule.

The landed symbol still carries a `code_` + address name, so this is reported as **EXACT-PENDING-RENAME**; the
authentic name `_ai_debug_render_encounter` needs a `config/symbols.json` line-654 edit, which is the
orchestrator's to make (packet at the end of this ledger).

`_code_0003a910` is recorded **SKIPPED-EXHAUSTED** with a full census and a refutation of the one law that
named it as a hypothesis. `_code_00039990` (laws_w2 section D "skipped as exhausted") and
`_ai_debug_render_actor` (brief section 1 DO-NOT-SPEND) were not touched.

## Baseline and final gate

`python -B tools/campaign/gate.py source/ai/ai_debug --all` — the `--forbid-emitted-symbol _point_from_line3d`
guard fails at baseline on this object's pre-existing out-of-line `_point_from_line3d`, as recorded by the 100K
handoff and the w2 ledger, so the gate is run without the guard and the owner census is used instead.

| row | January padded / relocs | baseline candidate | final candidate | verdict |
| --- | ---: | --- | --- | --- |
| 56 functions | — | EXACT | EXACT | unchanged |
| `_ai_debug_render_actor` | 24,976 / 1,905 | 24,848 / 1,903 | 24,848 / 1,903 (byte-identical) | not touched (DO-NOT-SPEND) |
| `_code_00039990` | 848 / 42 | 848 / 42 `[sha]` | byte-identical | not touched (exhausted) |
| `_code_0003a910` | 704 / 23 | 704 / 23 `[sha]` | byte-identical | SKIPPED-EXHAUSTED |
| `_code_00041220` | 2,336 / 169 | 2,448 / 185 `[size, relocs, sha]` | **2,336 / 169 EXACT** | **closed** |

Owner census of the final candidate versus `build/base/source/ai/ai_debug.obj`: symbol sets **identical**, i.e.
no new candidate-only code/data/BSS/COMMON owner; the pre-existing real_math select-any surplus is unchanged.
Parks: `scratch/parkcheck.py source/ai/ai_debug` → "no parks for source/ai/ai_debug".
`tools/fake_match_scan.py source/ai/ai_debug.c`: 0 review leads.
`branch_sweep.py source/ai/ai_debug`: 82 unique historical blobs, best historical is 12 exact; current tree 57.
File diff: 7 insertions / 5 deletions in one hunk.

## `_code_00041220` — what the residual actually was

**Frame census.** January `sub esp, 0x8ec`; ours `sub esp, 0x8ec`. Every `[ebp±N]` slot is identical on both
sides and in the same role (`-4` owner_actor_index, `-8` index copy, `-0x24` firing-positions block pointer,
`-0x28` index counter, `-0x2c` examined_count as a `short` read with `movsx …, word`, `-0x30/-0x34/-0x38`
origin.z/.y/.x, `-0x3c` history_start_time, `-0x78/-0x7c/-0x80` point.z/.y/.x). There was never a frame gap: the
`[size]` tag was not a frame-structural defect.

**Relocation census by target.** The 185 vs 169 gap is one surplus inlined `ai_debug_drawstack` body and
nothing else:

| target | JAN | baseline OURS |
| --- | ---: | ---: |
| `_ai_debug_drawstack` (call) | 3 | 2 |
| `_global_ai_debug_drawstack_height` | 3 | 6 |
| `_global_ai_debug_drawstack_next_position` +0 / +4 / +8 | 3 / 2 / 2 | 6 / 4 / 4 |
| `_global_ai_debug_drawstack_last_position` +0 / +4 / +8 | 2 / 2 / 2 | 4 / 4 / 4 |
| `_global_up3d` | 2 | 3 |
| the other 43 distinct targets | — | all equal |

+17 globals − 1 call = +16 relocations; +112 bytes = one inlined body.

**Site census.** The function has five textual `ai_debug_drawstack()` sites; sites 4 and 5 tail-merge, so four
are emitted:

- January: site 1 CALL @0x646, site 2 INLINE @0x6d7, site 3 CALL @0x8a6, site 4/5 CALL @0x8d6.
- baseline ours: site 1 INLINE, site 2 INLINE, site 3 CALL, site 4/5 CALL.

Everything else was instruction-identical. The only knock-ons were the `je` at 0x610 widening from a 2-byte
short jump to a 6-byte near jump (this is the 4-byte prefix shift the earlier ledgers saw at 0x612/0x616) and a
handful of downstream register renames in blocks that are otherwise instruction-for-instruction equal.

## The lever: laws_w3 **A23** applied in reverse

**Landed change** (`source/ai/ai_debug.c`, the firing-position colour-table calibration near the top of the
function):

```c
    if (global_ai_debug_firing_position_color_count==NONE)
    {
        global_ai_debug_firing_position_color_count = 0;

        while (global_ai_debug_firing_position_colors[…].alpha<=1.f &&
            global_ai_debug_firing_position_colors[…].red<=1.f &&
            global_ai_debug_firing_position_colors[…].green<=1.f &&
            global_ai_debug_firing_position_colors[…].blue<=1.f)
        {
            global_ai_debug_firing_position_color_count++;
        }
    }
```

replacing the empty-bodied `for (… = 0; <same four sentinel tests>; …++) ;`.

**Mechanism.** A23: a `for` loop *with an increment clause* inflates the estimated frequency of the code after
it; the inflation survives an enclosing if/join and compounds over consecutive loops, while `while`, `do`,
guarded `do`, `for(;;)`+break and `for(;c;)` do not inflate. The calibration loop sits inside
`if (count==NONE)` near the top of the function; the site-1 `ai_debug_drawstack()` call sits far below it,
inside `if (render_pursuit && record.pursuit)` then `if (selected_actor_index!=NONE)`. The inflation lifted site
1 over VC7's auto-inline threshold. Site 2 is one conditional shallower and therefore genuinely hotter, which is
why it stays inlined in both builds — exactly the ordering an absolute frequency threshold predicts.

**Two controls were measured, and both matter.**

| shape | change | result |
| --- | --- | --- |
| v1 | merge the duplicated evaluation render (`if (s) {A;B} else {B}` → `if (s) {A} B`), which the January CFG at 0x843-0x8b6 shows as one physical B reached from three paths | **worse**: 2,544 / 201, three inlines |
| v2 | respell the *adjacent* `for (color_index = 0; color_index<num_firing_position_colors; color_index++)` as a `while` | **inert**: byte-identical to baseline |
| v3 | respell the *distant* calibration loop | **EXACT 2,336 / 169** |

v2 proves the effect is not "any loop respelling perturbs the inliner": the nearest for-with-increment, only a
few statements above the site, is completely inert. v1 proves the decision is a **per-site frequency threshold,
not a caller-growth budget** — making a different site hotter *adds* an inline without evicting either existing
one — which is why the frequency estimate, and not a size budget, had to be what changed.

**Source credibility.** The landed spelling is plain C and the more idiomatic of the two: an empty-bodied `for`
with a lone `;` on its own line becomes "start the count at zero and advance while the table entry is still a
legal colour". The loop's own emitted bytes are identical under both spellings (the function's prefix through
0x5d2 was already instruction-identical before the change, and the calibration loop lies inside that prefix; the
function is now exact end to end). Nothing prohibited is involved: no `volatile`/`register` steering, no
representation cast, no alias or wrapper, no forced inline, no redundant store, no duplicated statement, no UB,
no bug reproduction. The one exact form recorded for this function in the 2026-08-29 census — a one-use
function-pointer alias of `ai_debug_drawstack` — is **not** used and remains rejected as byte-forcing.

**What this closes in the object's history.** The 2026-08-10 reopen criterion for this function was "any source
form, flag, or measured inliner-cost lever that flips the surplus site to a call without perturbing the
correctly-split site", and the 2026-08-29 minimization narrowed it to "a naturally justified source/context
lifetime must make that site a direct call". A23 is that lever; the flipped site is exactly the surplus one and
the correctly-split site 2 is untouched.

## `_code_0003a910` — SKIPPED-EXHAUSTED (full census, no shape spent)

**Frame census.** January `sub esp, 0x38`, ours `sub esp, 0x38`; the `[ebp±N]` slot sets are identical
(`+0xc`, `+0x10`; `-4`, `-8`, `-0xc`, `-0x10`, `-0x14`, `-0x18`, `-0x1c`, `-0x20`, `-0x24`, `-0x28`, `-0x2c`,
`-0x30`, `-0x34`, `-0x38`). Not structural.

**Relocation census.** 23 vs 23, every target equal in count and in address after the two-instruction shift.
The only rows the tool prints as differing are two known non-differences: `__real@3f800000` rendered as `symbol`
in the target versus `defined-noncode .rdata` in ours, and the csplit relocation alias (January attributes the
render-counter address to `_actor_path_debug_array+4`; ours owns it as `_global_ai_debug_path_render_id+0`, the
same image address — the w2 p1b probe already confirmed this). The gate tags the row `[sha]` only.

**The residual is two instructions**, the centre-averaging reciprocal fold. After `fild [ebp-4]; fdivr [1.0f]`:

```
January   fld [ebp-0x30] ; fmul st(1) ; fstp [ebp-0x30] ; fstp st(0)     keeps r, then pops it
ours      fmul [ebp-0x30] ; fstp [ebp-0x30]                              folds the third use into r
```

**Why no shape was spent.**

1. `ai_debug_obj.md` ("Post-walker negatives") already records the sweep: *"a910 divide spellings re-swept
   (divisions, braced scale, reciprocal x3, scale_vector3d): all still fold"*; `point = &centre` hoisted before
   the divides normalises away; per-arm `code_00039990` calls cost +80 bytes and still fold.
2. The only law naming this function, **A31 CONSTRUCTOR ROUTING** ("named consumers `_code_0003a910`,
   `_biped_update_physics` are hypotheses"), is refuted here by the target bytes. A31(a) is a deferred cdecl pop
   *after a call*, and a910's fold window is call-free in both directions; A31(b) governs the
   `set_real_vector2d/3d` aggregate copy form, and a910's only aggregate copy
   (`centre = node->closest_point_to_attractor;` at 0x21d-0x231) is already instruction-identical in both builds.
3. The discriminator the ledger identifies — counted-exit versus chase-exit loop — is byte-proven and cannot be
   respelled: the accumulation loop is a chase-exit `do { … } while (edge_index!=surface->first_edge_index);`,
   fixed by `mov eax,[esi+edi*4+8]; cmp eax,[ecx+4]; jne 0x194` at 0x1e6.
4. The A23 lever that closed 41220 does not transfer: it moves an *inliner* frequency input, and a910's residual
   is an x87 stack-liveness / instruction-selection decision at a call-free, inline-free site.

**Reopen criterion (unchanged):** a worked example of a chase-exit *keep* from any source shape anywhere in the
January image, or a c2-level IR lever identifying the input to the encode-time liveness computation. Do not
re-run divide-spelling sweeps, helper routing, `point = &centre` hoists, or per-arm call duplication.

## ORCHESTRATOR-PROPOSAL — rename `_code_00041220` (config + source, one commit)

- **Config:** `config/symbols.json`, **line 654**, edited in place (never sort or re-serialize the file): change
  only the `"name"` value `_code_00041220` → `_ai_debug_render_encounter`.
- **Source:** word-bounded rename of `code_00041220` → `ai_debug_render_encounter` in `source/ai/ai_debug.c` at
  the prototype (line 201), the single call site (line 485) and the definition (line 5700 before this wave's
  landing; the landed file shifts the definition by +2 lines).
- **Evidence:** `scratch/atlas/ai_debug.obj.txt:44` — `00041220 fuzzy ai_debug.obj _ai_debug_render_encounter`;
  the atlas's `exact`-tier records for this object are byte-matched at their recorded offsets and the w2 ledger
  re-anchored the file on `_ai_debug_dispose_from_old_map` (0x375B0) and `_ai_debug_drawstack` (0x39920).
  Corroboration: `ai_debug_render` calls it with `ai_debug.selected_squad_index` (an encounter index); the
  parameter is `encounter_index`; the callees are `encounter_get`, `encounter_actor_iterator_new/next`,
  `encounter_build_firing_position_owner_actor_indices`, `encounter_pursuit_position_already_examined`; the
  assert literal is the `c:\halo\SOURCE\ai\ai_debug.c` line-964 firing-position-colour invariant. No PDB public
  exists (cachebeta.pdb carries no Halo locals), so atlas plus topology is the strongest available tier.
- **Measured inert:** the w2 worker gated the 20-name atlas rename batch (`scratch/workers/ai_debug/w2/n1.c`
  with `n1.aliases`) at 56/4/0 with every hash identical; the rename cannot move a byte.
- **Consumers:** no other source file names the identifier. The only mentions are a comment in
  `source/ai/ai_scenario_definitions.h:313` and text references in `tools/emission_shape_oracle.py` and
  `tools/context_diff_oracle.py` (which name `_code_00039990` and `_code_0003a910`, not 41220). No
  `config/parked.json` or `semantic_matches.json` entry references it.
- The remaining 19 names of the w2 atlas batch may ride in the same commit; that proposal is unchanged.

## Law confirmation for the lane

**A23 (FOR-INCREMENT FREQUENCY) is confirmed on a second, independent function and in the opposite direction.**
w3 landed it by *adding* a for-with-increment to make a static inline late (`_cache_hardware_format_character`);
n2 landed it by *removing* a for-with-increment to make a static stay out of line
(`_code_00041220`). The 41220 measurement also adds two facts the w3 write-up asked for:

- the effect is **distance-insensitive but site-specific**: the distant calibration loop carries it while the
  immediately-preceding colour loop is inert (v2), so "nearest loop" is not the rule;
- the decision it moves is a **per-site frequency threshold, not a caller-growth budget** (v1: adding a hotter
  site adds an inline without evicting a colder one).

Both are recorded here rather than in the brief; the lane owner can promote them.

**Alias gate for the pending rename (measured 2026-09-15).** `python -B tools/campaign/gate.py source/ai/ai_debug --source scratch/workers/n2_ai_debug/rename_probe.c --alias ai_debug_render_encounter=code_00041220 --all` gives `EXACT 2336 _code_00041220` and `== exact 57  residual 3  unwritten 0`, i.e. the word-bounded source rename (3 sites) is byte-inert on top of this wave's landing.
