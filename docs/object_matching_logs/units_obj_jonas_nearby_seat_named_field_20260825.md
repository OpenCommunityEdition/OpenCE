# Units nearby-seat named-field recovery (Jonas, 2026-08-25)

## Result and bounded scope

This one-function wave starts from clean cumulative Units commit
`143c703b5292df30a564abfee426b4bd94d64e08`. It reconstructs
`unit_find_nearby_seat` in ordinary typed C, adds the missing
translation-unit-local external declaration for `ai_try_vehicle_eviction`, and
changes no shared header, protected source, configuration, storage owner,
semantic exception, parked-function record, or completion label.

The retained function is strict exact on the only edited-source production
compile:

| January owner | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_unit_find_nearby_seat` | 645 / 656 | 16 | `106fadafcdc9abaee4e1ae31ab19b8227ecc5c1fdacab85762c0035fbc1faad1` |

The complete direct census advances from 167 exact / 12 emitted nonexact / 10
absent to 168 / 12 / 9 across all 189 January function owners. The sole gain
and sole newly emitted target owner is `_unit_find_nearby_seat`; every one of
the 167 inherited exact owners remains exact. A candidate-versus-baseline
comparison covers all 210 common emitted code owners and finds zero changed or
removed sections; the nearby-seat owner is the only addition.

The object remains `NonMatching`. The retained source contains no raw offset,
pointer pun, union alias, undefined behavior, `volatile`, `_ReadWriteBarrier`,
pragma/intrinsic, attribute, `register`, inline assembly, byte patch,
code-generation anchor, or comparator exception.

## Immutable authority and corrected baseline

- January `cachebeta.exe`: 6,584,672 bytes, SHA-256
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `source/units/units.obj`: 138,090 bytes, SHA-256
  `e6a24ca597147dacff1f06806613d6bcfa4fa48f359dfbf542c34935f674b19f`.
- Cumulative source blob at `143c703b`:
  `9d6b683353a8bc47aa87ed1ae71ba2a49e9029e8`.
- Clean rebuilt cumulative object: 141,868 bytes, SHA-256
  `f4a610d884067730b747a7006e3167bc7be29abf3c5d826d97d16b4fa85c4fed`.
- Frozen retained source before commit:
  `a2d6ae15ed26a052c99290d65f7168b01e2ba074`.
- Immutable first-shot object: 143,148 bytes, SHA-256
  `b3528d6f6d78dde2a3e776000b2f1d30042f7fe9e267d843500ed98791090fa9`.
- XDK 3911 VC7 `CL.Exe` 13.00.9254.1: 81,920 bytes, SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.

The initially copied object was rejected before candidate emission because it
was the stale pre-declaration cumulative object: 140,865 bytes, SHA-256
`49f66a5a3f0e384325c5a63a46e62b4d133310182dae308a4314021499893fc9`.
That identity is recorded in the immediately preceding animation-update
declaration ledger as the object built from the older `b0ef2704` state. The
edited source was restored; both `git rev-parse
HEAD:source/units/units.c` and `git hash-object source/units/units.c` proved
the clean `9d6b6833...` blob. A clean cumulative rebuild supplied the corrected
object above, which was copied and frozen before the source packet was
reapplied. Thus the only edited-source code emission below is measured against
the exact assigned cumulative commit.

## Claude documentation, source provenance, and tooling

The complete dedicated `units_obj_nearby_seat.md` experiment record was read
before choosing source. Its UNS-0 through UNS-4 matrix establishes the
behavioral topology and the code-generation-sensitive distance calculation:
three distinct typed locals named for entrance distance, seat distance, and
their selected minimum are required to retain January's x87 temporary shape.
UNS-4 records the same 656-byte / 16-relocation target hash reproduced here.
The consolidated `units_obj.md`, final-topology, compiler-provenance,
crossbuild-provenance, donor/source-provenance, and current Jonas cumulative
Units ledgers were also treated as evidence. Historical source was never used
as a patch authority.

The readable exact body is independently present in Jonas commit
`4df1c4389a16588a0b30c6bbe1cc7a2de649a185` (`Match Units nearby seat
selection`) and in the authenticated final Claude Units carrier lineage. The
carrier's raw `unit_debug_function_is_active(target_unit)` spelling was not
retained. The current `source/objects/objects.h` is Git blob
`738c831f77a73e2dc3a1e7e9443144857c480ce8`, identical to the audited Claude
carrier layout: `object.damage_flags` is the named word at datum offset
`0xB6`, and `_object_dead_bit` is its named bit two. Therefore the ordinary
expression

```c
!TEST_FLAG(target_unit->object.damage_flags, _object_dead_bit)
```

states the target access without a raw address or layout helper. The strict
first-shot bytes independently authenticate that mapping.

The cumulative headers intentionally do not yet expose the historical nearby
seat result enum or seat flag bit nine, and this wave does not own them. Two
ordinary function-local enums preserve the authenticated names and values
`none = 0`, `occupied = 1`, `available = 2`, and
`requires_driver_bit = 9`. They have no storage or ABI. The only external
support declaration added is:

```c
boolean ai_try_vehicle_eviction(
	long actor_index,
	long entering_unit_index,
	boolean immediate);
