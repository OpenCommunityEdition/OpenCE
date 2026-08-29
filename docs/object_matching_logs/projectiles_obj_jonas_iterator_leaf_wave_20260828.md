# `projectiles.obj` Jonas iterator-leaf wave (2026-08-28)

## Result

This bounded natural-C wave starts from pushed integration commit
`73219efe5605bcfe2bd60a36950cefaa996e53ae`. The authenticated public
iterator pair was emitted together in one frozen candidate compile. Both
functions were independently strict on the first gate, so both are retained
without a spelling, declaration, control-flow, scheduling, or compiler-control
retry. There are no first-shot residuals and no rejected support declarations.

`source/items/projectiles.obj` advances from 6/30 to 8/30 exact functions and
remains `NonMatching`. The wave gains 127 meaningful and 128 padded code bytes,
seven exact relocations, and no data credit.

| Retained owner | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_projectiles_delete_all` | 64 | 64 | 4 | `cb46ceedeb0b3ea4210add097045b63b47e850789299864cb6fcc344bb51a374` |
| `_dangerous_projectiles_near_player` | 63 | 64 | 3 | `c0d5c6908c8de27ab04af3f163008ca4839d0e9255974ee001beb61708091540` |
| **Wave gain** | **127** | **128** | **7** | |

The hardened COFF comparator proves equal padded bytes and exact relocation
address, type, destination, and addend. The object now has 154/11,950 matched
meaningful code bytes. Its 2,092 target data bytes remain outside this wave's
credit.

## Selection and donor evidence

The ordinary closeout ranking was run against the newest pre-wave integration
reports with:

```text
python tools/rank_object_closeouts.py --report <integration>/build/report.json --semantic-report <integration>/build/semantic_report.json --config config/config.json --parked config/parked.json --ledger-dir docs/object_matching_logs --limit 30
```

Its smaller `NEAR_CLOSEOUT` entries are already covered by explicit fixed-point
or no-retry ledgers. The two data-only leaders are not legal typed-body work.
After excluding the objects and rasterizer lanes named in the assignment, the
pinned HCEA correspondence exposed a compact projectile cluster. The donor
rank was independently replayed with `tools.crossbuild_bridge rank` using
`hcea_january_correspondence.json` SHA-256
`c6ea66d530438fba92d27790001c631d973d41e7fb6bacb21a4525763ec6cfbc`.
It maps the public iterator owners alongside the smaller private-datum owners;
only the public typed pair satisfies this lane's ownership policy.

The clean HCEA oracle is commit
`c168af2e747d3095d9a29418ae401f3a39544863`. Its donor files are:

- `src/projectiles_delete_all.c`, Git blob
  `1ea04179aea0b7705f57468a5e3543b65a686d6c`, file SHA-256
  `635190f62bd3c02e56a077590aec4c04188b6a150f2aece1c2d4b07e5782fb53`;
- `src/dangerous_projectiles_near_player.c`, Git blob
  `e6b42032ac515d3fa8efcc407f0307da1810230a`, file SHA-256
  `f292566725259497a9b5d9ccdf573f6638de5b09ddbf12d449c5d648560352b7`.

The Stian complete-history cache is pinned at
`fc11d861865735877d5b8b4c408b1a0c06dde3cf`; its complete projectile source
has SHA-256
`6d818a55733956487711e2b225257bd49124379c4381b5017f42a320b2bbd73d`.
The Pastudan full-history cache is pinned at
`918af885935ec470a31256ecce9a977b12b01f80`; its complete projectile source
has SHA-256
`80aea1250314e09e33d36b864b965b9c744856f7f2846ea06668e392e4ff1b23`.
The delete loop converges across all three donors. The HCEA Xbox predicate
omits the PC build's tag-lookup side effect, while Stian and Pastudan preserve
it; January's own third relocation and disassembly independently require that
side effect.

January call ownership is public and unambiguous. Only
`source/cutscene/cinematics.obj` imports `_projectiles_delete_all`.
`source/game/game.obj` and `source/game/players.obj` import
`_dangerous_projectiles_near_player`. The delete caller ignores the return,
attesting the existing `void` declaration. The predicate returns zero or one
in `AL`, agreeing with the repository's one-byte `boolean` declaration in
`players.c`.

The retained implementation uses the canonical `struct object_iterator`,
`_object_mask_projectile`, `object_iterator_next`, `iterator.index`,
`object_delete`, `struct object_datum::definition_index`, and
`projectile_definition_get`. No private projectile runtime layout is added.

## Complete January owner map and disposition

The PDB2.00 order, January RVAs, and padded COMDAT spans give the following
complete 30-owner map. `Pre-existing exact` means strict before this wave;
`retained` means strict on the one permitted first shot.

| RVA | Owner | Padded | Disposition |
| ---: | --- | ---: | --- |
| `0xE7460` | `_projectiles_initialize` | 16 | Pre-existing exact |
| `0xE7470` | `_projectiles_initialize_for_new_map` | 16 | Pre-existing exact |
| `0xE7480` | `_projectiles_dispose_from_old_map` | 16 | Pre-existing exact |
| `0xE7490` | `_projectiles_dispose` | 16 | Pre-existing exact |
| `0xE74A0` | `_projectile_kill_tracer` | 48 | Excluded: private runtime-datum bit owner |
| `0xE74D0` | `_projectiles_delete_all` | 64 | **First-shot retained** |
| `0xE7510` | `_projectile_delete` | 16 | Pre-existing exact |
| `0xE7520` | `_projectile_set_target_object_index` | 32 | Excluded: private runtime-datum field owner |
| `0xE7540` | `_projectile_make_tracer` | 48 | Excluded: private runtime-datum bit owner |
| `0xE7570` | `_projectile_get_ballistic_acceleration` | 32 | Pre-existing exact |
| `0xE7590` | `_projectile_estimate_time_to_target` | 48 | Prior first-shot residual; documented no-retry |
| `0xE75C0` | `_dangerous_projectiles_near_player` | 64 | **First-shot retained** |
| `0xE7600` | `_projectile_handle_deleted_object` | 48 | Excluded: private runtime-datum field owner |
| `0xE7630` | `_code_000e7630` | 32 | Excluded: anonymous/private contract |
| `0xE7650` | `_code_000e7650` | 96 | Excluded: anonymous/private contract |
| `0xE76B0` | `_projectile_export_function_values` | 224 | Excluded: stateful/private layout |
| `0xE7790` | `_code_000e7790` | 96 | Excluded: anonymous/private contract |
| `0xE77F0` | `_projectile_handle_parent_destroyed` | 112 | Excluded: stateful runtime-datum ownership |
| `0xE7860` | `_random_vector_in_cone3d` | 48 | Prior ledger: inline/out-of-line ownership conflict |
| `0xE7890` | `_projectile_aim_ballistic` | 880 | Excluded: large reconstruction |
| `0xE7C00` | `_projectile_aim_linear` | 192 | Excluded: non-leaf reconstruction |
| `0xE7CC0` | `_projectile_aim` | 192 | Excluded: non-leaf reconstruction |
| `0xE7D80` | `_code_000e7d80` | 176 | Excluded: anonymous/private contract |
| `0xE7E30` | `_code_000e7e30` | 224 | Excluded: anonymous/private contract |
| `0xE7F10` | `_code_000e7f10` | 512 | Excluded: anonymous/stateful body |
| `0xE8110` | `_code_000e8110` | 1,040 | Excluded: large anonymous reconstruction |
| `0xE8520` | `_projectile_new` | 432 | Excluded: stateful runtime-datum ownership |
| `0xE86D0` | `_projectile_accelerate` | 496 | Excluded: stateful runtime-datum ownership |
| `0xE88C0` | `_code_000e88c0` | 2,928 | Excluded: large anonymous state machine |
| `0xE9430` | `_projectile_update` | 4,064 | Excluded: large state machine |

The four sub-64 private-datum helpers are donor-authenticated semantically but
cannot be expressed without importing an unowned runtime `projectile_datum`
layout or using raw byte offsets. They were excluded before compilation and
must not be treated as failed spellings. Reopen them only after a canonical
typed runtime layout has an independently proven owner. The earlier ballistic
ledger's explicit no-retry decisions for
`_projectile_estimate_time_to_target` and `_random_vector_in_cone3d` remain in
force.

## Fingerprints and strict replay

- Baseline source Git blob:
  `a30cd38809f89617b470c3ed73acfe0744905902`.
- Final source Git blob / normalized CRLF working-tree SHA-256:
  `4fa7177bd4a23fab7afcd59c0cdaecc9fed4ff47` /
  `c6afa8e1b5a20d96832d5c95bb44e6b30124e5a150c972c78ece90ab2be74214`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `projectiles.obj` SHA-256:
  `0f558b623efe404e1c803b8fab03338c4f2b5b12af8ce14c4634c3ad59d6c8da`.
- January PDB2.00 symbol payload SHA-256:
  `7a02e56d90c680ccc9e95325f04c3406d39c197b22e7d2023b1bfb69da3aa118`.
- First frozen candidate `projectiles.obj` raw SHA-256:
  `0f6b2233a46f117009f5d099ed86b20f900bb70fa27b8943a484f32f142df7f1`.
- Retained-only replay `projectiles.obj` raw SHA-256:
  `7a2a81021a8abfa57cf682663a1d4e41fc7202e0d43b46bc5005d3eb7ba61fcb`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1`,
  SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
  with the repository's unchanged XDK 3911 command and flags.

