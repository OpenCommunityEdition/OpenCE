# Encounters initialization caller-cluster reconciliation (2026-09-05)

## Scope and result

Canonical baseline is `5c22509d85270a056524b6a7eeb22f05a548a9e3`.
The genuine unwritten chain consists of public
`encounters_initialize_for_new_map` and private `encounter_new` and
`squad_reset_starting_locations`: 996 meaningful / 1,024 padded target bytes.
The initial object has 43 exact, four residual and fourteen unwritten functions.
The final natural packet has **45 exact, five residual and eleven unwritten**:
the initializer and reset gain **344 meaningful / 368 padded exact bytes**;
the 652-meaningful-byte constructor is honestly parked at **98.40206%**.
All inherited exact and runtime owners are preserved. No whole-object
completion or new matched-data credit is claimed.

Older completed Fable/Opus evidence is used read-only. The active Fable
worktree and its reserved implementations remain excluded. Canonical
`ai_script.obj` is a verification-only header consumer; its active-lane source
has not been inspected, changed, or certified unaffected.

## Genuine shared-schema prerequisite

The one preselected owner-header edit adds the PDB-named
`_platoon_initially_defending_bit = 2` and
`_actor_starting_location_required_bit = 0`, exposes the actual
`unsigned long flags` at platoon-definition offset 0x20, and reduces the
unrecovered tail from 0x8C to 0x88 bytes. Natural size/offset assertions record
size 0xAC and flags offset 0x20. No other later platoon field is imported.
Natural alignment changes from one to four; this is disclosed, not suppressed
with packing or a caller-local facade.

Root read the seven prerequisite Encounters ledgers and independently inspected
the complete January disassembly of all three functions. Direct target field
widths, access offsets, stride and bit operations agree with the original later
PDB. The primary evidence and reproducer are recorded in
`encounters_obj_platoon_schema_starting_location_bounds_20260905.md`.

Root accepts the valid-tag starting-location bound of 0 through 32 as a
corroborated cross-build inference: the shipped Sapien descriptor is actually
linked from the squads block and explicitly has maximum 32 / stride 0x1C;
the PDB and January accesses agree on one-word banks at runtime squad offsets
0 and 4, followed by `major_upgrade_error` at 8. Root reran the descriptor
reproducer and checked the parent-field identity. No direct January maximum
descriptor was recovered. Count 33 is outside this accepted input domain and
would corrupt adjacent state in the target. No extra cap/assertion, widened
bitset, or layout adjustment is invented. The separate 64-bit temporary vector
in an existing getter is not evidence of 64 persistent locations.

## Frozen header-only trial

The actual Ninja dependency census and direct include census agree on thirteen
canonical consumers. The before/after objects are frozen under
`scratch/encounters-schema-before-20260905/` and
`scratch/encounters-schema-after-20260905/`; their corresponding JSON inventories
record each path and SHA-256. Capture script:
`scratch/encounters_schema_capture_20260905.py`.

The header SHA-256 changes from
`395def4c448223f4c44e912d170333ffe111a93722d8929bec503fe5e62ca94b` to
`5424e4e887430e9def8e23dde5b1f7c02da0662a11f31a5bd80a40e1fd460618`.
Encounters source remains
`eca5fe872fe65e7cf8f219edd1bd25f72ec5ccbee4fef877194a9cc6ef93cccd`.

All thirteen objects compile. The complete 8,245-owner snapshot preserves all
6,217 strict owners with zero gains and zero regressions. The first full Ninja
stops correctly at progress validation because the inherited
`ai_communication_update_speech_timers` fuzzy park becomes stale. It is not
silently ignored or counted exact.

Root's frozen before/after disassembly comparison finds only the reversal of
two independent loads in that function at offsets 0x167 through 0x16E:
`mov ebx,[ebp-8]` and `mov di,[ebp+0x10]`. Both versions have 210 instructions,
672 padded bytes and 43 relocations. The body source has not changed. The new
normalized hash is
`7db4861a5adf519e0457a0ffe785ff4d47e878fda47cc0a06206a529776ac91c`, and
ordinary similarity is 97.76382%, previously 97.67839%. Complete independent
metadata/owner review remains required before refreshing that evidence.

