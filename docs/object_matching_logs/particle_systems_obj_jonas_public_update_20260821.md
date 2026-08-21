# `particle_systems.obj` Jonas public-update leaf (2026-08-21)

## Result and fixed boundary

This Jonas-owned ledger records the requested fixed two-body wave from exact
canonical commit `09186240fea218e8f19d9dc952125e94179a0775` on isolated
branch `jonas/particle-systems-public-update-20260821`. Both bodies were
present together in the first and only natural code-producing candidate
compile. `_particle_systems_update` was independently strict immediately and
is retained. `_particle_system_new_unattached` was nonexact and was removed
wholesale, together with every support-only declaration and shared-header
refinement, without a retry or code-generation tune.

`particle_systems.obj` advances from 9/25 to 10/25 exact functions and from
406/7,453 to 519/7,453 meaningful code bytes. It remains `NonMatching`. The
strict net gain is 113 meaningful bytes, 128 padded bytes, and ten exact
relocations; no data credit is claimed.

| Fixed candidate | January RVA | Meaningful / padded | Relocations | Normalized SHA-256 | First-candidate decision |
| --- | ---: | ---: | ---: | --- | --- |
| `_particle_systems_update` | `0x0008F760` | `113 / 128` | 10 | `0f7bfc79eebe2a66cf652a210cc5e93b4a5346066692e1411892b1fe88ec0518` | **strict exact, retained** |
| `_particle_system_new_unattached` | `0x0008F880` | `205 / 208` | 8 | target `39b66f0d516aefd4eee70e8ddb5c9ce80138d8a1f11f467f688738ef348b8e38`; candidate `597216d672b5e1e8e51360d3c160da480d3650aa36902ced0d719818d80a0c08` | rejected and removed |

## Immutable target and source provenance

- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January csplit `source/effects/particle_systems.obj` SHA-256:
  `a6c6b08ada7fe558c34455c623d8cf5249798f7e023b3cb3ddd4167dc3ac20ba`.
- Baseline `source/effects/particle_systems.c` Git blob:
  `142eb5036e619f91f4f79d0511290521f3fa11ac`, 4,773 payload bytes,
  with Git-blob payload SHA-256
  `496738c27765262e0a0f110aa36e24d0ea67caa1e9fc1ec56db6d4150a8e188d`.
- Retained translation-unit Git blob:
  `4c30ed4b6dfbd22779bd774515c31a243a38c213`, 5,241 payload bytes,
  with SHA-256 of the exact `git cat-file blob` payload
  `b54b4c42c29aee64e1bc1d051fa7cfbd4b3c21bf1ee1718ab2499eaa8ae3c6f6`.
  These are Git-blob bytes, not CRLF working-tree bytes.
