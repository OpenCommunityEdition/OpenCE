# `units.obj` Jonas private-foundation wave (2026-08-24)

## Result and fixed boundary

This ledger records one bounded Jonas/Codex wave from clean canonical commit
`8549f46cd0370eefba28b309e2f5db66bf204f60` on branch
`jonas/units-private-foundation-20260824`. One natural code-producing VC7
compile emitted the complete frozen closure. Fifteen January functions are
strict exact and retained, for 4,295 meaningful code bytes, 4,384 padded
bytes, and 217 relocations.

The strict external-section census advances `units.obj` from 97/189 to
112/189 exact functions. Its compiler/data COMDAT census advances from
60/129 to 94/129 exact sections. There are no inherited exact-function or
exact-data losses. The object remains `NonMatching`.

The requested update-animation closure is intentionally absent:
`code_00197f90`, `code_00198070`, `code_0019b160`, and `code_0019dff0` were
excluded because their only natural support body carries raw `+0xB6` access.
The separately owned `code_00198050`, `code_001980d0`, and `code_00198130`
were not touched. `code_00198400` was excluded because its surviving donor
shape depends on a register hint. No dead private definition was added.

## Immutable target and source provenance

- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split-object SHA-256:
  `e6a24ca597147dacff1f06806613d6bcfa4fa48f359dfbf542c34935f674b19f`.
- Canonical baseline commit:
  `8549f46cd0370eefba28b309e2f5db66bf204f60`.
- Canonical baseline translation-unit blob:
  `19a15c4a3a47849ffd8ba0c13989c0c389452519`, 118,776 payload bytes.
- Claude semantic/code donor commit:
  `deb5554ab8d16da80da934e2aa7081bc7d1f594f`.
- Claude donor translation-unit blob:
  `124f29e32497c091c4163b5faafc58dbf8890161`, 305,068 payload bytes.
- Retained pre-commit translation-unit identity:
  `06a743350d6d4edbde971fe49b9ccd60e9d5a2b8`; working payload SHA-256
  `c6f2d26ae27aa9bc24b2f0bb0533577c7127b2b424a75b8a9a058593d717ba5c`.
- The sole code-producing candidate object is 100,335 bytes with raw
  phase-specific SHA-256
  `5a796149467e2822ab84d8bdd57bb7b5ac150abc931ab49ceda7bf90378a29ac`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `xbox/bin/vc7/CL.Exe` has SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  Natural flags remain `/O2 /Oy- /DDEBUG /Dxbox` with the repository include
  paths.

The checked-in Claude ledgers for the private-helper, animation-impulse,
animation-state, and Euler-plan lanes supplied the measured source history.
The clean HCEA reference at `c168af2e747d3095d9a29418ae401f3a39544863`
and the HCEA PDB atlas supplied semantic names and later-build type/lifetime
hints only: `code_00197e30` is later named `unit_euler_axis_doplan`, and
`code_00198fd0` is later named `unit_can_play_animation_impulse`. HCEA is not
January byte, ABI, scope, or layout authority. January COFF, target callers,
and the hardened comparator remain the admission authority.

## Strict exact admissions

All hashes below are over relocation-normalized padded COMDAT bytes. Meaningful
sizes and RVAs come from the January ordinary report; padded sizes,
relocations, destinations, addends, and hashes come from the hardened COFF
comparison.