The scoped tooling suite passes: `python -B -m pytest tools -q
-p no:cacheprovider --basetemp scratch/encounters-schema-tools-pytest-20260905`
reports 288 tests plus 26 subtests. An earlier unscoped pytest invocation
incorrectly collected unrelated scratch donor tests and inaccessible historical
pytest folders; its collection errors were not production-test failures and
did not modify production. Subsequent campaign test commands explicitly name
`tools`.

No header-position, enum-order, spelling, compiler-option or source-shape
variant has been attempted. The header trial alone is not a publication or
whole-object admission claim.

### Independent frozen schema audit and park adjudication

The all-section report is
`scratch/encounters-schema-runtime-audit-support-20260905/summary.json`, SHA-256
`8a2ec89fd058e9bdaa7106828b7ba7d63f54015bef90268913c45a256bf0729a`.
Its adapter is
`scratch/encounters_schema_blast_runtime_audit_support_20260905.py`.
Root read the complete adapter and the summary and independently inspected
the speech-timer code change.

Across thirteen objects, 1,875 sections are unchanged, seven have only
compiler-local-label spelling changes, and the eighth changed section is the
previously described two-load speech-timer transposition. There are no runtime
or debug additions/losses, genuine function/helper owners, COMMON changes,
definition transitions, normalized symbol additions/removals, or new
`point_from_line3d` definitions. The 138 raw symbol-name pairs are solely
compiler-local spelling; owner metadata remains unchanged.

All incoming branches to the changed speech block still target its start
0x167 or its rejoin 0x16E; none enters either instruction's interior or the
second reordered load. The two ordinary stack reads commute on the function's
valid frame. Root accepts this semantically equivalent generated-code change
and updates only this park's measured hash, percentage and explanation. This
is honest evidence maintenance for a real owner-schema addition, not a code
credit, source-shape search, or exception to exact matching.

With that reviewed park measurement refreshed, the full Ninja build passes,
all 281 parks validate, and admission remains zero candidates/contradictions,
five recorded rejections and zero revocations. The changed-header fake-match
scan reports zero leads, and diff checking is clean. Counts remain 906,391
meaningful code bytes, 6,171 credited functions, 2,031,312 matched data bytes
and 391/833 Matching objects. The header prerequisite is accepted canonically;
the actual initialization bodies subsequently passed the separate checks below.

## First natural caller-cluster emission

Root reviewed and applied the complete source-only proposal without changing
its function bodies. The reviewed semantic defaults and private declarations
stay in the owning C file; the public initializer already belongs to
`encounters.h`. No public API or compiler option is invented.

The original source proposal is
`scratch/encounters_initialize_cluster_natural_20260905.inc`, SHA-256
`dc50e8cf84616d33ffefbd80284956ee889992a389bf155da757c14bbb9b780d`.
The unique four-edit production manifest is
`scratch/encounters_initialize_cluster_production_20260905.manifest.json`,
SHA-256 `8d4c1fb902801545a441fbf4c83effac9b754bb3acd88eff880fa7896522f0c1`.
Root verified every find-anchor once and no overlapping edits, then added
descriptive labels to the unchanged edit pairs for the gate's triple format.
No scratch include or preamble enters production.

The first PID-isolated object is
`scratch/encounters-initialize-cluster-first-natural-20260905.obj`, SHA-256
`4d921a37f50e65fb8fdde426d9a93b8b4d208d5de35ee77e8d8c992578cd8649`.
It gates 45E/5R/11U and passes the complete forbidden-symbol check for
`_point_from_line3d`. The two additions are:

| Function | Meaningful / padded bytes | Relocations | Result |
| --- | ---: | ---: | --- |
| `squad_reset_starting_locations` | 198 / 208 | 8 | strict exact |
| `encounters_initialize_for_new_map` | 146 / 160 | 11 | strict exact |
| `encounter_new` | 652 / 656 | 31 | natural fuzzy, 98.40206% |

The constructor's simple short-valued respawn default compiles to a direct
short assignment rather than January's EAX selection. That saves three
instruction bytes and shifts subsequent branches/relocations while padded size
stays 656. Its 30.0 constant is an already-existing genuine candidate owner,
where January has an external reference. No ternary, lifetime, declaration,
register, source-order or compiler-control variant was tried to close either
difference. This is a semantically complete fuzzy park, not claimed exact.

All three functions retain the actual cseries/Halo types, typed tag and datum
accessors, signed-short loop/counter widths, full salted encounter handle,
original assertion paths/messages/lines, bit-vector macros and explicit final
returns. The two inferred semantic names for the 999 constants are disclosed
as inferred names; their values and gameplay roles are directly in January.

