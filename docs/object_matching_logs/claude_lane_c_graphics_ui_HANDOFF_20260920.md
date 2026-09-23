# Lane C (graphics/UI) handoff - 2026-09-21

> Archival lane-local handoff, not a canonical admission report. This document
> was written before the final 60-commit tip and records only an interim
> tranche. See `claude_lane_c_canonical_reconciliation_20260922.md` for the
> audited net integration, rejected changes, and current metrics.

Brief: `docs/campaign_prompts/CLAUDE_REMAINING_LANE_C_GRAPHICS_UI_20260920.md`
(30 units: bitmaps, interface, rasterizer, rasterizer/xbox, render).

| | |
|---|---|
| frozen base | `6130661c9585fdb2785b3ff6bdb2ad0cfef0520c` |
| branch | `claude/lane-c-graphics-ui-20260920` |
| worktree | `C:\halo-worktrees\claude-lane-c-graphics-ui-20260920` |
| bare repo it was cut from | `...\2026-09-20\files-pasted-by-the-user-halo\work\halo-campaign.git` |
| pushed | **nothing** |
| canonical checkout | **untouched** |

Machine-readable per-function evidence:
`claude_lane_c_graphics_ui_admission_manifest_20260920.json`.

---

## 1. What this lane found that changes how the campaign works

Three inherited beliefs were wrong, and correcting them is worth more than the
bytes landed. All three are measured, not argued.

### 1.1 January's split objects cannot evidence COMDAT emission

A census over all 833 January split objects and all 833 of ours
(`scratch/orch/helper_census.py`):

| symbol | JAN definers | JAN referencers | OUR definers |
|---|---:|---:|---:|
| `_point_from_line3d` | 1 | 17 | 7 |
| `_normalize3d` | 1 | 54 | 60 |
| `_magnitude_squared3d` | 1 | 10 | 91 |
| `_scale_vector3d` | 1 | 5 | 61 |
| `_plane3d_distance_to_point` | 1 | 5 | 19 |

January shows **exactly one definer board-wide for every real_math header
inline** while dozens of objects reference each out of line. Identical COMDATs
folded to a single image copy and csplit attributed it to one object.

Consequence: `scratch/orch/newsyms.py` reporting *"January does not define it -
VIOLATION"* for a header inline is **structurally unreachable**, a false
positive by construction. It nearly cost this lane its first closure. The only
January-observable fact is out-of-line **reference**, which is exactly what the
owner's ruling keys on.

### 1.2 The inline-boundary census was unsound

`inline_boundary.py` compares only UNDEFINED external references, so any object
that emits a COMDAT itself never appears in the referencer list, and a symbol we
both define and call looks like a divergence. That produced phantom leads in
seven Lane C objects. Replacement: `scratch/orch/boundary2.py`, which classifies
every symbol DEF / REF / BOTH on both sides.

### 1.3 `_D3DSIMPLERENDERSTATEENCODE` is not a defect

The handoff flagged it as "one cause across six objects; find it once". Found,
and it is nothing. The relocation lives inside the XDK COMDAT
`_D3DDevice_SetRenderState`, at `mov ecx,[esi*4 + _D3DSIMPLERENDERSTATEENCODE]`
guarded by `cmp esi,0x52` (`State < D3DRS_SIMPLE_MAX`) - the scaled index proves
`State` is a variable, so `D3D8.h`'s `D3DINLINE` went out of line exactly as its
own comment describes. **Our build emits that COMDAT in precisely the same 19
objects as January.** The array itself is `extern CONST DECLSPEC_SELECTANY`;
ours has 106 definers and 0 referencers, January 1 and 20. Pure attribution.
No Halo source change exists or is needed.

---

## 2. New compiler laws, proven here

### LAW A - VC7 inline gating, decoded

VC7 /O2 decides inlining **per call site**, comparing one scalar callee cost
against a budget set by that site's statically estimated block weight.

Measured **inert**: caller size (callers to 2,384 bytes), number of call sites
(1/2/4/8 - strictly all-or-nothing at the boundary), argument shape, declaration
order of the callee's locals, statement order inside the callee, and all ten
optimisation-flag sets tried.

