# `actor_moving.obj` Jonas six-leaf wave

## Result

This bounded source-only wave starts from authoritative integration commit
`3f2ba63dfb1258f7e36baaec69fe495bcefdc20f`. Exactly six authenticated,
ordinary-ABI actor-movement leaves were emitted together in one natural
object-producing candidate compile. Three were independently strict and are
retained. The three misses were removed immediately without changing a
declaration, expression, branch, local, or compiler control and without a
second candidate attempt.

`actor_moving.obj` advances from 0/36 to 3/36 exact functions and remains
`NonMatching`. The wave gains 129 meaningful and 144 padded code bytes, seven
relocations, and no data credit.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_actor_move_keep_moving_past_destination` | 38 | 48 | 2 | `00196458cf1a472a0be6f10a753144237ea5a61c12e951384f770990b9d68f74` |
| `_actor_move_animation_busy` | 61 | 64 | 3 | `9fcb00ab9e3bd2cd37c1df910a592601cb5464621f1eb4d320521479dd420f7f` |
| `_actor_path_has_path` | 30 | 32 | 2 | `53b10b14ff7ec9b3e1c07f2f997b14c8271ad5054136ae583232d5c749d09359` |
| **Wave gain** | **129** | **144** | **7** | |

The hardened COFF comparator reports `all_equal: true` for all three retained
bodies. Their padded bytes and relocation addresses, types, destinations, and
addends are identical to January. The target's other 33 functions, 19,960
meaningful code bytes, and all 1,960 non-code bytes remain outside this wave's
credit.

## Provenance and signatures

- Baseline `source/ai/actor_moving.c` blob:
  `59e4235be6d6ec7e16fc8850601e68019a0d047d`.
- Retained source blob/committed-blob SHA-256:
  `6fe52f05e14e8eb304d39b0c8b0d298fa854987b` /
  `5cf91ea2bd0dc9d06ebb62cee1333b147875da93f938d28f125d8cdceb78b77d`.
  The SHA is over `git cat-file blob` payload bytes, not working-tree EOLs.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `actor_moving.obj` SHA-256:
  `d693781be5186ab1f618cc7765826856e679f27a40fc94802686c338c60520fa`.
- Clean HCEA semantic donor commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. The six body blobs, in the
  screened order below, are
  `72ff05007849a7233d386fa2d162a6a58e79e89f`,
  `4613867b2329504d02428bc0f293b182e3dd15c6`,
  `e414fbd17f5f7abf00bfea1b790d834a1759f502`,
  `77fd74779fb43186f85bde8755a2789c647056d9`,
  `fb2b6ba0cbc70c50a638f8a083e082cb127aad04`, and
  `9e217dbdf48cc159be3313d3323e162daca89821`. Its typed `actor_datum`
  layout blob is `77a69f79b6a42cf0dac4462e434e634ce71f7bc3`.
- Clean Stian January-PC corroboration commit:
  `e638cf5fa17ed5308ac5c101ef65e9c69ced65f7`; its
  `src/halo/ai/actor_moving.c` blob is
  `00a712bcd6723ddc7733925979202fdecf5e5559`.
- Existing canonical typed owners are `actors.h` blob
  `e061b529b05c782b1220cff8a05dd89e6e2f3ed2` and `units.h` blob
  `54492a4ee4cb8b37739f9ebbfa89c72fbd0e08fc`. No header refinement was
  required.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86, SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
  using the repository's unchanged XDK 3911 command and flags.

The January PDB, disassembly, callers, and canonical typed declarations fix
the screened signatures as:

```c
void actor_move_keep_moving_past_destination(long actor_index);
boolean actor_move_animation_busy(long actor_index);
void actor_path_clear(long actor_index);
boolean actor_path_has_path(long actor_index);
boolean actor_path_at_destination(long actor_index);
short actor_path_get_destination_firing_position_index(long actor_index);
```

The corresponding January RVAs are `0x18940`, `0x18970`, `0x189B0`,
`0x189E0`, `0x18A00`, and `0x18A40`. HCEA supplies readable natural bodies;
Stian independently corroborates the PC topology; January remains
authoritative for the 32-bit datum-handle ABI, symbol identity, extent,
padding, bytes, and relocation ownership.

## Authenticated retained behavior and ownership

The existing typed actor layout has a 0x724-byte datum. The retained setter
resolves the actor through `actor_get`, sets the named move-order and copied
path-order `keep_moving` flags, and returns. The busy predicate returns true
when an animation impulse is pending; otherwise it returns false only when
there is no unit or `unit_is_busy` returns false. The path predicate returns
the named `control.path.path.valid` byte.

January independently proves the relevant actor offsets: unit handle `+0x18`,
move-order keep-moving `+0x402`, animation impulse `+0x418`, copied path-order
keep-moving `+0x46E`, and path-valid `+0x4A8`. The canonical structures name
all of those fields, and `actors.h` owns typed `actor_get`/`_actor_data` while
`units.h` owns `unit_is_busy`.

The two simple retained bodies each have `DIR32 _actor_data` at `+8` and
`REL32 _datum_get` at `+15`. The busy predicate additionally has
`REL32 _unit_is_busy` at `+42`. The final candidate object defines only
`.drectve`, `.debug$S`, and the three code COMDATs. It owns no `.rdata`,
`.data`, `.bss`, COMMON symbol, or other global storage.

## One-shot rejection record

The first compiler invocation stopped during parsing because the math headers
used `byte` before its owning `cseries.h` include. It emitted no object.
Placing the existing `cseries.h` owner first was the sole parse-only
correction; the next invocation emitted the only six-body candidate object.

| Rejected function | Target meaningful / padded / relocs | Target hash | First natural candidate |
| --- | ---: | --- | --- |
| `_actor_path_clear` | `45 / 48 / 2` | `3b4dbcbbcc65d9a3dcfd54b668d009a64606afc5757324cbf5b14d88dd46e3a2` | `48` padded / 2 relocs, SHA `9eed0145a72c55c38e7b0ab83dddbf20ee5f8cc1eb2216c12c579110f28c783c`; natural named stores do not reproduce January's instruction schedule. |
| `_actor_path_at_destination` | `53 / 64 / 2` | `4fa43c4de2749ed7dca8168246762d7fb1d47a2d7cf243a9bcc074a0261834307` | `64` padded / 2 relocs, SHA `64a7a16ab1964af880f6d206c76ac7a1842a209b58599c5fa74db0f4c5ef2522`; the natural Boolean control flow is semantically correct but byte-different. |
| `_actor_path_get_destination_firing_position_index` | `49 / 64 / 2` | `1df131df840d01494662d3cfd77fa65e9f650167d70dcf92f3770355046cc63c` | `48` padded / 2 relocs, SHA `a6205d27abdbb419734451518c55599c070b1adbc6872680bb0cf15b85680c47`; January's sign/width code shape is not reproduced by the authenticated typed expression. |

Each target has `DIR32 _actor_data` at `+8` and `REL32 _datum_get` at `+15`.
All three rejected definitions and symbols are absent from the retained COFF.
They may be reopened only with newly preserved January source or independent
ordinary-C evidence; this wave does not tune from their rejected bytes.

After subset selection, the retained void leaf received only the required
house-format terminal `return;`. A normal rebuild proved that this style-only
edit leaves its strict COFF unchanged; no body-shape experiment or retry was
performed.

## Policy, scope, and validation

Only `source/ai/actor_moving.c` and this new Jonas-owned ledger are changed.
No header, storage definition, data section, configuration, semantic
exception, parked record, completion label, protected/frozen source,
pre-existing Markdown, or Claude-owned file is edited. Every other
`actor_moving.c` body remains excluded.

The retained implementation is readable typed C. It contains no assembly,
raw address or byte-offset access, pointer/integer reconstruction, cast or
union pun, inactive-union access, undefined overflow, `volatile` scheduling
device, force-inline annotation, optimizer pragma/barrier, synthetic anchor,
or object-byte/code-generation forcing.

- Complete 569-edge `halobetacache_build` and `libcmt_build`: pass; the final
  house-format rebuild and aggregate no-op confirmation also pass.
- Direct hardened comparison: all three retained functions pass; all three
  rejected symbols are absent from the final COFF symbol table.
- Semantic audit: 470 units, 4,075 functions evaluated, 3,935 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 3,996 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 complete objects, 3,983/11,060 exact functions,
  476,824/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes. This wave adds three functions and
  129 meaningful bytes with no data increase.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass; `git diff --check` is clean.

A clean committed-state regression snapshot/check followed by deletion and
forced same-path rebuilding of `actor_moving.obj` is the final reproducibility
proof. The post-rebuild hardened comparison must remain strict for all three
retained functions, with the rejected symbols absent. No push is performed.
