# Encounters platoon schema and starting-location bounds (2026-09-05)

This is a read-only primary-evidence review for the unwritten 1,024-padded-byte
`encounters_initialize_for_new_map` / `encounter_new` /
`squad_reset_starting_locations` caller cluster. It performs no compilation,
changes no production source/header/configuration, and claims no progress.
The active Fable lane and its contents were not inspected.

## Disposition

The platoon flags member and the two required flag names are directly
authenticated. Exposing `unsigned long flags` at offset 0x20 in the owning
`ai_scenario_definitions.h` is justified, subject to root's full dependent
rebuild and ownership checks. The remainder of this partially recovered
definition must stay opaque; no later unrelated fields are required.

The persistent starting-location banks really are one 32-bit word each, not
two words. Valid-tag counts of 0 through 32 are strongly corroborated by an
actual shipped Halo CE editor tag descriptor, the later original PDB layout,
and January's accesses. **A January tag-definition maximum was not directly
recovered.** The claim that January shares the 32-location valid-input
invariant is therefore an explicit cross-build inference, not a quoted January
constant or an independently verified January tag descriptor.

This evidence supports a source draft using the existing one-word banks with
that documented valid-tag precondition. Root must explicitly accept this
inference before admission; if direct January maximum evidence is required,
the body stays withheld. Do not enlarge the banks, clamp counts, fabricate an
assertion, or interpret the separate 64-bit temporary vector as permission for
64 persistent locations.

## January primary evidence

Canonical root throughout:
`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/rasterizer-model-wrapper-alias-20260829`.

- `cachebeta.exe`, SHA256
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- `build/split/source/ai/encounters.obj`, SHA256
  `7edc1863eb2c08f7f2ad612e3cfa7b246c0eb9a3b5907fd08c2c17ecd9a056a3`.
- Reproduce readable target instructions with
  `python -B tools/audit/dump_coff_disasm.py build/split/source/ai/encounters.obj _encounter_new`
  and the same command with `_squad_reset_starting_locations`.

In `encounter_new`, function-relative +0x264 passes element size 0xAC to the
tag-block accessor. At +0x270 the result is read as a dword at +0x20; +0x273
shifts right two; +0x276 masks bit zero; and +0x27D stores the resulting byte
to the platoon runtime datum's first member. This proves the field width,
offset, bit index, and containing tag stride without relying on donor C.

In `squad_reset_starting_locations`:

| Offset | Actual January behavior |
| --- | --- |
| +0x58 | Load starting-locations count from squad definition +0xD0. |
| +0x64..+0x6A | Compute `((count + 31) >> 5) * 4`. |
| +0x6E..+0x74 | Fill that many bytes with 0xFF beginning at runtime squad +4. |
| +0x87 | Access starting-location elements with stride 0x1C. |
| +0x90..+0x99 | Read byte flags at +0x13 and test bit zero. |
| +0x9B..+0xAF | OR the required bit into the bank beginning at runtime squad +0. |
| +0xB1..+0xBD | Advance a signed-short index and compare against the complete block count. |

There is no local upper-bound guard in this function. The routine does not
clear the required bank itself; its genuine initialization caller clears the
whole runtime squad array first. Other January constructor stores authenticate
later datum offsets, including `respawn_actors_left` +0x0C,
`automatic_migration_target` +0x10, `delay_timer_started` +0x11, and
`delay_timer` +0x12. Enlarging either bank would contradict these accesses.

For count 0 the unused-bank write is empty. For counts 1 through 32 it touches
only bytes +4..+7. Count 33 already writes eight bytes starting at +4,
overwriting the next named field at +8; required bit 32 also aliases the unused
bank. This is a semantic boundary, not an incidental code-generation gap.

## Original later PDB: exact names and layouts

Primary PDB:
`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research/hcea_jun2011_prototype/payload/Halo CE Anniversary (Jun 24 2011)/HCEX.pdb`.
SHA256:
`f55cfe957da8079a62a26a6753f1ddbb700b067a66de0bd5db5aac182d985ff1`.

Tool:
`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research/tools/DIA2Dump/x64/Release/DIA2Dump.exe`.
Queries use `-type platoon_definition`, `-type squad_datum`,
`-type actor_starting_location_definition`, and
`-sym _platoon_initially_defending_bit` /
`-sym _actor_starting_location_required_bit` before the PDB path.

Direct DIA output confirms:

