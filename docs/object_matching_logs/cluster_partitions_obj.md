# `source/structures/cluster_partitions.obj` strategy ledger

This retroactive ledger preserves the evidence available from Git,
`config/parked.json`, source comments, and the current report. The original
lane predates mandatory experiment matrices; missing per-variant hashes are
called out rather than reconstructed.

## Scope and baseline

- Compiler: XDK 3911 `CL.Exe` 13.00.9254.1,
  `/O2 /Oy- /DDEBUG /Dxbox`.
- January target:
  `build/split/source/structures/cluster_partitions.obj`.
- Current inventory: 14/14 target functions exact; 1,808/1,808 padded code
  bytes, with every relocation address, type, destination, and addend exact.
- All 14 target-owned `.rdata` COMDATs are strict-exact: 476 meaningful bytes
  (500 aligned bytes in the ordinary report), zero relocations. The object has
  no target-owned `.data` or `.bss`.
- `_reference_list_copy` was the final residual and is now resolved.

## Historical parked residual (resolved)

| Function | Size T/B | Relocs T/B | Hash T/B | Measured residual | Class |
|---|---:|---:|---|---|---|
| `_reference_list_copy` | `0xC0/0xC0` | `9/9`, including addresses | `28ab903006bf625bb291ffe9b82a710eabf80400b9d1f2403b78b04429904c8b` / historical `dcebabeded0e38211be7325fc38d98591a621f800556829f6184e38dbacf7c03` | VC7 assigned source and destination pointer live ranges to opposite registers. | resolved by coalescing order |

## Preserved experiment history

Eight legal-C source shapes were measured by the historical closeout lane.
The surviving record proves they retained equal size and relocation identity
without breaking the pointer-register mirror. The exact eight source diffs
and transient hashes were not preserved, so this ledger deliberately does not
invent them.

Do not repeat blind declaration-order or source/destination pointer-alias
spelling sweeps. Do not use `volatile`, assembly, byte patches, undefined
behavior, or compiler-flag changes.

## Resolution

Commit `d84c4d13` established the reachable source control. Both changes are
required:

- initialize `absolute_index` before deriving the cursor pointers, so the
  counter claims ESI before the source value dies; and
- advance `source_reference` before `result_reference`, preserving the source
  cursor in EBX while the result cursor spills.

That produces the target hash
`28ab903006bf625bb291ffe9b82a710eabf80400b9d1f2403b78b04429904c8b`
with 192/192 padded bytes and 9/9 exact relocations.

## Full-object admission audit (2026-08-11)

Fresh XDK 3911 rebuild and `section_infos_equal` sweep:

- target functions: 14; candidate shared functions: 14; exact: 14/14;
- padded target code: 1,808/1,808 bytes;
- target-owned non-code: 14/14 `.rdata` sections, 476/476 meaningful
  bytes, exact normalized hashes and relocation identity;
- `.data` / `.bss`: absent in both target ownership inventories.

The candidate also emits three unreferenced out-of-line inline-helper COMDATs
(`reference_list_new`, `reference_list_get_next_datum_index`, and
`reference_list_add`) and four pooled string COMDATs. These are not unique
object-owned content:

- every extra section uses `IMAGE_COMDAT_SELECT_ANY`;
- no relocation in `cluster_partitions.obj` calls the three extra helper
  bodies because all calls were inlined;
- the selected January owners are `source/hs/object_lists.obj` for the three
  helpers and two helper strings, `source/objects/objects.obj` for
  `"location"`, and `source/camera/editor_flying_camera.obj` for
  `"position"`;
- each candidate duplicate is itself strict-exact against that selected
  January owner, including helper relocations.

This is the documented discardable-COMDAT case, not extra ordinary code or
data. Parameter declarations were reformatted to the house rule (one
parameter per line); the exact function source retains explicit final returns
and the target-required early exit in `reference_list_remove`.

## Final validation

- `ninja halobetacache_build`: exit 0 after 465 actions.
- `ninja progress`: strict semantic audit passed and admitted this object;
  Halo progress became 3,412/7,574 accepted functions, 413,891/1,770,166
  code bytes, 1,793,884/3,923,451 data bytes, and 255/468 objects.
- Post-progress `coff_compare` recheck: all 14 functions remain exact.
- Comparator, regression-gate, semantic-audit, and semantic-progress tests:
  85/85 pass.
- A clean pre-edit manifest reports all 14 functions `still_exact` and no
  changed non-exact function. Its only hard differences are the intentional
  `config.json` admission and anonymous `.debug$S` metadata: the clean
  baseline and candidate were built in different isolated worktree paths,
  and the header-only parameter formatting changes line records. Neither
  changes target-owned code, relocations, ordinary data, or symbol ownership.
  This caveat is recorded rather than hidden or adjudicated away.

Disposition: strict-complete, full-build clean, and admitted as `Matching`.
