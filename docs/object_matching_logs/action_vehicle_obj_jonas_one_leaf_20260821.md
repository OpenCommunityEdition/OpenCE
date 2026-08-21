# `action_vehicle.obj` Jonas one-leaf wave

## Result

This bounded source-only wave starts from authoritative integration commit
`d3453bd46972f3c24148b2de14f495286fa3637c`. The three permitted HCEA-backed
action-vehicle bodies were emitted together in exactly one natural candidate
compile. `_action_vehicle_flush_structure_indices` was independently strict
and is retained. `_action_vehicle_begin` and `_action_vehicle_control` missed
and were removed immediately, without a spelling, declaration, control-flow,
or compiler-control retry. The pre-existing exact `_action_vehicle_update`
body and bytes are unchanged.

`action_vehicle.obj` advances from 1/17 to 2/17 exact functions and remains
`NonMatching`.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_action_vehicle_flush_structure_indices` | 34 | 48 | 2 | `0463c71d743e4bc0945ec5b28c91d1650764463a866ff5e552074a88f4ad1130` |
| **Wave gain** | **34** | **48** | **2** | |

The hardened COFF comparator proves identical padded bytes and exact
relocation addresses, types, destinations, and addends. The object now reports
35/4,909 meaningful code bytes and two exact functions. The target's remaining
fifteen functions and all 116 non-code bytes stay outside this wave's credit.

## Provenance and signatures

- Baseline `source/ai/action_vehicle.c` blob:
  `77f976dc9617cb72d35c768e2fa2b839be7d7ad5`.
- Retained source blob/SHA-256:
  `21e056ab7bf30cb7f746e8a610ca15146a4785ae` /
  `b7e4c267346e0c4e6426e4a3b5a44507a2fdf3ce5fbabc4191c48bd22dc1ba0a`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split-object SHA-256:
  `49f00a0115cecf578e51102fc33f665f876a4fafcb78ee3b53520545406ef769`.
- Clean HCEA semantic donor commit:
  `570c83fd9c365dad6f2a3e7041705d5b84c7847c`. Its exact-name
  `src/blam/ai/action_vehicle_begin.c`,
  `src/blam/ai/action_vehicle_flush_structure_indices.c`, and
  `src/blam/ai/action_vehicle_control.c` files supply the three natural typed
  behaviors. The retained donor file's blob is
  `99133bb4ae11f72755ea63cc95f4fede8c9d4182`.
- January PC disassembly independently fixes the 32-bit actor datum handle,
  `actor_get`/`datum_get` access path, destination-surface field store, exact
  call identities, code extents, padding, and relocation ownership. January
  COFF remains authoritative for acceptance.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86, SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
  with the repository's unchanged XDK 3911 command and flags.

The retained signature is:

```c
void action_vehicle_flush_structure_indices(
	long actor_index);
```

Existing `actors.h` and `actions.h` own the typed actor/action layout and
`actor_get` contract. Existing `cseries.h` owns `NONE`. No shared-header or
storage repair is needed.

## Authenticated behavior and one-shot rejection record

The retained body resolves the full 32-bit actor handle through `actor_get`
and invalidates
`actor->state.action_data.vehicle.destination_surface_index` with `NONE`.
Its target relocations are `DIR32 _actor_data` at `+8` and
`REL32 _datum_get` at `+15`.

All three candidates were compiled together once. The following two misses
were removed without retry:

| Rejected function | Target meaningful / padded / relocs | Target hash | First natural candidate |
| --- | ---: | --- | --- |
| `_action_vehicle_begin` | `76 / 80 / 3` | `5e5ac406a3045fd7df7a8414f08608cd1ece5bd940ea167941fb293bd192a8f0` | `80 / 3`, SHA `fef3ce0d9fc229b139957cfc30fdcc6d1a4a212be1536f2ebc864411f2046c65`; the relocation addresses and identities are equal, but normalized instruction bytes differ. |
| `_action_vehicle_control` | `175 / 176 / 3` | `81612c85cf900b15c2de8ea6d52646f5727a96fbea5e2f13018e6f9613fcd293` | `176 / 3`, SHA `3f96cbf0cdc3a58d1013ac2d44df261acc0d3b68c37ebab639956ddfa1a046ad`; the relocation addresses and identities are equal, but normalized instruction bytes differ. |

For `_action_vehicle_begin`, the common relocations are `DIR32 _actor_data`
at `+8`, `REL32 _datum_get` at `+16`, and `REL32 _game_time_get` at `+35`.
For `_action_vehicle_control`, they are `DIR32 _actor_data` at `+4`,
`REL32 _datum_get` at `+17`, and `REL32 _actor_path_has_path` at `+89`.
Both rejected COMDATs and their candidate-only includes/constants are absent
from the final object. Reopen either body only with independently preserved
January source or a newly authenticated ordinary-C contract; do not tune from
the rejected objects.

## Policy, scope, and validation

Only `source/ai/action_vehicle.c` and this new Jonas-owned ledger are changed.
No shared header, storage definition, data section, configuration, semantic
exception, parked record, completion label, frozen-five source, pre-existing
Markdown, or Claude-owned file is edited.

The retained source is readable typed C. It contains no assembly, raw address
or byte-offset dereference, pointer/integer reconstruction, pointer or union
pun, inactive-union access, undefined overflow, volatile scheduling device,
force-inline annotation, optimizer pragma/barrier, synthetic anchor, or
object-byte forcing. The final COFF's only action-vehicle code symbols are the
pre-existing `_action_vehicle_update` and retained
`_action_vehicle_flush_structure_indices`; it adds no owned storage.

- Complete 569-edge `halobetacache_build` and `libcmt_build`: pass.
- Direct hardened comparison: both `_action_vehicle_update` and
  `_action_vehicle_flush_structure_indices` pass; both rejected symbols are
  absent from the final COFF symbol table.
- Semantic audit: 470 units, 4,053 functions evaluated, 3,913 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 3,974 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 complete objects, 3,961/11,060 exact functions,
  475,672/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes. This wave adds one function and 34
  meaningful bytes with no data increase.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass; `git diff --check` is clean.

A clean committed-state snapshot followed by deletion and forced rebuilding
of `action_vehicle.obj` is the final same-path reproducibility proof. The
post-rebuild hardened comparison remains strict for both accepted functions.
No push is performed.