The exact relocation maps are:

- `_projectiles_delete_all`: `REL32 _object_iterator_new` at `+15`,
  `REL32 _object_iterator_next` at `+24`, `REL32 _object_delete` at `+40`,
  and `REL32 _object_iterator_next` at `+49`;
- `_dangerous_projectiles_near_player`: `REL32 _object_iterator_new` at
  `+15`, `REL32 _object_iterator_next` at `+24`, and `REL32 _tag_get` at
  `+44`.

After the first gate, the generated candidate object was deleted and rebuilt
from the retained-only source. The same-path hardened comparison remains exact
for both new functions and all six baseline functions. The direct exact-set
comparison reports no lost exact owner and exactly these two newly exact
owners.

## Policy, ownership, and validation

Only `source/items/projectiles.c` and this Jonas-owned ledger are changed. No
header, data owner, configuration, semantic exception, parked record,
completion label, Units/Vehicles source, Claude-active file, or other object is
edited. The candidate COFF defines eight code COMDATs plus debug/directive
sections and owns no runtime `.data`, `.bss`, or `.rdata` section.

The retained source is ordinary C89-compatible typed C. It contains no
assembly, `volatile`, `register`, pragma, intrinsic, optimizer barrier,
undefined behavior, raw address, byte-offset access, representation pun,
private-ABI body, artificial anchor, or byte/code-generation forcing.

- `tools/campaign/gate.py source/items/projectiles --all`: 8 exact, zero
  residual, 22 unwritten.
- Hardened pair comparison: both retained owners are strict; seven relocation
  records are exact.
- Retained-only forced rebuild: pass with identical strict fingerprints.
- `halobetacache_build` and `libcmt_build`: pass.
- Semantic audit: 470 units, 4,661 functions evaluated, 4,530 semantic exact,
  125 hidden exact / 72,363 hidden code bytes, 4,552 accepted exact, and zero
  unit errors.
- The umbrella default build compiles all 569 object edges and produces both
  reports, then its final unrelated `progress` verifier stops on the inherited
  excluded rasterizer alias owner
  `source/rasterizer/xbox/rasterizer_xbox_plasma_energy:_code_0015e2b0`.
  This wave does not touch that unit; the strict projectile gates above pass.

No commit or push is performed.
