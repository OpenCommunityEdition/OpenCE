# Units typed damage-record wave (2026-08-24)

## Result

This bounded wave retains one strict January-PC function in
`source/units/units.c`:

| function | meaningful | padded | relocs | normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_unit_record_damage` | 585 | 592 | 14 | `5822b73bd7216a1f01894d55c61360f8a72b4858ff9effee7bf37ac7901adfbc` |

The retained function is byte- and relocation-exact. The wave adds 585
meaningful code bytes, 592 padded bytes, and 14 relocation identities. It
adds no writable storage and claims no data credit.

The fixed first-shot candidate set also contained four independently
removable policy rewrites. They missed strict comparison and were removed
wholesale without a spelling, scheduling, topology, or compiler retry:

| rejected body | target padded/relocs/hash | first-shot candidate padded/relocs/hash |
| --- | --- | --- |
| `_unit_export_function_values` | `384 / 18 / 4149e4cd513cba59c251998986db6acd8857e41f96d6182b9692023361b20c35` | `384 / 18 / 9e3c8db670f86c1b686d97c7f5303d7967ccb461927de65ffb22f51479bea812` |
| `_units_debug_get_next_unit` | `240 / 8 / 5098e32812b1e3e53fcb6201a48145c446f16b5ca7146f3bcee4c7bb29632879` | `240 / 8 / 6af79599a8efe5834ab136e58ba5f9ef136ad2dffa930becd4db46d219b24138` |
| `_units_debug_get_closest_unit` | `208 / 6 / de1f11019c5719a10ef09ec6987d302764cebcc2f9ffdb78fd2747254870034c` | `208 / 6 / 5be6e301b203b4d858ae764921bb722a13ffcc107ded5fc43f10dbbe2ec8d7f8` |
| `_unit_adjust_projectile_ray` | `208 / 3 / b9483057825a0c63739bf76296b17e8d0cf8f9a7435995762673ff54f0acc841` | `192 / 3 / 56974161e52c17013e454ad72749192ad987c56589ad28ff769d0bb642cd2643` |

Those four symbols are absent from the retained object. Their support-only
enums and macros were removed with them.

## Frozen boundary and one-shot adjudication

The wave started from exact clean cumulative Units commit
`1d52d3e2969096617d749549912bb078293d4ef7`. Its committed Units source blob
was `e4bd8cd0690ad222763900ed156571123318f9bb`: 128,549 raw Git-payload bytes,
SHA-256 `fe7651e454b5ef9821006fb5dd6f28d3dff20fe0077d604bd2f553a7dc595abe`.
That base already contained the first seven-function inventory wave and the
three-function postprocess wave and reproduced 107/189 strict target code
owners.

The immutable January target `units.obj` is 138,090 bytes with SHA-256
`e6a24ca597147dacff1f06806613d6bcfa4fa48f359dfbf542c34935f674b19f`.
The production compiler is XDK VC7 `CL.Exe` 13.00.9254.1 with SHA-256
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.

Before code generation, the source diff was frozen to exactly five bodies and
their TU-local typed support. VC7 `/Zs` succeeded and left the candidate object
absent. Exactly one natural full `units.obj` production compile then
adjudicated the fixed set. `_unit_record_damage` was strict immediately; all
four misses above were removed without another candidate-shape compile.
The subsequent retained-source build and committed replay are verification
builds, not candidate tuning.

## Claude and HCEA evidence

Every Claude Units document, recovery handoff, residual census, compiler
survey, and final consolidated source was treated as reconstruction evidence,
not as permission to import the consolidated TU. The strongest Claude carrier
is clean source blob `124f29e32497c091c4163b5faafc58dbf8890161` at
`deb5554a`. It produces 175/189 strict code owners, but it also contains
barriers, raw layout access, codegen unions, and other constructs that are not
admissible in this lane.

The Claude version of `unit_record_damage` is exact but walks attacker records
from a `char *` pointing at `damage_inflicted`, reconstructs the containing
record with `offsetof`, and casts the address back to `real *`. None of that
source was retained. The accepted version iterates a named
`struct unit_attacker *`, accesses `game_time_stamp`, `damage_inflicted`,
`object_index`, and `player_index` through their typed fields, and preserves
the independently authenticated January control flow. This ordinary typed C
nevertheless emits the same 592 padded bytes and all 14 relocation identities.

Clean HCEA reference commit
`c168af2e747d3095d9a29418ae401f3a39544863` contains semantic donor blob
`a711e668051f9430dff2c965bfad1c7c36919a69` for
`src/unit_record_damage.c`: 5,838 raw Git-payload bytes, SHA-256
`0977d5fe86fdb46d6380fb6f710c7994af0fe7327f5c423fa279176cd40105a9`.
HCEA authenticates the named attacker-record semantics. January PC COFF is
the byte, relocation, layout, and ABI authority.

The four rejected policy rewrites were useful negative evidence:

- export omitted Claude's `_ReadWriteBarrier` and used the named
  `object.functions_active_flags` field instead of raw `+0xB6` access;
- both debug selectors used that same named flag byte;
- projectile adjustment used disjoint scoped point and vector temporaries
  instead of an anonymous point/vector overlay;
- damage recording used only the typed attacker array described above.

No rejected form will be respelled or swept in this wave.

## ABI, storage, and ownership

`unit_record_damage` remains an ordinary C definition with seven promoted
stack arguments and a plain return. Existing named fields prove the 16-byte
attacker record; no private/register ABI, raw offset, packed overlay, or
undefined aliasing is used. The helper declarations are ordinary C contracts:
`game_team_is_enemy(short, short)` and
`ai_handle_killing_spree(long, short)`.

The function owns no `.data`, `.bss`, COMMON, aggregate, or writable object.
Its only new non-code owner is the target-authenticated assertion expression
for `best_new_attacker_index!=NONE`; that compiler literal receives no
standalone campaign data credit. Existing aggregate `.data` and
`_unit_globals` ownership remain unchanged and strict.

## Validation before commit

The retained source compiles to an object with 108/189 strict target code
owners, eight emitted nonexact owners, and 73 absent owners. Removing the one
new accepted body leaves all 107 inherited strict owners. The final retained
object SHA-256 before commit is
`384d4e016340573b63e9a20f2bcce6c765c7c84035299c8ead1b68056ada6853`;
the raw object hash is timestamp-dependent, while the accepted COMDAT hash and
relocations are stable.

- Complete 568-action Halo plus libcmt graph: pass.
- Semantic audit: 470 units, 4,173 functions evaluated, 4,035 semantic exact,
  103 hidden exact, 4,096 accepted exact, and zero unit errors.
- Campaign progress: 375/833 objects, 4,081/11,060 functions, 485,782 matched
  code bytes, and 1,835,088 matched data bytes.
- Admission audit: zero candidates and zero revocations; the inherited
  unrelated shell completion-label contradiction remains.
- Parked validation: three active, zero stale, zero invalid.
- Tooling suite: 179/179 tests pass.
- `git diff --check`, changed-path, protected-source, header, storage, and
  banned-construct checks are clean.

The report edge initially attempted to download absent optional `csplit` and
`objdiff-cli` binaries and failed before report generation because network is
sandboxed. Authenticated local copies (SHA-256 `708f957c...` and
`090987aa...`) were supplied as ignored build tools. The exact generated
commands—csplit, objdiff report, semantic audit, and progress—then completed
successfully. No production source or candidate object changed during that
infrastructure recovery.

Only `source/units/units.c` and this NEW Jonas ledger are eligible for the
implementation commit. No header, protected source, configuration, Claude
file, pre-existing Markdown, storage owner, completion label, or unrelated
source is changed. No committed-state replay is claimed yet; it will be added
only after a clean Jonas implementation commit, verified object deletion,
normal one-unit rebuild, regression check, direct hardened comparison, and
additive ledger-only commit.