Movers:
- an aggregate local's `= { 0 }` initialiser is one whole cost STEP;
- an early-return guard lowers the budget of the rest of the function by a step
  (the same `if` without a return is inert);
- a call site inside a loop gets about +3 steps, +1 per nesting level;
- storage class is inert (`__inline` == `static` == `static __inline`); only
  `__forceinline` overrides;
- `__asm` in a body does not block inlining; a `match_assert` in the body is
  what pushes a small helper over.

**A site's budget is measurable**: insert a static probe helper of tunable
statement count and bisect. Lab and probes: `scratch/research/inline-gating/`.

It paid: `s3tc::RGBToColor` cost 12 against a budget of 11 at both
`DecodeBlockRGB` sites; dropping its `= { 0 }` closed two functions.

**Forbidden**: `do { } while (FALSE)` or a one-trip `for` to buy the loop bonus.
It works, and it is fake matching. It is called out in the worker brief.

### LAW B - block/label selection IS source-reachable

This **refutes** the standing law that cross-jump selection is a scratch-register
consequence. VC7 merges returns that provably yield the same VALUE; the survivor
is canonical and alone carries a `$L` label, and a conditional branch targets it.
Changing only the VALUE of a second return flips which identical tail is
targeted, all other instructions unchanged. Returns merge on value, not spelling.
**Instrument: `CL /FAsc`.**

### LAW C - loop landing-pad reload order

Follows the order the loop body's values were **defined**, not declared.
**Caveat, measured**: sixteen definition-order shapes were byte-inert in
`_bitmap_2d_alpha_bleed`, so this is not the operative rule everywhere.

### LAW D - shrink-wrap

VC7 sinks a callee-saved push below a guard test only when the guard's false path
has its **own** return statement.

### LAW E - assert predicate strings

An assert predicate's `.rdata` string records the **preprocessed** source text
exactly: every space is a space and a line continuation becomes one space. Its
byte length therefore pins January's spacing *and* line layout of the predicate.

### LAW F - inner-block scoping

A scratch out-parameter aggregate declared at function top gets its own frame
slot; declared in the innermost block that uses it, VC7 coalesces it with a dead
local. This closed the frame blocker that `_rasterizer_sun_glow_draw`'s park had
recorded as donor-only.

### LAW G - global aggregate stores

VC7 hoists and sinks stores to distinct non-overlapping members of one global
aggregate freely. Their emitted order is **not** a source-statement-order readout.

---

## 3. Measured dead - do not respend

- **Adding a `REAL_MATH_EXTERNAL_*` switch**: nine trials across six units, every
  one inert or a regression; two *broke* exact functions
  (`_render_frustum_build_point_flags`, `_build_sprite_compute_vertex_fade`).
  An all-or-nothing switch cannot express a per-site inline decision.
- **The `first_person_weapons` switch lattice**: all fifteen PAIRS probed on top
  of the singles and the all-six case. Best is size-equal at 61 relocations
  against January's 59; the count never goes below 60.
- **`_rasterizer_bitmap_new`'s early-return guard**: `[size 384!=400]`, sixteen
  bytes worse. January really does place the guard block at the end.
- **The `interface` profile stall enumeration**: unrecoverable, not merely hard.
  `_profile_rasterizer_stalls` has **zero referencers in January too**, so the
  bit meanings are not in the image by construction; the Xbox 1 XDK headers carry
  no stall enum; and a full `DIA2Dump -t` of HCEA's `HCEX.pdb` (78 MB) contains
  no `stall_index`, `stall_count`, `profile_frame_info` or
  `profile_frame_iterator` at all - the 2011 build had removed the debug profile
  code. Every "stall" hit there is an Xbox 360 GPU register bitfield.
- **Declaring `local_lens_flare_occlusion_test_results` with one more entry** to
  avoid January's overrun: invents a bound January did not have *and* moves the
  `.bss` layout. Rejected; the overrun is real (see 5.4).

---

## 4. The relocation-alias class - 3,872 bytes disposed, no credit

Four `ui_widget_game_data_input_functions` rows differ from January in exactly
one relocation's **symbol spelling** and nothing else: identical code bytes,
identical relocation count, offsets and types, identical normalized hash on both
sides, objdiff 100.0. Resolved through `config/symbols.json`, both spellings name
the same image address:

| ours | January | address |
|---|---|---|
| `_cached_player_profile+156` | `_local_player_index_for_draw_string_and_hack_in_icons` | 5841532 |
| `_local_player_controller_bitmap_frames+26` | `"invalid game_data_input_referenc..."+2` | 2584102 |

`cached_player_profile` is three 52-byte entries at 5841376, so `+156` is one
past its extent and csplit attributes it to the next image symbol. The array in
the second case is 24 bytes at 2584076 and the literal begins right after it.

Parked as `csplit-relocation-alias` in commit `5f7ca192`. `_bitmap_sharpen`
(352 B) is the same class, already parked, and verified here the same way:
`_bitmap_sharpen_positive_table-2` and `_bitmap_sharpen_negative_table+510` both
resolve to 3260670; `_bitmap_sharpen_negative_table-2` and `_extract_data+42`
both to 3260158.

No C source makes VC7 name the successor symbol, so there is no source lever.
These rows finish in category 4 of the brief, not "unexamined".

---

## 5. Owner rulings obtained, and their effect

Full text: `scratch/lane_c/owner_rulings.md`.

### 5.1 Granted
- **Real_math inline COMDATs may be emitted** wherever January's same object
  references the helper out of line, **extended to all such helpers**, on five
  conditions (byte-identical COMDAT, strict-exact caller, authentic header use,
  full ownership sweep, zero regressions). Applied in `a224e887`.
- The all-inlined header-inline COMDAT class binds, no per-symbol approval.
- `_eat_my_shorts`' duplicated equivalent branch: admitted as **recovered control
  flow**, not steering.
- `_rasterizer_lights_reset_for_new_map`'s 34-byte overrun: admitted **on proof**
  that the clobbered bytes are reinitialised before any read (5.4).
- `s3tc::RGBToColor`'s unwritten alpha byte: admitted as target-proven and
  harmless, after the premise of an earlier hold changed (5.5).

### 5.2 Held, with ready candidates
- **`hud_draw::_fast_ftol_C`** - EXACT at 56 B, but only with
  `#pragma optimize("",off)`. Held; rule 8 stands. The 56 code bytes are
  byte-identical either way - the pragma changes only 8 tail nops and the inline
  decision. Stripping it also regresses `_hud_draw_meter` from EXACT 1312.
  `__declspec(noinline)` and `naked` both give 64. January's image genuinely
  holds an unpadded 56-byte COMDAT (followed by `cc` fill), and only 2 of 7,561
  January code sections are non-multiples of 16.
  **Separate owner action requested**: `source/ai/ai.c:2888` already ships
  `#pragma optimize("", off)` in production and contradicts the rule.
- **The `interface` packet** - `_render_debug_profile_stall_tick` EXACT 272 plus
  `_render_debug_profile` fuzzy 2,272. Held pending four corrections outside the
  allowlist; the proposal and consumer census are in
  `scratch/lane_c/interface_prerequisite_proposal.md`. Note `profile.h` has **52
  consumers**, so whoever lands it must run the full-tree diff - Lane C did not,
  deliberately, because wave workers were compiling against those headers at the
  time and a mid-wave shared-header change would have corrupted eight units'
  measurements.
- **`render_sprite`'s 304 B** - reachable only by passing `&transformed_origin`
  (a `real_point3d **`, a 4-byte object) where a `real_vector3d *` is expected
  and letting the callee write 12 bytes through it, leaving `*transformed_origin`
  unwritten in screen-space mode and read uninitialised at three call sites. Held
  under rules 8 and 10. The clean part of the unit landed separately.
- **`rasterizer_xbox_environment_fog`'s five `*(long *)&real` punning casts** -
  held; zero exact credit.
- **The `interface` stall switch's raw labels** - held; enum unrecoverable (3).

### 5.3 The data-lane prerequisite
Ruling: **take none**. Lane C is a pure cut of `6130661c` and must be rebased
after `claude/data-section-credit-20260920` merges. The seven prerequisite
commits are listed in the Lanes A/B handoff; three change relocation identity
Lane C code will need (assert respellings in `rasterizer_lights` and
`rasterizer_xbox_shadows`, the `bitmap_group` field rename in
`rasterizer_xbox_environment_fog`). **Reopen criterion**: after that merge,
re-measure those three units first.

