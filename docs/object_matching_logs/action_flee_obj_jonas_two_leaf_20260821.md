# `action_flee.obj` Jonas two-leaf wave

## Result

This bounded source-only wave starts from authoritative integration commit
`a03585774f859697a788a73e4c3fa6bde7c6e3ea`. Six HCEA-backed action-flee
bodies were emitted together in one natural candidate compile. Two were
independently strict and are retained. The four nonexact bodies and their
support-only declaration were removed immediately, without a spelling,
declaration, control-flow, or compiler-control retry. `action_flee.obj`
advances from 0/13 to 2/13 exact functions and remains `NonMatching`.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_action_flee_end` | 41 | 48 | 3 | `649eb65fd4e8e516d5c8675b4dbf5e08dab7b6d07888e25af8327f32b5c6be98` |
| `_action_flee_blind_panic` | 30 | 32 | 0 | `7bcf33bcce1177b460ded4cc5dee5518afa9db14695e8fdd49652483675b6297` |
| **Wave gain** | **71** | **80** | **3** | |

The hardened COFF comparator proves identical padded bytes and exact
relocation addresses, types, destinations, and addends for both retained
functions. The object now reports 71/3,312 meaningful code bytes and two
exact functions. The target's remaining eleven functions and all 312
non-code bytes stay outside this wave's credit.

## Provenance and signatures

- Baseline `source/ai/action_flee.c` blob:
  `73990e6dee5af609201a2b17af56424f3276161a`.
- Retained source blob/SHA-256:
  `65b90ce0ba18f6e01c13ef33c8b6bd539db16079` /
  `67e862acfb2a040c4a652ecbe85c842d1a9869a3159eefc9bc3059bb351874b7`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split-object SHA-256:
  `5d6e667103c4b42824a5e1c69fb047860a60d901eda36153162cf19672eff59e`.
- Clean HCEA semantic donor commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. Its exact-name
  `action_flee_end.c`, `action_flee_flush_position_indices.c`,
  `action_flee_modify_color.c`, `action_flee_replace_prop.c`,
  `action_flee_blind_panic.c`, and `action_flee_begin.c` files supply the
  six natural typed behaviors.
- January PC disassembly independently fixes 32-bit actor datum handles,
  the `actor_get`/`datum_get` path, the named flee-state offsets, the signed
  panic interval `[9, 12]`, and both unit blind-running calls. January COFF
  remains authoritative for symbol identity, bytes, padding, and relocation
  ownership.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86, with the repository's unchanged XDK 3911 command and flags.

The retained signatures are:

```c
void action_flee_end(
    long actor_index);
boolean action_flee_blind_panic(
    short panic_type);
```

The existing `actors.h` owns the typed actor/action layout and `actor_get`
contract. The existing `units.h` declaration owns the stop-running call.
No shared-header repair is needed.

## Authenticated behavior and one-shot rejection record

`action_flee_end` resolves the complete 32-bit actor handle through
`actor_get`; when the actor owns a valid unit datum, it calls the typed
`unit_stop_running_blindly` API. `action_flee_blind_panic` returns true only
for the inclusive signed-short interval from grenade attachment through
burning to death, represented by the source-local enum values 9 and 12.
Both bodies have explicit terminal returns.

All six candidates were compiled together once. The following four misses
were removed without retry:

| Rejected function | Target meaningful / padded / relocs | Target hash | First natural candidate |
| --- | ---: | --- | --- |
| `_action_flee_flush_position_indices` | `39 / 48 / 2` | `8dad23011290d89f5753258c4f1a2c41b21837956d5556a5de2c9a344de11329` | `48 / 2`, SHA `8da380385d9a8cfbb0f1465c1bef5c314add784ef72b616032e8646cded81413`; same relocation identities, different normalized instruction schedule. |
| `_action_flee_modify_color` | `99 / 112 / 4` | `9eef0cd0460952e50234aa38cad6a232cef2ed909f574428956b38026790dfb2` | `80 / 4`, SHA `61e086d8f401f02517b7ad56f1938c82a0121a05b697fba3cc11b86747d8685d`; the optimizer joined the color-copy tails instead of January's duplicated branches. |
| `_action_flee_replace_prop` | `48 / 48 / 2` | `10b5ae0e39ea2d834df55c0984c57bad0465ad8ee69612fc6cc02a1f7762cbf0` | `48 / 2`, SHA `48b2a3f2fe62a1474a20508f3c90d3866f70e390310adee035f55fd0db12bbf4`; same relocation identities, different normalized instruction schedule. |
| `_action_flee_begin` | `92 / 96 / 3` | `009e97240ce60b10098e18f6f625c4b678cd424dd832d92d69e7a3eb485ea9a0` | `96 / 3`, SHA `1aa8574ec4ab3fee338e6ea51c0ad4be4b1e0a7b2098b18442f21131151c2fab`; the unit-start call relocation lands at `+79` instead of January's `+83`. |

All four rejected COMDATs and the begin-only unit-start declaration are
absent from the final object. Reopen them only with independently preserved
January source or a newly authenticated ordinary-C contract; do not tune
their spelling or scheduling from this rejected object.

## Policy, scope, and validation

Only `source/ai/action_flee.c` and this new Jonas-owned ledger are changed.
No shared header, storage definition, data section, configuration, frozen-five
source, semantic exception, parked record, pre-existing Markdown, or
Claude-owned file is edited.

The retained source is readable typed C. It contains no assembly, raw-address
or byte-offset dereference, pointer/integer reconstruction, pointer or union
pun, inactive-union access, undefined overflow, volatile scheduling device,
force-inline annotation, optimizer pragma/barrier, synthetic anchor, or
object-byte forcing. The final COFF defines only two code COMDATs plus debug
metadata; it defines no `.rdata`, `.data`, `.bss`, COMMON symbol, or global
storage.

- Complete `halobetacache_build` and `libcmt_build`: pass.
- Direct hardened comparison: both retained functions pass; all four
  rejected symbols are absent from the final COFF symbol table.
- Semantic audit: 470 units, 4,052 functions evaluated, 3,912 semantic
  exact, 101 hidden exact / 61,876 hidden code bytes, 3,973 accepted exact,
  and zero unit errors.
- Campaign progress: 375/833 complete objects, 3,960/11,060 exact functions,
  475,638/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes. This wave adds two functions and
  71 meaningful bytes with no data increase.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass; `git diff --check` is clean.

The fail-closed pre-wave check identifies exactly the two retained functions
as `NEWLY_EXACT` and reports `changed_nonexact: []`. Adding the first two
COMDATs necessarily changes the translation unit's debug section and symbol
set, which the generic structural check surfaces for review; direct hardened
comparison independently proves both accepted functions. No adjudication or
configuration exception is added.

A clean committed-state snapshot/check followed by deletion and forced
rebuilding of `action_flee.obj` is the final same-path reproducibility proof.
No push is performed.
