# Profile.obj frame-value getters natural boundary (2026-09-05)

## Scope and disposition

This is a scratch-only reconciliation of the two remaining public Profile frame-value getters from the frozen older Opus lane:

- `_profile_find_frame_value`, January `0x430` / 1,072 padded bytes
- `_profile_frame_get_value`, January `0x3A0` / 928 padded bytes

No production source, header, symbol metadata, park, build, commit, or GitHub state was changed by this work.  The active Fable lane was not read or modified.

The published Profile baseline is canonical commit `f90b7f0c3`, where the unit is `35 exact / 2 residual / 7 unwritten`.  Later independent Transport work does not alter this Profile baseline.

Independent source/ABI/owner review passes the packet at function level, conditional on the ordinary owner-header and full-build checks: `scratch/profile_getters_independent_review_20260905.md`, SHA-256 `a8e04b0e6ee52caf700cf6c1561e4426fc03917b2d04bdd9e9fe7ea2ea7192bd`.  Its independent owner census is `scratch/profile_getters_independent_owner_review_20260905.py` and `.json` (SHA-256 `4b4b5e5382f4e60d9ec4d01fc01a3b4cc9ef4e4f2e05c91583cb2d33e83dc7c4` and `7c01b8bf7f334344dc5202481170ac5833eec24b56213f26536154e9a72ab0a2`).

The natural result is deliberately split:

- `profile_find_frame_value` is a strict exact reconstruction and is a good admission candidate after its real owning-header prototype passes the complete `profile.h` consumer blast.
- `profile_frame_get_value` is source-credible and semantically complete, but remains an honest compiler-schedule residual.  It should be integrated only with the real `seconds_elapsed` type/API repair and parked; no register/lifetime/source-spelling search is warranted.
- The five remaining target-only helper copies have no Profile relocation callers and were not emitted through anchors, false exports, forced inline controls, or dummy calls.

## Frozen evidence

Artifacts:

- January target: `scratch/profile-getters-target-frozen-20260905.obj`, SHA-256 `9ba98a2f5776859e4000ee8d1fd287973d507b4041effaf6403c4b57720f001b`
- canonical baseline: `scratch/profile-getters-base-frozen-20260905.obj`, SHA-256 `8ae70465f8b54f7b7c5f47d5d78f2e606f6ebd3e10a56ba79d8ba5750acb26ed`
- older Opus donor: `C:\halo-worktrees\opus-small-families-30k-20260902\scratch\w3b_profile.obj`, SHA-256 `7b2d900759e969c7ffd7d60ff4ba482e8fc1658655763a52b554cf160d7ebe3a`
- natural final candidate: `scratch/profile-getters-natural-final-20260905.obj`, SHA-256 `86c9ea45c7536b6b4616e70c68bb0512b467785ad60d5a0395a63017f45abf1e`
- target/ABI evidence: `scratch/profile_getters_target_evidence_20260905.json`
- all-object caller census: `scratch/profile_getters_callers_20260905.py`, SHA-256 `0a2b06f1ee8c18a4bc358ca593525a010c3f1b5a88f7d2b1d6488e1c41f101fe`
- pooled `"idle"` owner census: `scratch/profile_getters_idle_owner_census_20260905.py` and `.json`

Natural source inputs:

- `scratch/profile_getters_enum_20260905.inc`, SHA-256 `8c5a7932e7fd47e1dc7a9e848fca90661b73dd015a9615c71044c85cd2860bfa`
- `scratch/profile_find_frame_value_natural_20260905.inc`, SHA-256 `1fc221aa9942806aab054d4e45882f228678e5f5807cec833bed4a7ed71bfe8c`
- `scratch/profile_frame_get_value_natural_20260905.inc`, SHA-256 `6c891f2f1237aa0b34ca694c9ecf33a343b0c65aa235b34a15493348a9bc8df8`
- `scratch/profile_getters_natural_20260905.edits.json`, SHA-256 `7e4767d3d75a8dd542b7a799e6cb544920a9bd4fbcc9feb39a210fcc45efc37b`

The one natural gate was:

```text
python tools/campaign/gate.py source/cseries/profile \
  --edits scratch/profile_getters_natural_20260905.edits.json \
  --all \
  --forbid-emitted-symbol _point_from_line3d \
  --out scratch/profile-getters-natural-final-20260905.obj
```

It reports `36 exact / 3 residual / 5 unwritten`, versus the published `35 exact / 2 residual / 7 unwritten` baseline.  All 35 inherited strict functions and both inherited parks are retained.  The hard `_point_from_line3d` emission guard passes.

## Name and ABI evidence

The January PDB2.00 LABEL census authenticates both public names, plus `_profile_seconds_elapsed`; it provides no surviving function prototype record.

An all-833-object January relocation census finds:

- one incoming call to `_profile_find_frame_value`, from Interface `_code_000cea10` at caller relocation `+776`
- one incoming call to `_profile_frame_get_value`, from the same Interface owner at caller relocation `+1134`
- one incoming call to `_profile_seconds_elapsed`, from Main `_main_update_time` at caller relocation `+1404`
- two calls to `_profile_frame_iterator_new` and four calls to `_profile_frame_iterator_next`, all from the same Interface owner

The Interface caller proves the public APIs more precisely than the frozen donor:

```c
short profile_find_frame_value(
	const char *name,
	short *section_index_reference);

real profile_frame_get_value(
	struct profile_frame_iterator *iterator,
	short frame_value,
	short section_index);
```

The caller passes `name` and the address of a `short` section index, then stores AX into a `short` frame-value slot.  For `profile_frame_get_value`, it pushes three arguments in the order implied above; the callee uses the iterator and frame-value arguments and intentionally ignores `section_index`.  The return is consumed with `fstp`, proving a `real` return.  The older Opus two-argument declaration happens to generate the same callee under cdecl because the third argument is unused, but it is not an acceptable source/API reconstruction.

The frame-value enum is naturally private to `profile.c`: public ABI uses `short`, the parser produces the values, and no external caller uses the enum constants.  Its authenticated numeric order is `frame=1`, `load=2`, `game0..7=3..10`, `player0..3=11..14`, `nonplayer=15`, `render0..render0_3np=16..20`, `render=21`, `game_render=22`, `stall=23`, `texture=24`, `idle=25`, `dt=26`, `gpu=27`, and `pushbuffer=28`.

## Frame schema and `seconds_elapsed` repair

The target getter establishes a `0x1128`-byte frame record and these typed offsets:

- `game_tick_count` at `+0x10`, `window_count` at `+0x12`, `window_ids` at `+0x14`
- frame total at `+0x28`
- game-tick timers at `+0x40`, stride `0x18`
- window timers at `+0xE50`, stride `0x18`
- render/stall/texture/idle totals at `+0xEB0/+0xEC8/+0xEE0/+0xEF8`
- `seconds_elapsed` at `+0xF00`
- rasterizer GPU time at `+0x1110`
- unsigned rasterizer push-buffer size at `+0x1114`

January loads `seconds_elapsed` with `fld dword ptr [frame+0xF00]` and multiplies by 1000.  Main computes a floating value, moves its raw bits to the outgoing argument slot, and calls `_profile_seconds_elapsed`.  The coherent correction is therefore:

```c
real seconds_elapsed;

void profile_seconds_elapsed(
	real seconds);
```

The narrow scratch correction preserves the existing strict `_profile_seconds_elapsed` body.  It must not be hidden behind a duplicate view or a caller-local declaration.  A prior ledger records a broad experiment that combined this repair with an unrelated Main/header change and regressed other objects; that experiment does not isolate this narrow repair.  Admission requires the real prototype in `source/cseries/profile.h` and a complete sweep of all 25 direct C consumers (including Units 189/189).  If that natural owning-header blast regresses, this portion stays withheld; header-position steering or a narrow fake wrapper is not acceptable.

## Natural-function results

### `profile_find_frame_value`

The final body is strict exact at 1,072 padded bytes and 60 relocations.  Its normalized target and candidate hash is:

```text
ed1ef05e1432f759433108cd666ecebca1cc25d995d6f94b51d5d8d5db742ccd
```

The string order, including testing `render` before the `render0*` family while returning numeric value 21, follows January.  It writes `NONE` to the section-index output on every normal return.  The reconstructed assertion expression and line number are target-backed, not adjusted solely for instruction selection.