### 5.4 The proof behind the lights overrun
January memsets 0x7722 = 897x34 bytes over an array whose own `.bss` extent in
January is exactly 30,464 = 896x34 (from `config/symbols.json`: the array starts
at 4877216, `_local_lens_flare_parameters` at 4907680). It genuinely clobbers 34
bytes of the next global. Admitted because:
1. the memset and `local_lens_flare_count = 0` are in the same function with no
   read between them;
2. every read of `local_lens_flare_parameters` is guarded by that count - the
   accessor asserts `lens_flare_index<local_lens_flare_count` and all four
   consumer loops bound on it;
3. the count only grows at the site immediately followed by
   `memcpy(lens_flare_parameters, parameters, sizeof(*lens_flare_parameters))`,
   a full-struct write.

So after the reset no element is readable, and an element becomes readable only
once wholly overwritten. The argument is independent of the struct's size.

### 5.5 The s3tc alpha byte
`RGBToColor` leaves `rgba[S3TC_ALPHA]` unwritten and copies it out indeterminate,
as January does (January stores only three bytes of the local and copies four).
Both callers assign the alpha of every colour on the line immediately after, so
it is never read, and `byte` is unsigned char so there is no trap representation.
**It is not avoidable**: `= { 0 }`, an explicit alpha store before the channels,
and one after the channels were each measured and each returns both functions to
residual.

---

## 6. Data sections - three have ZERO differing bytes

| unit | section | size | objdiff | differing bytes |
|---|---|---:|---:|---:|
| render_particles | .data | 1528 | 99.739% | **0** |
| bitmaps | .data | 1116 | 89.002% | **0** |
| hud | .data | 4 | 50.000% | **0** |

Not content defects - **symbol coverage**. `bitmaps .data` shows it plainly: the
bytes at +1024..+1115 are identical, but ours names `_bitmap_type_string_table`
and `_bitmap_format_string_table` there while January's object names nothing,
because they are `static` and therefore absent from `cachebeta.pdb`'s publics.
objdiff cannot pair them and withholds the section. This is the naming-gap class.

Not pursued: it needs `config/symbols.json` surgery plus a re-split, the file is
the riskiest artifact in the campaign, the evidence a static needs cannot come
from the PDB by construction, and the data lane owns this area. Full diagnosis
and reopen criteria: `scratch/lane_c/data_section_diagnosis.md`.

`interface .rdata` (328 B, 56.8%) is a different case and is **code** work: the
three sections we lack are the two unwritten functions' string literals.

---

## 7. Method notes for the next lane

- **Re-measure every agent claim.** Across four waves, reviewers and the
  orchestrator caught: a gain bought by a `#pragma`; a gain bought by a 12-byte
  write through a 4-byte object; a candidate that privately redefined another
  TU's struct with different signedness and declared a six-argument prototype for
  a one-argument function; three unrelated UB changes shipped alongside one real
  gain; and roughly a dozen "GAIN" labels on rows that never reached EXACT.
  Every one was caught by re-running the gate, not by reading the report.
- **Make the reviewer build the minimal landing.** The single most useful
  reviewer output was `revF_minimal.c` - production plus only the one hunk that
  mattered - which turned a rejectable candidate into a clean 544-byte landing.
- **A parked sibling whose bytes move must be refreshed in the same commit.**
  The s3tc landing staled `_Quantize` and `_DecodeBlockRGB__single_pixel` and
  failed ninja's PROGRESS gate until `refresh_parks.py --apply` re-measured them
  (they improved to 88.31% and 99.28%).
- **Do not touch shared headers while a wave is running.** Workers compile
  against them; a mid-wave change silently corrupts every unit's measurements.
- **`castcheck.py` has a blind spot**: its regex matches only `real_*` view casts
  and cannot see `(long *)&`. A PASS from it does not clear punning.
- Agent economics here: ~165-210K tokens per agent; a wave of 8 units
  (8 workers + 8 reviewers) cost ~3.3-3.4M tokens and ~57 minutes at 8 concurrent.

