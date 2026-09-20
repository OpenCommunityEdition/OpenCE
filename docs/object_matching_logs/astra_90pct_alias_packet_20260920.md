# Astra 90% campaign: relocation aliases and point helper ownership

Research-only packet at `47d2e7b69f1c1277123e979296b6f0a7de00c80b`.
No source, shared header, symbol, park, comparator, admission, or build-setting
change. No Ninja, commit, or push was run by this worker.

## Finding that changes the campaign queue

The historical **15 functions / 5,856 meaningful bytes** in the csplit A45
alias pool are **already in the current accepted ledger**. They occupy 5,952
padded bytes. This packet contributes zero new exact bytes. Counting the alias
pool as future coverage would double-count inherited credit.

Fresh evidence:

- `scratch/astra-alias-census.py` and `scratch/astra-alias-census.json`;
- the current `build/semantic_report.json` accepted ledger;
- `scratch/astra-alias-pdb/pdb_symbols.json`, `pdb_types.json`, and
  `pdb_delta.json`, freshly extracted from the supplied January PDB;
- direct 32-bit operands from the local `cachebeta.exe` (SHA-256 is recorded
  in the census).

The scan visits all Halo source objects and retains only pure identity
differences after equal padded size, normalized bytes, relocation count, and
every relocation address and type have been established. It resolves names
through the existing fail-closed unique-address mapping. It changes neither
that mapping nor the comparator. It encountered 7,256 ordinary exact controls
and zero object-read errors.

There are 17 pure identity rows. Fifteen resolve fully and equally; all are
accepted. `render_debug_decals` is also accepted but does not fully resolve
through a global name-only map because `_debug_decals` is ambiguous. The only
unaccepted identity row is the existing 224-byte `render_debug_obstacle_path`
semantic veto. No new alias-only candidate remains.

| Accepted family | Functions | Meaningful | Padded |
|---|---:|---:|---:|
| leaf_map | 5 | 1,401 | 1,424 |
| collision_usage | 1 | 243 | 256 |
| ui_widget_game_data_input_functions | 3 | 2,698 | 2,720 |
| hs_runtime | 1 | 139 | 144 |
| hs_compile | 3 | 972 | 992 |
| thread_win32 | 1 | 59 | 64 |
| bitmap_utilities | 1 | 344 | 352 |
| Total | 15 | 5,856 | 5,952 |

The 12 current alias-class parks include accepted ledger entries; park
membership must therefore not be interpreted as absence of credit. The
current campaign inventory must join against `accepted_ledger` first.

## January address evidence

All **17 differing relocation operands** across those 15 fully resolved rows
agree with the literal operand in January's image. Every differing relocation
has type `IMAGE_REL_I386_DIR32` (6). Image base is `0x400000`; PE raw offsets
and RVAs are identical in this supplied image. The script stores operand file
offset, raw value, resolved RVA, both original symbolic records, and any PDB
public-name witness, rather than silently normalizing the production objects.

| Identity pair | Equal RVA |
|---|---:|
| `_faked_xbox_command_line + 4` / `_leaf_map_globals - 4` | 4,982,956 |
| `_collision_usage_current + 2974` / `_global_current_collision_users - 2` | 5,835,422 |
| UI diagnostic string `+ 2` / `_local_player_controller_bitmap_frames + 26` | 2,584,102 |
| `_local_player_index_for_draw_string_and_hack_in_icons` / `_cached_player_profile + 156` | 5,841,532 |
| `__hs_type_scenery_default + 12912` / `_hs_enum_table - 256` | 2,493,140 |
| action-index assertion string `+ 60` / `_hs_tag_reference_type_group_tags - 96` | 2,479,852 |
| action-index assertion string `+ 58` / `_hs_object_type_masks - 86` | 2,479,850 |
| `_transport_address_string + 36` / `_thread_globals + 1580` | 3,263,828 |
| `_bitmap_sharpen_negative_table + 510` / `_bitmap_sharpen_positive_table - 2` | 3,260,670 |
| `_extract_data + 42` / `_bitmap_sharpen_negative_table - 2` | 3,260,158 |

The January PDB freshly confirms both public names for collision_usage, the
profile-cache boundary, and the HS pairs. It confirms the UI diagnostic
string. It does not publish every private leaf_map/bitmap/thread owner name;
their names are inherited reconstruction evidence, not new PDB discoveries.
The raw image operands independently confirm the destinations used by the
existing mapping. Address equality does not by itself recover a private
source name or permit indexing a preceding unrelated object in source.

The 2026-09-15 collision_usage ledger contains an arithmetic typo:
`5832448 + 2974` equals **5835422**, not 5835424. The latter is the array base,
before its `-2` bias. Current configuration, the new PDB extraction, and the
actual image operand agree on the correct destination.

`render_debug_decals` demonstrates why duplicate-name resolution must remain
fail-closed. January's relevant operand targets RVA 4,414,816, whereas the
PDB public `_debug_decals` label is at 5,842,644. Selecting one same-named
record globally would erase a genuine ownership distinction. The existing
224-byte obstacle-path veto is likewise preserved: its proposed coherent
globals lack independent image-name address evidence, and source aliases to
raw BSS owners remain inadmissible.

## Fresh point_from_line3d COMDAT evidence

Evidence scripts and results:

- `scratch/astra-alias-comdat.py`;
- `scratch/astra-alias-comdat-census.json`;
- `scratch/astra-alias-comdat-summary.json`.

The wrapper imports the archived, read-only PDB/COMDAT inspectors from
`C:\halo-worktrees\fable5-comdat-research-20260917\tools\campaign` and runs
them against this worktree's current objects and the supplied January PDB.
All **22,455** existing contribution records agree with the freshly parsed
PDB on image offset, module, size, and flags.