### `profile_frame_get_value`

The natural body is size/relocation exact at 928 padded bytes and 50 relocations, but instruction-stream residual:

- target normalized hash: `34007f52073a5931791a093800b82b1569907d4fe1c5f5dd5333df7ecaacf121`
- candidate normalized hash: `4e64455e0b5ef659900eee87601b7c65210340e95655975ca17cc2c4ffa461e6`

The meaningful difference is confined to the `render0_3np` loop: January holds `window_count` and player count in different registers and tests the window designator before the address/add schedule; the candidate computes the address/add earlier and then tests the designator.  The streams rejoin afterward.  This is an ordinary allocator/scheduling residual, not a semantic defect.  No lifetime, spelling, declaration-order, or forced-inline search was performed.

The seemingly incomplete switch behavior is target-established: `game1..game7` and `player1..player3` reach the zero/default result, while only `game0` and `player0` have dedicated bodies.  The unused `section_index` formal is likewise established by the three-argument caller and two-argument callee use.  These behaviors were preserved instead of being "improved."

## Complete emitted-owner boundary

The candidate has 39 code owners against 44 target code owners:

- no candidate-only code owner
- no candidate COMMON owner
- no `_point_from_line3d`
- the only target-only code owners are the five previously identified no-out-of-line-caller helper copies: `_code_0007e7e0` (16), `_code_0007e7f0` (80), `_code_0007ee00` (48), `_code_0007f9c0` (48), and `_code_0007f9f0` (96)

The natural find function adds its 28 target-backed name literals.  The getter adds the expected `__real@35aaaaab` and `__real@42055555` constants.  The candidate emits a select-any local owner for the `"idle"` spelling while January Profile references a pooled external owner.  A corrected full census parsed all 833 target and 572 base objects with zero errors: January and canonical both already contain compatible selected owners, and candidate/target/base copies all agree on name, 5-byte payload, zero relocations, flags `0x40301040`, selection 2, storage 2, and normalized hash `133669bfa125619239c71f745155bc3af4996a109ac543231736e9f3df6047c5`.  This is ordinary compatible literal pooling, not new target-owned data credit.

## Production handoff

The minimum coherent production packet is:

1. Put the private `enum profile_frame_value` in `source/cseries/profile.c` under constants.
2. Change only `struct profile_frame.seconds_elapsed` from `long` to `real`.
3. Add the two natural bodies from the frozen `.inc` files at their January source positions.
4. Change `profile_seconds_elapsed`'s definition to accept `real`.
5. In the owning `source/cseries/profile.h`, forward-declare `struct profile_frame_iterator` and add the two public getter prototypes plus `void profile_seconds_elapsed(real seconds);`.
6. Do not add the frozen donor's two-argument frame-getter prototype.
7. Do not expose the private enum or move the complete iterator layout merely to compile this packet.
8. Run all `profile.h` consumer gates/full build, the stable per-owner sweep, Units 189/189, the no-point guard, park validation, and the complete non-code-owner census before admission.

The unwritten Interface caller remains a distinct cluster.  It needs the full iterator/info public layout and exposes a separate known ABI debt: January calls `profile_frame_get_messages` with six arguments while canonical source currently declares/defines only a one-argument placeholder.  This getter packet must not broaden that known-wrong API, invent a duplicate Interface view, or claim the Interface caller is ready.

If the complete field/API packet triggers a genuine consumer regression, the exact `profile_find_frame_value` parser is independently admissible in principle: it depends only on its two-argument owning-header declaration, the private enum, `_stricmp`, and target-backed literals.  It does not reference the frame layout, iterator type, globals, or elapsed-seconds setter.  That smaller packet still requires the ordinary 25-consumer header blast and full owner census; it must not be used to smuggle in a partial or wrong frame-getter ABI.

This separability was also compiled once without changing the body: `scratch/profile_find_frame_value_independent_20260905.edits.json` (SHA-256 `60b9cbdf65ee923e57b842cfefe7f5fd1ac55fbb1d7549aa16caeebcd11e73f7`) produced `scratch/profile-find-frame-value-independent-20260905.obj` (SHA-256 `fcdcc23da46f715b54647da6c40f1a3671a8c1abe96cb6dc73037ea2a18c47ae`).  The result is `36 exact / 2 residual / 6 unwritten`, retains every inherited owner and park, and passes the hard no-point guard.  This was a subset proof, not a source-shape variant or matching search.