- The final shared `source/effects/particle_systems.h` is unchanged at blob
  `b3eb17fc90c5d80eb18e4f48adf4180fef7bbcf8`, 2,559 payload bytes,
  payload SHA-256
  `6eac1128999e6642b1e002bc2b6ef420c4448c0704a1ce58719343a6b3674fd4`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `xbox/bin/vc7/CL.Exe` SHA-256 is
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`;
  the natural flags remain `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` with the
  repository's unchanged XDK 3911 include graph.

The clean local HCEA semantic reference is exact commit
`c168af2e747d3095d9a29418ae401f3a39544863`. Both cited files are directly
tree-reachable there, and their hashes below are over exact Git-blob payloads.

| HCEA path | Git blob | Bytes | Payload SHA-256 |
| --- | --- | ---: | --- |
| `src/particle_systems_update.c` | `6142073577e29629b1b935e046b27f23dfdd931d` | 612 | `9ad942fae837ab46eb9e20d703f280a800a582a230320b35cb8e79fac3935b28` |
| `src/particle_system_new_unattached.c` | `f3b251638c598a00d418f7a7da2492f95a71fa71` | 2,321 | `51df148ae13127bbd92b57ab1dd2bd3006c358547f00c9d13d313d529ba6a338` |

HCEA supplies readable public intent and the natural iterator/allocation
topology, but it is not the PC layout or byte authority. In particular, its
update donor omits January's assertion, and its unattached donor contains a
CE/A render-enable gate that is absent from January. The gate was deliberately
not imported. January COFF remains authoritative for signatures, field
offsets, constants, assertion provenance, call identities, padding,
relocation order, and acceptance. No Stian source was needed for this wave.

Repository-wide history search finds no prior committed definition of either
fixed body and no earlier `particle_systems.obj` matching ledger. The January
target defines each public symbol exactly once. No competing particle-systems
source branch was active when this wave was dispatched.

## Typed ABI, layout, and consumer boundary

The retained ordinary-C interfaces are:

```c
void particle_systems_update(real delta_time);
void code_0008e7f0(real delta_time, long system_index);
```

On the repository's 32-bit PC/Xbox C ABI, `real` is a 32-bit float and `long`
is a 32-bit signed integer. The source identifiers compile naturally as the
external cdecl symbols `_particle_systems_update` and `_code_0008e7f0`; the
latter remains an undefined value-zero function dependency in this object.
The inherited `data_next_index(struct data_array *, long)` declaration also
returns signed `long`, and `NONE` is `-1`. The final body therefore needs no
cast, narrowing, raw address, or private/register calling convention.

Before editing, the complete direct shared-header consumer census was
performed. Exactly two reconstructed translation units include
`effects/particle_systems.h`: owner `source/effects/particle_systems.c` and
sole non-owner `source/objects/objects.c`. Repository-wide search finds no
reconstructed-source caller of `particle_systems_update`; its definition
self-types the function. Consequently, the final result does not add an
unused public header prototype.

The initial fixed candidate needed readable typed fields for the unattached
body, so its header refinement named January's `real_argb_color` at `+0x38`
and lighting color at `+0x48`, while preserving the existing `function_value`
at `+0x14`, `location` at `+0x18`, position at `+0x20`, velocity at `+0x2C`,
and total datum size `0x158`. A controlled compile of every non-owner consumer
compared `objects.obj` against the exact-HEAD baseline: all 314 runtime/code
and other non-debug sections were identical; only compiler type information
in `.debug$S` differed among 315 sections. Once the unattached body missed,
those two fields, their offset checks, and both support-only public prototypes
were removed wholesale. The final header blob is therefore exactly the
baseline identity recorded above, and the retained wave is source-only.

The rejected candidate used the fixed public signature:

```c
long particle_system_new_unattached(
    long definition_index,
    real_point3d const *position,
    real_vector3d const *velocity,
    real_argb_color const *color,
    real scale);
