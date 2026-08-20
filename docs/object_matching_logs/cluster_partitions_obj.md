# `source/structures/cluster_partitions.obj` strategy ledger

This retroactive ledger preserves the evidence available from Git,
`config/parked.json`, source comments, and fresh strict COFF measurements. The
original lane predates mandatory experiment matrices; missing per-variant
hashes are called out rather than reconstructed.

## Scope and baseline

- Compiler: XDK 3911 `CL.Exe` 13.00.9254.1,
  `/O2 /Oy- /DDEBUG /Dxbox`.
- January target:
  `build/split/source/structures/cluster_partitions.obj`.
- Current inventory: 14/14 target functions strict-exact; 1,808/1,808 padded
  code bytes, with every relocation address, type, destination, and addend
  exact.
- All 14 target-owned `.rdata` COMDATs are strict-exact: 476/476 logical bytes
  (500 aligned bytes in the ordinary report), zero relocations. The target
  object owns no `.data` or `.bss`.
- `_reference_list_copy` was the sole residual and is now resolved.

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

## Resolution and provenance

Claude-assisted commit `d84c4d13` established the reachable legal-C control.
Both source-order changes are required:

- initialize `absolute_index` before deriving the cursor pointers, so the
  counter claims ESI before the source value dies; and
- advance `source_reference` before `result_reference`, preserving the source
  cursor in EBX while the result cursor spills.

That produces the target hash
`28ab903006bf625bb291ffe9b82a710eabf80400b9d1f2403b78b04429904c8b`
with 192/192 padded bytes and 9/9 exact relocations.

## Full-object admission audit (2026-08-20)

Fresh isolated XDK 3911 rebuild and the hardened regression fingerprint:

- target functions: 14; candidate shared functions: 14; strict-exact: 14/14;
- padded target code: 1,808/1,808 bytes;
- target-owned non-code: 14/14 `.rdata` sections, 476/476 logical bytes,
  exact normalized hashes, symbols, flags, and relocation identity;
- target-owned `.data` / `.bss`: absent.

The candidate also emits three unreferenced out-of-line inline-helper COMDATs
(`reference_list_new`, `reference_list_get_next_datum_index`, and
`reference_list_add`) and four pooled string COMDATs. These are not unique
object-owned content:

- every extra candidate section uses `IMAGE_COMDAT_SELECT_ANY`;
- no relocation in any of the 14 target functions calls the three extra
  helper bodies because all calls were inlined;
- the selected January owners are `source/hs/object_lists.obj` for the three
  helpers and two helper strings, `source/objects/objects.obj` for
  `"location"`, and `source/camera/editor_flying_camera.obj` for
  `"position"`;
- every candidate duplicate is freshly strict-exact against that selected
  January owner, including helper relocations.

This is the documented discardable-COMDAT case, not extra ordinary code or
data. Parameter declarations follow the house rule (one parameter per line);
the exact function source retains explicit final returns and the
target-required early exit in `reference_list_remove`. Typed tag/object access
rules do not apply because this translation unit performs neither kind of raw
access.

## Validation completed before admission

- Forced `cluster_partitions.obj` XDK compile: exit 0.
- Hardened function comparator: `all_equal: true`, 14/14 functions.
- Fresh target-owned section/ownership fingerprint: all 14 runtime sections
  strict-exact, with no target-only section.
- Full `halobetacache_build`: exit 0 after 466 actions.
- Full `libcmt_build`: exit 0 after 102 actions.
- Strict semantic report: 470 units scanned, 3,604 functions evaluated, zero
  unit errors; the admission audit found `cluster_partitions` as the sole
  finished-but-unmarked candidate with zero function and data gaps.

## Post-admission gates

- `ninja progress`: exit 0. Halo progress became 3,349/7,574 accepted
  functions, 406,188/1,770,166 code bytes, 1,792,466/3,923,451 data bytes,
  and 268/468 objects.
- Post-admission audit: no finished-but-unmarked candidate remains and no
  `cluster_partitions` contradiction was reported. The unrelated pre-existing
  `shell_xbox` contradiction remains outside this lane.
- Parked-evidence validation: 6 active, 0 stale, 0 invalid.
- Fail-closed repeat-rebuild regression fingerprint: target evidence,
  accepted-function fingerprints, every non-code section fingerprint, symbol
  ownership, and comparison states were unchanged. The complete-object SHA
  changed only because MSVC stamps the COFF header; the regression gate does
  not treat that non-section timestamp as acceptance evidence.
- Comparator, regression, semantic, admission, and parked-evidence tests:
  98/98 pass. Full tools test discovery: 179/179 pass.
- Diff-policy audit: exactly five object-scoped tracked files changed; the only
  Markdown change is this ledger; no frozen object or Claude Markdown path is
  changed or deleted; `git diff --check` passes.

Disposition: strict-complete, full-build clean, regression-stable, and
admitted as `Matching`.