---

## 8. Tools written for this lane (`scratch/orch/`)

| tool | purpose |
|---|---|
| `helper_census.py` | board-wide definer/referencer census for any symbol set |
| `boundary2.py` | sound DEF/REF/BOTH boundary census (replaces `inline_boundary.py`) |
| `find_reloc.py` | which function holds a relocation, with disassembly context |
| `callers_of.py` | which functions in an object call a symbol |
| `alias_resolve.py` | resolve two relocation spellings to image addresses |
| `comdat_bytes.py` | compare a COMDAT's body bytes across objects |
| `folding_probe.py` | test the COMDAT-folding hypothesis for a symbol |
| `switch_add_probe.py` | boundary-driven probe that ADDS a `REAL_MATH_EXTERNAL_*` switch |
| `park_add.py` | append parked.json entries by line surgery, measured live |
| `make_manifest.py` | build the admission manifest |
| `WORKER_BRIEF.md` | the binding worker contract, incl. the wave-1/2 lessons |

Evidence lives in `scratch/lane_c/` (research notes, rulings, data diagnosis,
the interface proposal) and `scratch/research/` (the inline-gating lab).

---

## 9. Results as of 2026-09-21 (interim - the lane is still running)

| | baseline | now | delta |
|---|---:|---:|---:|
| strict exact functions | 697 | 705 | **+8** |
| meaningful code bytes | 156,512 | 160,175 | **+3,663** |
| padded code bytes | 157,680 | 161,408 | **+3,728** |
| lane data bytes | 458,655 | 459,507 | **+852** |
| exact regressions | - | - | **0** |

Eleven commits on `claude/lane-c-graphics-ui-20260920` from `6130661c`. Nothing
pushed; the canonical checkout was never touched. Per-function before/after
evidence is in the admission manifest beside this file.

The eight closures: `_rasterizer_lens_flares_submit_occlusion_tests` (400),
`_DecodeBlockAlpha3__single_pixel` (432), `_eat_my_shorts` (32),
`_hud_show_action_response` (1,744), `_rasterizer_lights_reset_for_new_map` (48),
`_RGBToColor` (96), `_DecodeBlockRGB` (448), `_Quantize` (528).
`source/interface/hud` reached FUNCTION-COMPLETE at 22/22.

Two commits deliberately bank zero credit and are landed for correctness:
the `ColorToFcolor` loop form (preserves a measured 2,192-byte option that the
unrolled spelling silently foreclosed) and `_render_blip`'s initialiser move
(frame closed to January's exactly, relocation mismatch resolved).

### Measured, verified, and waiting on owner decisions - about 6,900 bytes

| item | bytes | blocked by |
|---|---:|---|
| `_EncodeBlockRGBColorKey` | 2,192 | needs the `&& cOpaque == 16` conjunct (HELD: reachable 0/0 and `mapRGB3[(long)NaN]`) |
| `_render_debug_profile_stall_tick` + fuzzy caller | 272 (+2,272) | cseries/effects header corrections outside the allowlist |
| `_DecodeBlockRGB__single_pixel` | 496 | needs a 60-byte write past a single-pixel caller object (HELD) |
| `_render_sprite` transform | 304 | needs a 12-byte write through a 4-byte object (HELD) |
| `_fast_ftol_C` | 56 | needs `#pragma optimize` (HELD) |

s3tc would reach 18/18 function-complete on the first and third together; the
all-safe variant was measured and gains nothing, so there is no safe path.

### What remains, by kind

- ~27,800 B frame-structural, now addressable by the decoded frame-slot rule
  (section 2) - wave 6 is the first wave to use it.
- ~7,400 B in functions that are ALREADY SOURCE-CORRECT and blocked by
  translation-unit state with no byte signature. Packet P3 of
  `scratch/lane_c/breakthrough_proposal.md` instruments the compiler to name that
  state. **These must not be parked as ties**: January's bytes came from
  human-written C, so the state is a consequence of TU content and is
  recoverable.
- 4,224 B in the csplit relocation-alias class - the one class where "not source
  reachable" is a true statement, proven at the address level. It needs a
  splitter or comparator decision, not source work.