```

That signature naturally decorates as cdecl
`_particle_system_new_unattached`. Its typed candidate followed January's
allocation, field-copy, lighting, initialization, failure-delete, and return
flow while omitting HCEA's donor-only enable gate. Because the body was not
strict, none of its declarations or layout refinements survives.

## Retained semantic and relocation proof

The retained body first executes January's exact source assertion at line
`0x232`:

```c
particle_systems && particle_systems->valid
```

It then walks the live data-array indices naturally from `NONE`, calls
`code_0008e7f0(delta_time, system_index)` once for each live entry, and uses
`data_next_index` for the successor. The terminal return is explicit. HCEA's
loop corroborates this behavior; January supplies the missing assertion and
the PC helper identity.

The hardened comparator proves all ten relocations identical in offset, type,
destination, order, and addend:

| Offset | COFF type | Destination |
| ---: | --- | --- |
| `+0x04` | `DIR32` | `_particle_systems` |
| `+0x1B` | `DIR32` | exact source-path literal |
| `+0x20` | `DIR32` | exact assertion-predicate literal |
| `+0x25` | `REL32` | `_display_assert` |
| `+0x2C` | `REL32` | `_system_exit` |
| `+0x31` | `DIR32` | `_particle_systems` |
| `+0x3D` | `REL32` | `_data_next_index` |
| `+0x53` | `REL32` | `_code_0008e7f0` |
| `+0x58` | `DIR32` | `_particle_systems` |
| `+0x5F` | `REL32` | `_data_next_index` |

The compiler-owned 42-byte path COMDAT has normalized SHA-256
`a2a1badea9fbbaa6c5d56a72fc7053f8babc722648afb33f56d617d79f861bbf`;
the 44-byte predicate COMDAT has normalized SHA-256
`77015809af2445440520634d0fb94b935d4819f10618b6e95bbe2bd41c530944`.
Both chunks independently equal January, including ownership identity.

All nine inherited accepted bodies remain independently strict after final
retention: `_particle_systems_initialize`,
`_particle_systems_initialize_for_new_map`, `_particle_system_orphan`,
`_particle_systems_dispose`,
`_particle_systems_disconnect_from_structure_bsp`, `_code_0008e0d0`,
`_code_0008e140`, `_code_0008e310`, and
`_particle_systems_dispose_from_old_map`. Direct hardened comparison proves
the complete ten-body inventory has exact padded bytes, normalized hashes,
and relocation semantics.

## One-shot rejection and storage boundary

Both complete natural bodies and all required typed support were fixed before
the sole code-producing candidate compile. `_particle_systems_update` was
strict in that object. `_particle_system_new_unattached` emitted the target's
same 208 padded bytes and eight relocations, but not the target normalized
bytes or relocation schedule. Its final failure-delete pair was seven bytes
late: January's `_particle_systems`/`_datum_delete` relocations are at
`+0xB0/+0xB7`, while the candidate emitted them at `+0xB7/+0xBE`.

The miss was removed immediately. There was no expression-order, declaration,
control-flow, qualifier, field spelling, gate, or compiler-switch retry. Reopen
it only with independently authenticated January-PC source evidence that
explains the seven-byte schedule naturally.

The target object owns 572 ordinary `.rdata` bytes and no `.bss` or writable
`.data`. The final candidate owns only four runtime string COMDATs totaling
129 bytes, including the two new target-exact assertion chunks; the rest is
compiler directive/debug material. `_particle_systems` and
`_system_particles` both remain undefined value-zero external objects. This
wave defines no storage owner, COMMON symbol, `.bss`, or writable `.data` and
claims zero non-code credit while the object remains incomplete.

The retained implementation is readable typed defined C. It contains no
assembly, raw address or byte-offset dereference, pointer/integer
reconstruction, object-representation cast, pointer or union pun,
inactive-union access, undefined signed overflow, private/register ABI,
volatile scheduling device, force-inline annotation, optimizer pragma or
barrier, synthetic anchor, or byte-forcing expression.

## Scope and pre-commit validation

Only `source/effects/particle_systems.c` and this new Jonas-owned ledger are
tracked changes. No shared header, configuration, semantic exception, parked
record, completion label, storage owner, pre-existing Markdown, or tracked
path deletion is changed. The five protected sources remain untouched:
`source/units/vehicles.c`, `source/ai/ai_debug.c`,
`source/ai/ai_script.c`, `source/ai/actions.c`, and
`source/units/units.c`.

- Complete `halobetacache_build` plus `libcmt_build`: pass.
- Direct hardened COFF comparison: all ten emitted functions are strict;
  `_particle_system_new_unattached` is absent from the retained object.
- Semantic audit: 470 units, 4,129 functions evaluated, 3,989 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 4,050 accepted exact, 36
  ordinary-only, 35 structurally accepted, one rejected, and zero unit
  errors.
- Campaign progress: 375/833 matched objects, 4,037/11,060 exact functions,
  480,789/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes.
- Halo progress: 273/468 matched objects, 3,870/7,574 exact functions,
  467,875/1,770,166 meaningful code bytes, and
  1,829,896/3,923,451 matched data bytes. Libcmt remains 102/212 objects,
  167/476 functions, 12,914/55,015 code bytes, and 5,192/8,637 data bytes.
- `particle_systems.obj`: 10/25 exact functions and 519/7,453 meaningful code
  bytes; it remains nonmatching and claims no data.
- Object admission: zero candidates and zero revocations; only the inherited
  unrelated `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass; `git diff --check`, source scope,
  protected-five, banned-construct, and deleted-path checks are clean.
- The exact-HEAD pre-wave TU gate reports precisely
  `_particle_systems_update` as `NEWLY_EXACT`, all nine inherited bodies as
  `still_exact`, and `changed_nonexact: []`. It fail-closes, as designed, on
  the two new target-exact assertion `.rdata` COMDATs, compiler `.debug$S`,
  and the expected new function/dependency symbols. No adjudication or credit
  metadata was added; direct January comparison supplies the acceptance
  proof.

The final handoff additionally requires a clean committed-state one-unit
snapshot, verified same-path deletion and normal Ninja rebuild of
`particle_systems.obj`, an immediate regression check, direct ten-body
comparator replay, rejected-symbol absence check, and committed Git-blob
payload recheck. No push, amend, or history rewrite is performed.