## Canonical integration and verification

Root applied the complete natural packet in its owning C/header files, with no
source variant, alias, header-position search, or narrow-header workaround.
The final production gate is **36 exact / 3 residual / 5 unwritten**. The
parser contributes **1,063 meaningful / 1,072 padded exact code bytes**; the
getter is a fresh **90.12195%** unclassified fuzzy park, not exact credit.

The production object SHA-256 is
`1d3acc81be0b5d08100b0307c87fb73ef483bacb0eddd9b4004f7bb18c0370b2`.
`scratch/profile_getters_canonical_owner_audit_20260905.py` and its JSON
repeat the complete owner census on this actual Ninja output: every inherited
code/non-code owner remains unchanged, exactly two intended code owners and
30 ordinary literal/constant owners are added, and there is no COMMON or
candidate-only code. The two function normalized hashes are the same as the
independently reviewed scratch result.

Full Ninja succeeds. The rename-stable sweep over all 8,245 target owners
gains one strict owner and loses none: **6,217 strict owners**. The resulting
snapshot is `scratch/profile-getters-canonical-first-20260905.json`, compared
with `scratch/transport-write-socket-canonical-first-20260905.json`.
Units remains **189/189**; its inherited whole-object helper veto is neither
waived nor broadened. The Profile hard no-point guard passes.

All **288 tests plus 26 subtests** pass; **275 parks** validate with zero stale
or invalid entries. Admission remains **0 candidates / 0 contradictions /
5 rejections / 0 revocations**. Source screening reports only the seven
unchanged low-level timestamp-assembly lines, with no new review lead.

The pre-edit binaries for all 25 direct header consumers are frozen under
`scratch/profile-getters-consumer-baseline-20260905/`.
`scratch/profile_getters_consumer_runtime_audit_20260905.py` compares all
runtime sections, anonymous non-code sections, symbols, relocation ownership,
and COMMON allocations against the actual rebuilt consumers. This is a
bounded binary audit, not a falsely claimed clean-Git regression manifest.
The final metadata adjudication and data-credit attribution are recorded below.

### Header-consumer metadata adjudication

The only differences outside Profile are compiler-local labels in 38 functions
across eight consumers. The independent
`scratch/profile_getters_consumer_local_label_audit_20260905.py` and JSON
prove that all 324 labels are renamed by the same compiler ordinal shift
(+16). Parent owner, offset, storage, type, relocation address/type/addend,
resolved destination and symbolic destination are unchanged. No runtime
section or COMMON owner is lost, added or changed in any other consumer.
The script SHA-256 is
`9e3b9250f44ece7dd9e1a59aeaf0a4a43e0fa0c42a364a7a3663cb477e989522`;
the result hash is
`33174b18d50d744b66e1c2662a862f9f86f25cb2da3bcb8ccf375ccd52af1b74`.
This narrow label adjudication does not waive real symbol or section changes.

### Exact data-credit attribution

The ordinary report credits the merged 2,104-byte Profile `.rdata` section
only when it reaches 100%. Isolated baseline/current reports against the same
frozen target show **89.350914% -> 100%**. Consequently Profile matched data
rises from **1,129,828 -> 1,131,932**, exactly explaining the global **+2,104**
credit without any unrelated-unit gain.

This is not 2,104 newly written data bytes. The packet adds **215 physical
bytes** in 30 named owners: 29 target-local constants totaling 210 bytes and
the compatible pooled `"idle"` literal of five bytes. Previously emitted
data becomes credited when the merged section closes. There is still no
whole-object approval: the inherited global-storage linkage discrepancy,
five missing helper owners and separate incomplete iterator/message API are
not waived by ordinary data credit.

The verified production totals are **906,391 meaningful matched code bytes**,
**6,171 credited functions**, **2,030,868 matched data bytes**, and unchanged
**391/833 Matching objects**. The active Fable lane and every reserved
implementation remain untouched.