| Function | January RVA | Meaningful / padded | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_code_00197e30` | `0x00197E30` | `319 / 320` | 9 | `774bc6849b5a2f1f57ff151ca5c3cd67ac40e6b5cdc017e038c99858200e5f66` |
| `_code_00198170` | `0x00198170` | `29 / 32` | 0 | `63bd6be90ca89741efca2f9e68c76a3e65da2d0087e3f7d854a68b9efe83103c` |
| `_code_00198190` | `0x00198190` | `96 / 96` | 5 | `11075223c5a90423430ab4f6404e6f591820349ada2575102303575aaded0319` |
| `_code_001981f0` | `0x001981F0` | `57 / 64` | 4 | `60bfd4bf68ea5e3a4dfd0ec6ec0dcd6b495bf8e34e3808117de190fb5687f5e7` |
| `_code_00198230` | `0x00198230` | `62 / 64` | 0 | `65d343d25a811a43ef1e186517fd09f0b8b1341642392466d6226e74ce551687` |
| `_code_00198e40` | `0x00198E40` | `399 / 400` | 33 | `b7afd27757e11efa83b8d75cd37f2ff2df2508518c97da1fe1e24e9bb0f7bad1` |
| `_code_0019a170` | `0x0019A170` | `216 / 224` | 4 | `48ec4825c898e5629cbb329e713372e94cedce56c050d4a030a791c61a797620` |
| `_code_0019a640` | `0x0019A640` | `46 / 48` | 1 | `ebbb9a02adc57b1ea7825ce48db2c175aa3c2d0e0988d73af378f9e070bb04e9` |
| `_code_0019b0b0` | `0x0019B0B0` | `170 / 176` | 10 | `a798fa2bad8f3d82260ec97d08f28349a8ea5046b226415674f4f4d31f71a479` |
| `_code_0019b600` | `0x0019B600` | `113 / 128` | 8 | `c21a1302f3b82a1d388b9b96218db4df4b95e1adc7f00256542531ce96494673` |
| `_code_0019bf70` | `0x0019BF70` | `1263 / 1264` | 74 | `d92f0967743c782b5ba5beabf67309370b5511798ce1f5704d91eac5e52389d0` |
| `_code_0019c460` | `0x0019C460` | `502 / 512` | 20 | `b2f5525b4fb9487ff0e90d2d1a4d1eebf30b57851ac6898af6bb45ea8091bc97` |
| `_unit_set_actively_controlled` | `0x0019D800` | `162 / 176` | 3 | `c1f3be9876879e52e57a5e5eac45695f95001467deafaaab122cfa31842abf52` |
| `_unit_cause_melee_damage` | `0x0019E130` | `585 / 592` | 27 | `23be8d86830161cba5055c1e4b5f3b3be85c43a208c53ce2aaf117dcb82f7d0a` |
| `_code_0019ea70` | `0x0019EA70` | `276 / 288` | 19 | `e529b4781c38a6cd0b7bf7ee9b527a8c313cfc074adf2a3406cd44244a0c438c` |

The barrier-free `code_0019bf70` body remains literally exact: deleting the
donor `_ReadWriteBarrier()` changes no emitted byte or relocation under VC7.
This converts what was a compiler-control donor shape into policy-clean
ordinary C without sacrificing the January identity.

## Natural closure and per-body adjudication

Unreferenced static definitions are eliminated by VC7 `/O2`, so every retained
private body has a real same-TU caller. The dependency closure is:

| Natural caller/support | Required exact private bodies | Result |
| --- | --- | --- |
| `unit_preprocess_node_orientations` | `code_00198170` | caller support nonexact; private exact |
| `unit_animation_set_state` | `code_00198190`, `code_001981f0`, `code_00198230`, `code_0019b0b0` | caller support nonexact; four private exact |
| `unit_start_animation_impulse` plus `code_00198fd0` | `code_00198e40`, `code_0019b0b0`, `code_0019ea70` | caller/helper support nonexact; three private exact |
| `unit_set_actively_controlled` | `code_0019a170` | caller and private exact |
| `unit_add_weapon_to_inventory` | `code_0019a640` | caller support nonexact; private exact |
| `unit_cause_melee_damage` | `code_0019b600` | caller and private exact |
| `unit_euler_aiming_update` plus `code_0019bf70` | `code_00197e30`, `code_0019c460` | public caller support nonexact; all three private bodies exact |

The six nonexact support COMDATs are retained only because deleting any one
would remove one or more exact private admissions:

| Required support | Candidate padded / relocs | Candidate SHA-256 | January padded / relocs | January SHA-256 |
| --- | ---: | --- | ---: | --- |
| `_code_00198fd0` | `208 / 8` | `197a8fec69bd7f994c2dfbf6c7161128e732248d9e9218549ae59d8fa0ad0d09` | `208 / 8` | `ec86b60dd8c4768d86bb222b94ccbb326d8e76a735647a245fe1f915f7738199` |
| `_unit_start_animation_impulse` | `288 / 11` | `16de7a3c3aa69b158a3f3368be7d2a21418126ca1ab472beebbf0ab072facfbf` | `288 / 11` | `db9ae277a81fe8df84632bfeb74678459480eff07df3a181eb0c65a80b5f1416` |
| `_unit_add_weapon_to_inventory` | `304 / 11` | `d9588c71ed2d832177802ae925c962d36a453d1edf32c2ffce6663a9bf33cd20` | `304 / 11` | `de883e04c6d062bfa397e4730c0b0b94ba4176d2f6d7f1a2a20d187bf06692c6` |
| `_unit_animation_set_state` | `1408 / 84` | `95a6c15ef9fcf38d2588e184c5b4d39fa9d6fe4b3ac47c2feb7a3d4f205f64f5` | `1408 / 84` | `2597b15db86c021e8347516930759d42dab6cdbb30387dddc5825b5141802a29` |
| `_unit_preprocess_node_orientations` | `1920 / 87` | `a5c3016d484fe073ca760b54334db298d465ca42e01deef6cd07b8a928f1f613` | `1920 / 87` | `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed` |
| `_unit_euler_aiming_update` | `1744 / 78` | `12a6cad099dc6d76367943847d86ac4387edd06bdb91206a62f26fb3467e1c61` | `1744 / 78` | `f91399d1f28dc103336e3538dde43586a4a4658ce04bd9c4bc6a9b6dd2c4ed63` |

The repository header ABI is unchanged. `unit_start_animation_impulse` keeps
the canonical public `short animation_impulse`; `unit_add_weapon_to_inventory`
keeps the canonical public `long is_starting_weapon` and narrows it once into
a readable local `short mode`. Their target signatures differ in the final
Claude reconstruction, so the two public bodies receive no credit and were
not tuned after the one-shot result.

## One-shot and policy record

The exact natural command was first exercised with `/Zs`. That parse-only
check emitted no object and exposed missing source-local constants and the
typed material-definition include; those prerequisites were added and the
final parse-only check passed. The next and only code-producing `units.c`
compile contained the frozen packet. There was no alternate body compile,
source-shape retry, scheduling tune, or post-result recompilation.

The retained source is readable typed C. It contains no assembly, raw address
or byte-offset access, pointer/integer reconstruction, pointer or union pun,
inactive-union access, undefined behavior, `volatile`, `register`, force-inline
annotation, optimizer pragma, compiler barrier, synthetic anchor, or
byte-forcing expression. No header, protected file, configuration, semantic
ledger, parked record, storage declaration, or other translation unit changed.

## Data and ownership

The candidate preserves the baseline 1,564-byte `.data` aggregate and 4-byte
`.bss` aggregate exactly. This wave adds no source storage owner and changes no
COMMON symbol. Its `.rdata` increases from 2,751 to 3,945 bytes solely through
compiler literals required by the natural functions; the January target owns
4,951 `.rdata` bytes. Thirty-four additional literal/real COMDATs are strict
exact and no inherited exact data COMDAT is lost. No data credit is claimed
for unmatched literals or for debug/directive sections.

## Pre-commit validation

- Complete 569-action `halobetacache_build` and `libcmt_build`: pass. The
  already-produced one-shot `units.obj` was up to date and was not rebuilt.
- Hardened `units.obj` census: 112/189 strict external functions and 94/129
  strict external data sections; exactly the fifteen named functions are new,
  and inherited exact losses are empty.
- Semantic audit: 470 units, 4,184 functions evaluated, 4,039 semantic exact,
  104 hidden exact / 62,428 hidden code bytes, 4,100 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 complete objects, 4,084/11,060 exact functions,
  486,863/2,198,102 meaningful code bytes, and 1,835,088/4,176,062 matched
  data bytes.
- Halo progress: 273/468 complete objects, 3,917/7,574 exact functions,
  473,949/1,770,166 meaningful code bytes, and 1,829,896/3,923,451 matched
  data bytes.
- Object admission: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass.
- `git diff --check`, deleted-path, changed-path, source-policy, protected-file,
  and frozen-scope checks pass. Only `source/units/units.c` and this new
  Jonas-owned ledger are tracked changes.

The generic pre-wave whole-TU manifest was also run without adjudications and
did what it is designed to do: it returned nonzero on the expected large
COMDAT, literal, symbol-set, and physical-section expansion. It identified
exactly the fifteen admissions as `NEWLY_EXACT`. Its `changed_nonexact` list
contains the six retained closure COMDATs plus nine downstream/nonexact
compiler-context changes; none was exact in the baseline or is claimed here.
Because that manifest fingerprints physical ownership and deliberately does
not grant new credit, it is not used to waive the findings. The independent
hardened semantic comparison proves all 97 inherited exact functions and all
60 inherited exact data COMDATs remain strict.

## Committed-state replay

The clean committed-state snapshot, forced same-path object rebuild, direct
comparison, and ledger-only replay record will be appended after the
implementation-and-ledger commit. Nothing is pushed, amended, removed from
history, or force-updated.