## Independent runtime and complete ownership verification

The independent integrated test executes the real public caller, both real
private helpers, actual header accessors and actual `__ftol2`. It passes
**49/49 cases** covering valid counts, allocation failure, flags, fractional
timers, respawn defaults, squad/platoon limits and all four overflow assertions.
Complete state and ordered external events match January. Root read the full
harness and ledger, then reran all 49 cases against the actual canonical
emission; all pass. See
`encounters_obj_initializer_cluster_runtime_review_20260905.md` and
`scratch/encounters-initializer-canonical-differential-20260905.json`.
The bounded input exclusions remain explicit in that ledger.

The canonical object is frozen at
`scratch/encounters-initialize-cluster-canonical-first-20260905.obj`, SHA-256
`b3854f0845f6b0366892764a8c0a21b399e8454a339ceac582b87efa6b829aa1`.
Its runtime records are identical to the first PID candidate. The one
non-runtime difference is the debug record's output-object path/length;
COFF timestamp changes are likewise not code changes. The complete comparison
is `scratch/encounters-initializer-actual-audit-20260905.json`, SHA-256
`41a38802d66b3ced1e1b6011af964ad8598c278cf6069b5fa409a445af1c7bac`.

Root's all-section pre-body-to-canonical audit is
`scratch/encounters-initializer-all-section-audit-20260905.json`, SHA-256
`3a58b82caa3368315a9c57e4dde8ff86f1fa699a7dd509394ac79379fa37d5e9`.
It preserves 107 section records exactly; the sole changed old runtime record
is compiler-local-label spelling in `encounter_create`. Its 736-byte body was
already 736 bytes before this wave and has no normalized instruction change.
There are no lost runtime sections, COMMON changes or definition transitions.

The only seven additions are the three real functions and four authentic
constructor assertion strings. Independent owner comparison in
`scratch/encounters_initialize_cluster_owner_audit_20260905.json` preserves all
51 pre-existing code and 49 noncode owners, their linkage/properties, and all
four inherited COMMONs. The four new literals are strict/property-equal to
their actual January owners: 39, 41, 67 and 71 bytes, totaling 218 physical
bytes. They create **zero additional aggregate matched-data credit**. No
new helper or incompatible duplicate owner is introduced. Inherited extra
helpers/COMMON and other unfinished functions are not newly approved as a
whole-object completion.

The final schema audit, including explicit zero indirect-branch counts in the
speech-timer function, is
`scratch/encounters-schema-runtime-audit-support-20260905-v3/summary.json`,
SHA-256 `c7a26496b8956fd0281c45023effa5e02f1a4e190f439e230fe8b628723f2932`.
Its final adapter SHA-256 is
`0e0151208f1233cede393a13459acc80080ecb97ae617532ded0cfc3f12786fe`.
Earlier report versions are preserved, not overwritten.

## Final canonical verification

Full Ninja and the complete stable-owner sweep pass: **6,219 / 8,245 strict
owners**, two gains / 368 padded bytes and zero regressions. All **288 tests
plus 26 subtests** pass. Both changed source/header files have zero fake-match
review leads, and diff checking is clean. The constructor park increases the
validated park count to **282**, with no stale or invalid entries. No
completion label or matcher exception is changed.

Final admission remains zero candidates, zero contradictions, five recorded
rejections and zero revocations. The protected Bitmap hashes remain
`ab6002bf2baecdbc18c7b308bb7846c58fbcf1c7c72f586621d2b1f271d49140`
for `bitmap_utilities.c` and
`b435f38e406c7dcdb60b4df12e78bcf98d1e4b0e57fe2ab2240bc78f799eba93`
for `bitmaps.h`; the 189-exact Units sentinel is unchanged in the full sweep.

Verified canonical accounting becomes **906,735 meaningful matched code
bytes**, **6,173 credited functions**, **2,031,312 matched data bytes**, and
**391/833 Matching objects**. Final `encounters.c` SHA-256 is
`20143a0b6da455da20b1336e5bd6f2f7f7ea1171b531d45e5aa9382eea055828`.
The active Fable lane remains excluded; its later reconciliation must account
for the genuine shared scenario-schema addition rather than assuming its
uninspected source is unaffected.
