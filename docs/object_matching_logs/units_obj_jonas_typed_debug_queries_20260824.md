# Units typed debug-query recovery (Jonas, 2026-08-24)

## Result and boundary

This bounded wave starts from exact clean cumulative Units commit
`266fe153868887f36f6cddc230a7c1055dfe0ec5`. It authenticates the three
remaining Claude-era exact owners whose recorded source raised a current
natural-source policy question:

- `unit_export_function_values(long unit_index)`;
- `units_debug_get_next_unit(long current_object_index)`; and
- `units_debug_get_closest_unit(long reference_object_index)`.

The two debug queries have complete named typed support in the current tree.
Their historical readable bodies were recovered with the numeric dead-bit
test replaced by the existing `_object_dead_bit` name. Both were strict exact
on the only code-producing candidate compile and are retained. The exporter
is deliberately not reconstructed: Claude's complete 30-experiment ledger
shows that its exact pointer setup depends on `_ReadWriteBarrier()` after the
unit-definition getter. That compiler fence is outside the natural-source
boundary. No exporter body was added or compiled in this wave.

The retained implementation uses no raw offset, pointer pun, union alias,
undefined behavior, `volatile`, `_ReadWriteBarrier`, pragma/intrinsic,
attribute, register forcing, inline assembly, byte patch, code-generation
anchor, or comparator exception. No header, protected source, configuration,
storage owner, completion label, semantic ledger, parked-function record, or
pre-existing documentation is edited. The only production source edit is 90
lines in `source/units/units.c`.

## Authority and evidence reviewed

January PC remains the byte authority. The authenticated artifacts are:

| artifact | bytes | SHA-256 |
| --- | ---: | --- |
| `cachebeta.exe` | 6,584,672 | `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520` |
| split `source/units/units.obj` | 138,090 | `e6a24ca597147dacff1f06806613d6bcfa4fa48f359dfbf542c34935f674b19f` |
| corrected cumulative pre-wave object | 134,343 | `338c22c3fe456bd1056516a9c8c2b1eb6ab3a019d635f0f0c21f300308fec3ff` |

Before choosing source, all relevant Claude Units evidence was read: the full
consolidated `units_obj.md` history; the dedicated
`units_obj_export_functions.md` UEF-0 through UEF-30 record at final commit
`751ea0bf`; the exact historical debug-query bodies at `f2f7b6b8` and
`cee81f83`; the carrier/caller evidence; and the current Jonas Units ledgers,
including the immediately preceding typed-declaration pair. The campaign
README, house rules, exact-match acceleration playbook, semantic/progress,
admission, parked, regression-gate, and hardened COFF comparator tooling were
also reviewed before classification. Historical source is reconstruction
evidence, not a patch authority.

The current `source/objects/objects.h` is Git blob
`738c831f77a73e2dc3a1e7e9443144857c480ce8`, identical to the final Claude
exact tree's blob. Its ordinary layout places `object.damage_flags` at datum
offset `0xB6`; `_object_dead_bit` is the named value two. The unit extension
begins at `0x1A4`, so its first two typed longs are
`unit.actor_index` at `0x1A4` and `unit.swarm_actor_index` at `0x1A8`.
The repository already independently asserts the actor offset in `ai.c`.
Thus every target access is expressible through a current named field; no
header or layout edit is required. The strict first-shot result below is an
additional emitted-code proof of those mappings.

## January target, relocations, and callers

The three target sections are independently fingerprinted as follows:

| January owner | meaningful / padded | relocs | normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_unit_export_function_values` | 380 / 384 | 18 | `4149e4cd513cba59c251998986db6acd8857e41f96d6182b9692023361b20c35` |
| `_units_debug_get_next_unit` | 228 / 240 | 8 | `5098e32812b1e3e53fcb6201a48145c446f16b5ca7146f3bcee4c7bb29632879` |
| `_units_debug_get_closest_unit` | 197 / 208 | 6 | `de1f11019c5719a10ef09ec6987d302764cebcc2f9ffdb78fd2747254870034c` |

The next-unit relocations are exclusively two iterator initializations and
six iterator advances. The closest-unit relocations are one iterator
initialization, three advances, two origin queries, and the zero-real COMDAT;
`distance3d` is naturally inlined. The exporter calls
`object_get_and_verify_type`, `tag_get` twice, and
`tag_block_get_element_with_size`, with the remaining relocations accounting
for its real constants and internal jump table.

The exporter is address-taken by the January `_unit_data_definition` callback
table in `source/objects/object_types.obj` (relocation offset 212). The two
debug functions have no same-TU caller; both are called by January
`_code_000a79c0` in `source/game/player_control.obj` (relocation offsets 364
and 374). These uses agree with ordinary external cdecl definitions. No
prototype or ABI invention is needed.

## Why the exporter fails closed

Claude's UEF matrix establishes the exporter's complete behavior and nearly
all of its natural lowering. UEF-1 through UEF-7 exhaust ordinary typed
pointer initializers, assignments, declaration order, constness, and typed
member/`offsetof` address forms. The best natural setup remains 384 bytes and
18 relocations but schedules the independent output pointer and getter result
differently from January. UEF-8 first makes that setup byte-exact only by
placing `_ReadWriteBarrier()` immediately after the definition getter.
UEF-9 through UEF-30 then resolve the independent signed-short comparison
shape while retaining that fence; the final exact source at `751ea0bf` still
contains it.

The record therefore supplies no policy-clean natural source packet for this
owner. Raw `+0xB6`, a barrier, volatile forcing, attributes, register/lifetime
forcing, or a new unsupported source-shape sweep would merely restate a
closed code-generation problem. The exporter remains absent and receives no
compile attempt or credit in this wave.

## One-shot typed adjudication

The two debug bodies were frozen together before emission. `git diff --check`
passed, and a real VC7 `/Zs` parse-only invocation succeeded without touching
the copied baseline object. The next code-producing action was exactly one
normal Units production compile:

```text
[1/1] RUN configure.py
[1/2] CL build\base\source\units\units.obj
units.c
```

`configure.py` is the generator edge; there was exactly one Units code
emission. The immutable first-shot candidate object is 135,341 bytes with
SHA-256
`9bd0c61cf76e79bc31d18229baca148ee40861e00a18e2b0b91ad88f168c43ea`.
The frozen source hashes as Git blob
`26a4f43aebcb230a7e3100ad0a29a52b7276ecab`.

Strict padded-byte, relocation-masked hash, and complete relocation
address/type/destination/addend comparison produced:

| January owner | result | padded | relocs | normalized SHA-256 |
| --- | --- | ---: | ---: | --- |
| `_units_debug_get_next_unit` | retain, strict exact | 240 | 8 | `5098e32812b1e3e53fcb6201a48145c446f16b5ca7146f3bcee4c7bb29632879` |
| `_units_debug_get_closest_unit` | retain, strict exact | 208 | 6 | `de1f11019c5719a10ef09ec6987d302764cebcc2f9ffdb78fd2747254870034c` |

The strict gain is two functions, 425 meaningful bytes, 448 padded bytes,
and 14 exact relocations. There was no retry, pruning, or source tuning.

## Corrected baseline and whole-object preservation

The initially copied integration-lane `build/base` object was stale: it
contained two in-progress exact bodies that are not present in tracked commit
`266fe153`. Treating that artifact as the baseline would have falsely
reported a two-function loss. It was rejected before acceptance. The corrected
baseline is the clean committed replay object from the preceding typed-pair
lane. Its source blob is exactly `3fd40fa3885b465b0169d86ae3ac506304a508c8`,
which is also the tracked `units.c` blob at `266fe153`; its object identity is
the `338c22c3...` row above. The emitted candidate itself was never replaced
or recompiled during this correction.

Against that authenticated baseline, the established 189-owner hardened
census advances from 158 exact / 11 emitted nonexact / 20 absent to 160 exact
/ 11 emitted nonexact / 18 absent. The exact-set delta is precisely the two
debug queries, and no inherited exact owner is lost.

The complete 129-section January runtime-data census is unchanged at 121
strict exact / two present nonexact / six absent. A separate candidate versus
baseline runtime-owner A/B contains 180 owners on each side: all 180 are
common, none is added or removed, and every acceptance fingerprint is
identical. Writable `.data` remains 1,564 bytes, seven relocations, normalized
SHA-256 `6ed61c39a648905cf23b12b916e2adadfe97b6e5aa492c29a14c7265809ef1a7`;
`_unit_globals` remains four BSS bytes, zero relocations, with no raw payload.
There is no `.data`, `.bss`, COMMON, aggregate, literal, or storage-owner
change and no data credit is claimed.

## Pre-commit gates

- Full `halobetacache_build libcmt_build`: pass, all 568 catch-up edges. The
  frozen Units object was up to date and did not recompile; warnings were
  inherited from unrelated translation units.
- Authenticated local csplit and objdiff report generation: pass. The report
  is 1,573,188 bytes with SHA-256
  `01e2fade84abc9d1bc74b48244842e62bf259ec042bfebf0aca2fe0f40dc9e5f`.
- Semantic audit: 470 units, 4,228 functions evaluated, 4,087 semantic exact,
  110 hidden exact, 4,148 accepted exact, and zero unit errors.
- Campaign progress: 375/833 objects, 4,126/11,060 functions, and 500,656
  accepted code bytes.
- Object admission: zero candidates and zero revocations; only the inherited
  unrelated `shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, zero invalid.
- Complete tooling suite: 179/179 tests pass.
- Source scope, policy scan, target/caller audit, data/ownership A/B,
  `git diff --check`, and changed/deleted-path review: pass.

The compiler is Microsoft VC7 `13.00.9254.1`; `xbox/bin/vc7/CL.Exe` is
81,920 bytes with SHA-256
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
The natural `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` production rule and include
graph are unchanged.

The copied Ninja log tried to refresh already-installed downloader edges when
the report phony target was invoked, but sandboxed network was unavailable.
No source or object changed. The already-authenticated installed csplit and
objdiff binaries were invoked directly, after which semantic/progress and all
other gates passed. This operational detail grants no exception.

## Commit/replay boundary

This initial record claims only the frozen one-shot evidence above. The next
steps are to commit exactly `source/units/units.c` and this new ledger as
Jonas; capture a clean implementation-state regression manifest; verify that
the generated object resolves under this isolated worktree; delete only that
object; perform one normal same-path rebuild; and immediately rerun the full
regression gate, direct two-name comparison, complete 189/129 censuses,
runtime-owner A/B, and no-work dry run. Actual replay evidence will be
appended to this ledger in one additive ledger-only commit. No amend, push,
history rewrite, configuration change, or worktree removal is authorized.