```

This boolean declaration corrects the implicit-C89 return/call boundary while
matching the independently declared caller in `players.c` and the recovered
AI behavior. No header edit is needed.

The authenticated csplit v0.0.2 and objdiff-cli v3.3.1 binaries have SHA-256
`708f957c37caa66df90ade35d1e65e2617e2cc2c8c2ccf68d8bf5cb20e752`
and `090987aa22c0fe9b7d252b2b44c2c0c92c5dd3e9b5965d353060802226a13677`.
The normal generated VC7 rule remains `/nologo /c /O2 /Oy- /DDEBUG /Dxbox`
with the repository include graph unchanged.

## Parse-only gate and sole candidate emission

The first VC7 `/Zs` pass failed closed before object emission because the
header-free cumulative tree lacked the four constants described above. After
adding only the function-local enums, `git diff --check`, the one-path scope
audit, and the prohibited-construct scan passed. A second production-flag
`/Zs` invocation succeeded and proved that the corrected baseline object was
still byte-identical.

The next and only code-producing action on edited source was the normal
generated edge:

```text
[1/1] CL build\base\source\units\units.obj
units.c
```

The first-shot object was copied immutably before adjudication. Hardened
padded-byte, relocation-masked hash, and complete relocation
address/type/destination/addend comparison returned `all_equal: true` for all
656 bytes and 16 records. There was no retry, source tuning, candidate pruning,
compiler-option change, or second edited-source emission.

## Runtime-data and ownership boundary

Across all 129 January `.rdata`, `.data`, and `.bss` owners, the corrected
baseline is 125 strict exact / zero present nonexact / four absent. The
candidate is 126 / zero / three. The sole exact data gain is the natural
assertion literal `parent_seat_index != NULL`, which is referenced by the
recovered function and is byte/relocation exact. No inherited target data
owner is lost.

The broader candidate-versus-baseline runtime-owner A/B contains 182 common
owners with zero changed fingerprints, zero removals, and two additions: the
exact assertion literal and the natural `1.5f` real COMDAT used for driver-seat
distance scaling. The latter is candidate-only and receives no target credit.
Writable `.data`, BSS storage, and every existing runtime owner remain
unchanged.

## Full pre-commit gates

- Full `halobetacache_build libcmt_build`: pass, all 568 copied-build catch-up
  edges. The already-adjudicated Units object was current and did not compile.
  Warnings were inherited from unrelated translation units.
- Authenticated direct csplit and objdiff report generation: pass. The report
  is 1,573,455 bytes with SHA-256
  `34258ba197881ea1e5ba773d6946caad5016ada45fd6a014ea943932c35b69ee`.
- Semantic audit: 470 units, 4,237 functions evaluated, 4,095 semantic exact,
  114 hidden exact / 64,705 hidden code bytes, 4,156 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 objects, 4,130/11,060 functions, 501,438 accepted
  code bytes, and 1,835,088 accepted data bytes. Halo is 273/468 objects,
  3,963/7,574 functions, and 488,524 accepted code bytes.
- Object admission: zero candidates and zero revocations; the sole
  contradiction is the inherited unrelated `source/shell/shell_xbox`
  completion label.
- Parked validation: three active, zero stale, zero invalid.
- Complete tooling suite: 179/179 tests pass.
- Direct function census, target-data census, common-code A/B,
  runtime-owner A/B, source scope, policy scan, and `git diff --check`: pass.

The generated report target initially attempted to refresh already
authenticated downloader edges, but sandboxed network was unavailable. No
source or object changed. The locally authenticated csplit and objdiff binaries
were invoked directly, after which semantic/progress and all remaining gates
passed. This operational detail grants no exception.

## Committed-state replay plan

No committed-state replay is claimed in the implementation commit. After the
source and this initial new ledger are committed by Jonas Volman and the
worktree is clean, a one-unit accepted-state regression snapshot will pin that
commit. The generated Units object path will be resolved and proven inside
this isolated worktree, deleted with literal-path semantics, rebuilt once by
its ordinary Ninja edge, and checked immediately. Direct code/data censuses,
the nearby-seat fingerprint, and a final Ninja dry run will be repeated before
one additive ledger-only replay commit. The same sequence will then be run at
corrected HEAD. No push, amend, rebase, history rewrite, or worktree removal is
performed.