- `platoon_definition.name`: `char[0x20]`, offset 0.
- `platoon_definition.flags`: `unsigned long`, offset 0x20.
- `_platoon_initially_defending_bit`: unnamed-enum constant 0x2.
- `actor_starting_location_definition.flags`: `unsigned char`, offset 0x13.
- `_actor_starting_location_required_bit`: unnamed-enum constant 0x0.
- `squad_datum.required_locations`: `unsigned long[0x1]`, offset 0.
- `squad_datum.unused_locations`: `unsigned long[0x1]`, offset 4.
- `squad_datum.major_upgrade_error`: `float`, offset 8 (canonical spelling
  remains `real`, not the PDB printer's primitive spelling).
- Subsequent squad members agree with January's stores through the real at
  offset 0x1C, confirming the 0x20 runtime stride.

The extracted January type JSON at
`../../research/pdb200/pdb_types.json` contains 1,836 types but no actor,
encounter, squad, platoon, or scenario type record. Its absence is not evidence
for a different limit. A bounded HCEX global-symbol name search also found no
named maximum-starting-locations constant; no constant name is invented here.

## Shipped editor descriptor: maximum 32, correctly linked to squads

Primary binary:
`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research/halo_hek_oracle/extracted/sapien.exe`.
SHA256:
`f980a4fde0033031b914ba67c5b7e76b6f20c971a112e7e558153f42f959cffa`.
This is a later Halo CE editor binary, **not** the January target.

The unique `actor_starting_locations_block` string is at file offset 0x51F82C,
VA 0x91F82C. Its unique pointer reference begins the tag-block descriptor at
file offset 0x635390, VA 0xA35390. Its eleven dwords are:

```text
0091F82C 00000000 00000020 0000001C 00000000 00A35318
00000000 00000000 00000000 00000000 00000000
```

Using the actual `tag_block_definition` layout, this is name, flags 0,
maximum element count **32**, element size **28**, no default element,
fields pointer 0xA35318, and zero callback/byte-swap pointers.

This is not merely a same-named disconnected string. The definition is
referenced by a type-34 block field at file offset 0x6357D0 / VA 0xA357D0,
whose name is `starting locations`. It is field index 32 in the field table
at VA 0xA35650 owned by `squads_block`, descriptor VA 0xA357F4. That parent
descriptor has maximum count 64 and element size 0xE8, matching January's
squad tag stride. The preceding real block is `move positions`, and a
12-byte pad and terminator follow, matching the known squad schema tail.

Reproduction is a read-only script (prints JSON, writes no output files):
`scratch/encounters_schema_bounds_evidence_20260905.py`, SHA256
`8ceedad82d6f6d50f7a72fe12b02d3a4dc990767a23400df0fb7ab3d5acab783`.
Run it from canonical with Python. It verifies the descriptor's unique pointer
identity, parent-field linkage and stride, and emits the count 0/1/31/32/33/64
write-boundary model. The script separately records that January cachebeta.exe
has no `actor_starting_locations_block` string; this does not prove that every
possible unnamed representation of a January descriptor is absent.

## Minimal owner proposal and withheld checks

Only the following newly consumed schema is justified in
`source/ai/ai_scenario_definitions.h`:

```c
enum
{
    _platoon_initially_defending_bit = 2,
};

enum
{
    _actor_starting_location_required_bit = 0,
};

struct platoon_definition
{
    char name[TAG_STRING_LENGTH+1];
    unsigned long flags;
    unsigned char reserved[0x88];
};
```

Size remains 0xAC; natural alignment changes from 1 to 4. Root must preserve
all inherited exact owners across the complete header consumer graph, and
must not try declaration-position variants if this genuine definition fails.
Size/offset assertions may document the authenticated layout. No custom
packing, wider squad vectors, caller-local prefix structures, raw offset
access, or additional source inlining is proposed. Source bodies should use
the existing cseries bit-vector macros and named flags.

This review read the full triage plus all seven prerequisite Encounters
ledgers: the 20260821 iterator/setter and actor-iterator waves, 20260830
encounterless activation and dirty-status waves, 20260902 Fable salvage,
20260903 Opus two-leaf reconciliation, and 20260904 remaining-leaf
reconciliation. Their old declaration-placement/tuning history is provenance,
not current permission to bypass the house rules. The existing exact getter's
64-bit temporary vector and its local name were deliberately not changed or
treated as a tag-capacity guarantee.

No Ninja, gate, production object, source/header, metadata, Matching label,
park, or Git commit was changed by this research. Any schema-only trial run by
root in parallel is a separate artifact and is not claimed verified here.