`_point_from_line3d` is selected at file offset/RVA **5504**, in
`source/ai/action_charge`, DBI module **470**, contiguous `.text` run rank
**2**. It has one selected defining split object and 17 current undefined
referrer objects. The selected owner's external caller is
`_action_charge_perform`.

There are six later current emitters and none earlier:

- `source/ai/ai_debug`;
- `source/math/real_math`;
- `source/physics/bsp3d`;
- `source/physics/collision_features`;
- `source/render/render_sky`;
- `source/units/units`.

All seven emitted bodies strictly agree with the selected January 48-byte,
zero-relocation body, SHA-256
`9b763841f8519177ac2d3fd471226972b9fa92cf1e9061e9f92359077dc82741`.
The **current reconstructed** action_charge owner uses selection 1
(`NODUPLICATES`), while each later copy uses selection 2 (`ANY`). This is
not evidence that the original raw owner had selection 1: the final image
and PDB do not preserve discarded raw input COMDAT selection metadata.

The independent `_cross_product3d` control is selected in action_obey,
module 465, rank 7. It has 45 later current emitters, none earlier, and 13
undefined referrers. Forty-five of its 46 current emitted bodies match the
selected 64-byte body; breakable_surfaces is the exception. A blanket rule
that all later same-named copies have identical bytes would therefore fail.

The broad census contains 93 first-copy-consistent helpers, three
counterexamples (`_power`, `_random`, `_set_real_quaternion`), 14 helpers
absent from January's name table, and 15 unresolved owners. This validates
first-copy consistency for the point helper but rejects a universal law
that current raw helper emission reconstructs every January input object.
The archived inspector's rank is derived from PDB-proven contiguous `.text`
module contributions; its `FIRST-COPY-CONSISTENT` result is compatibility
evidence, not proof of the exact original linker command line.

## Boundaries and do-not-repeat decisions

The previous primary investigation remains relevant:
`point_from_line3d_link_selection_evidence_20260919.md`. Its VC7 matrix
established that ordinary, static, external, and prototype-before-inline
forms all emit an object-local helper when used for inlining. Force-inline
controls also emitted it and are independently inadmissible. The fresh
census supplies no new authentic source mechanism that suppresses the raw
later COMDAT while retaining the required inline schedule.

Do not repeat:

1. Source rewrites of the 15 already accepted alias rows, negative indexing
   through an unrelated previous object, or duplicate-name address guessing.
2. Global cross_product3d direct-store replacement: the prior complete
   consumer experiment cost 37 inherited exact functions for one 480-byte
   gain and was restored.
3. Renamed helper duplicates, manual helper expansion, forced inline/noinline,
   or raw-object guard removal to admit point-dependent candidates.
4. Treating final-image absence of discarded code as proof that original raw
   objects contained no duplicate inline bodies.
5. Reopening the 224-byte semantic veto from address-shape equality alone.

The available live Ghidra RPC was tested and refused the connection at
127.0.0.1:18081. No live Ghidra name or source-provenance claim is made by
this packet. The prior IDA/Ghidra investigation reports no additional local,
type, storage-declaration, or inline-site record for this helper. This packet
uses fresh PDB, PE, and COFF evidence, and preserves that limitation.

## Independent atlas accounting review

`scratch/astra-alias-atlas-review.py` and its JSON identify a reporting bug in
the first generated atlas: `coff_compare.section_info` returns the entire
parent section for an embedded `$L` or jump-table symbol.

The correct physical padded accounting deduplicates `(unit, section)` and
counts a section once when an accepted offset-zero owner exists. It keeps
the canonical accepted-ledger meaningful bytes and function counts intact.

- Halo accepted: **1,489,642 physical padded bytes / 7,272 distinct owner
  sections**; the canonical ledger still has 7,281 entries and 1,431,592
  meaningful bytes. Nine accepted embedded labels inflated the first atlas
  by 11,248 padded bytes.
- Overall accepted: **1,511,627 physical padded bytes / 7,539 sections**.
  Five SDK sections have valid offset-zero aliases totaling 398 bytes;
  `function_sections()` deliberately omits them as ambiguous, so using it
  alone would undercount the overall physical total.
- The canonical 293 unresolved Halo report rows contain **289 section
  owners plus four embedded records** (436 meaningful bytes): the
  ai_debug actor jump table (388), and units `$L7451` (8), `$L7450` (24), and
  `$L8061` (16). The units parent functions are already accepted; the actor
  parent is residual.
- Genuine unwritten owners: **7 / 3,733 meaningful / 3,784 padded**.
  The canonical missing-symbol group remains 11 / 4,169 meaningful.
- Written unparked owners: **112 / 188,188 meaningful / 189,408 padded**.
  `_projectile_collision_test_line` has a body despite zero objdiff score;
  a zero fuzzy percentage is not evidence that it is unwritten.

Recommended row fields are `symbol_offset`, `embedded_owner`,
`parent_section_size`, and distinct `owning_section_padded_bytes`; an embedded
record has no additional owning-section padding. These are reporting
corrections only, and cannot create coverage credit.

## Concrete disposition

Remove the historical alias pool from gain projections and retain its current
credit. Correct padded inventory accounting before selecting another wave.
Preserve all parks and the point emitted-symbol guard. No admissible
production edit is proposed by this packet, and no broad source spelling
search is justified by its results. A future point-related opening requires
new original source/input-object provenance that satisfies the existing
no-surplus rule; the current evidence does not supply it.
