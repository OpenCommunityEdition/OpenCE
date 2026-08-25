# `units.obj` Jonas damage / lifecycle closure (2026-08-24)

## Result and frozen boundary

This Jonas-owned wave starts from clean cumulative Units commit
`4083cc99d7ab45da4d4c03f795d7b11be254fbf2` on isolated branch
`jonas/units-damage-lifecycle-closure-20260824`. Before source emission, the
boundary was frozen at ten Claude-reconstructed damage/lifecycle candidates.
`code_001a0cf0` and `unit_died` were admitted only as natural nonexact support
where a retained real caller required them. `unit_record_damage` was excluded
because a separate typed-policy wave owns it.

One parse-only `/Zs` prerequisite and exactly one natural candidate compile of
the generated `units.obj` path produced eight strict matches:

| Retained January owner | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_unit_cause_continuous_melee_damage` | 663 / 672 | 28 | `8277797c34b144277f884e34650bd6fc12b1b809a25fadec524ba76aa1e8a688` |
| `_unit_impact_melee_damage` | 569 / 576 | 23 | `540e3ec8b45b9e35b475cf0be79d74c11a33adf7e795de508631aab1bb9562cd` |
| `_unit_detach_from_parent` | 307 / 320 | 14 | `a7ffb8434da04169f1f4968c7a21cad1d5131c947ff0f99dd76cbbb304cfbb5b` |
| `_unit_start_flaming_to_death` | 153 / 160 | 5 | `036ce64f848be8778ebd0e24676ad57dc6812b38eb433a3d523213a37066f7d9` |
| `_unit_start_running_blindly` | 193 / 208 | 7 | `44a6d5ff2d1ae369252139d49aecd90150ecfada55afd41297e128bd8e6fe9fc` |
| `_unit_running_blind` | 732 / 736 | 49 | `4574e6deff6d9675aae003830c5213a8e337cc78fe03a411d4b64f6f274f1f65` |
| `_unit_flame_to_death` | 255 / 256 | 11 | `a95b7ac2f1baf5bb6a7e4a743a8155303564be287fbb1922c670ab933ad0829c` |
| `_unit_unsuspecting` | 164 / 176 | 6 | `d7a54adb7e11657b168a762294f4b9ffeee5ab7e778612f64486687dd4c40a0c` |

The retained closure adds eight exact code owners, 3,036 meaningful bytes,
3,104 padded bytes, and 143 exact relocation identities. The other two fixed
candidates missed on the first shot and were removed without a spelling,
declaration, expression-order, control-flow, type, or compiler-option retry:

| Rejected fixed owner | Target padded / relocations / hash | First-shot padded / relocations / hash | Final state |
| --- | --- | --- | --- |
| `_unit_damage_aftermath` | 1,600 / 60 / `8bb721e70465d8c4c3d09069f879176ddc08a639d1e67c75fe525f9d367870dd` | 1,600 / 60 / `5d91685865cd2258581718b33bd80ef69e6587098792654b93257d26351337cd` | absent |
| `_unit_test_spawning` | 176 / 6 / `d30a923ccc6986e4548167d5d9f2f323bdc08b25a1b1092b0a2c030e25535ccc` | 176 / 6 / `81e293c9b29245eb35a7747b523a0a22af672ef77bfb590fa4bc19c5a3b186e4` | absent |

The aftermath body historically depended on a shared `game_engine` formal
type change. Shared headers were outside this wave, so the donor body was
tested only against the current canonical typed interface and then rejected
fail closed. No header delta was imported to rescue it.

`code_001a0cf0` was emitted only to support the aftermath candidate. It also
missed with the correct 1,568 padded bytes and 43 relocations: target hash
`365511af9b5f31d519ee339975e9f667004f04caa4d8c868266e41c17278fc63`
versus first-shot hash
`b207eb6a3c649ac22355ec45af62bfb4bcd26844ffb0fce56f3040c49aaba5cc`.
After aftermath was removed, `code_001a0cf0` had no retained caller and its
body, prototype, include, constants, and support declarations were removed
wholesale.

`unit_died` is retained only because the already-emitted `unit_update` has a
real direct call to it. It remains explicitly nonexact: January is 624 padded
bytes, 28 relocations, and hash
`8b37840499ffc8d0aabd052b0f3ba89f877264935d2f7e53a66186bb9000cb77`;
the natural readable body is 608 padded bytes, 28 relocations, and hash
`6ec5ddafaa1405f60d6282c0f83fda4cd4383f75c0ca40f2fbe4574c41202806`.
No credit or completion claim is made for it.

`units.obj` remains `NonMatching`. No header, protected source, build
configuration, semantic exception, parked-function record, storage owner,
completion label, or pre-existing documentation is edited.

## Immutable inputs and compiler provenance

- January executable `cachebeta.exe`: 6,584,672 bytes, SHA-256
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January csplit `source/units/units.obj`: 138,090 bytes, SHA-256
  `e6a24ca597147dacff1f06806613d6bcfa4fa48f359dfbf542c34935f674b19f`.
- Starting canonical `source/units/units.c` Git blob
  `a972ea79b6451e1a2a64a829942b0f812256b9d3`: 182,749 payload bytes.
- Final retained source before commit: Git blob
  `9ffdce31d8ac5d607ed0187ee01ca2a584ec2f85`, 202,075 raw working payload
  bytes, raw SHA-256
  `95a664c856164cad1846a8b082774ff5037e73af638262951bafd0c41c153c3b`.
- Immutable one-shot twelve-body object
  `build/audit/units_damage_lifecycle_firstshot.obj`: 123,694 bytes,
  SHA-256
  `2940f76e63634963f0b8cfc1a75e800bc3c99d4ce83ad0b87cc1e7207e6abd66`.
- Final retained-source pre-commit object: 116,959 bytes, phase-specific
  SHA-256
  `bdc8baf24cfeb703e3d9fea6697b859557756a18f8ef222e9335a5f952b65a0f`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `xbox/bin/vc7/CL.Exe` is 81,920 bytes with SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  The repository's natural `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` production
  rule and XDK 3911 include graph are unchanged.

The pre-emission hardened January owner census was 127 exact, 12 emitted
nonexact, and 50 absent. The final retained-source census is 135 exact, 11
emitted nonexact, and 43 absent. Removing the eight retained owners from the
final exact set yields all 127 inherited exact owners, proving zero inherited
loss.

## Claude evidence and authenticated donor

All relevant Claude Units reconstruction evidence was read as evidence, not
as authority to transplant unrelated source. This included the complete
final `units_obj.md` ledger and the specialized lifecycle, running-blind,
detach-parent, damage-aftermath, death-record, melee-damage, combat,
small-residuals, `code_001a0cf0`, compiler-provenance, and final-topology
ledgers. The current Jonas Units ledgers were also read to preserve the
cumulative caller topology and ownership boundaries. The repository matching
methodology plus hardened COFF, semantic, progress, regression, admission,
parked-function, and transfer-tool documentation governed acceptance.

The most complete readable Claude translation unit was authenticated at tree
state `deb5554ab8d16da80da934e2aa7081bc7d1f594f`. Its
`source/units/units.c` blob is
`124f29e32497c091c4163b5faafc58dbf8890161`, with raw payload SHA-256
`173fd6fdcc09d979ba6507aefa2cd77790f8f17f93413b9acbb679c32bb8f7c4`.
Each emitted fixed body was extracted from that authenticated source and
checked byte-for-byte at the source-body level before compilation. Only the
fixed bodies and the minimum current-TU typed support were applied. No
historical header, configuration, storage, or documentation change was
imported.

The historical ledgers independently establish the important constraints
honored here:

- the running-blind caller/private-helper pair is a single natural cluster;
- continuous melee damage is private and must remain called only by the real
  unit update topology;
- flaming death uses the falling-damage tag-block element layout;
- `unit_unsuspecting`, detach, and start-death bodies are ordinary public
  cdecl functions;
- aftermath's former exact result depended on the historical game-engine
  formal type and cannot authorize a shared-header edit in this bounded wave;
- spawning was a known first-shot-sensitive residual and must be removed if
  strict comparison fails;
- `unit_died` and `code_001a0cf0` are nonexact support, never exact credit;
- `unit_record_damage` belongs to its separate typed-policy closure.

## Typed behavior and ownership

The implementation is readable typed C. It contains no assembly, inline
intrinsic, `volatile` scheduling device, optimizer pragma, attribute,
explicit register convention, raw-address access, pointer/union pun,
inactive-union read, undefined overflow, synthetic anchor, serialized-byte
forcing, or representation trick.

The current canonical headers already provide the object, unit, actor,
player, collision, damage, material-effect, and animation types consumed by
the retained bodies. The one missing falling-damage tag-block element is
carried as a translation-unit-local typed structure with size and member
offset checks because this wave does not own `game_globals.h`. It is accessed
through `TAG_BLOCK_GET_ELEMENT`, never through address arithmetic. No public
ABI or storage owner changes.

The retained topology is natural:

- `unit_update` calls private `unit_cause_continuous_melee_damage` and the
  natural nonexact `unit_died` support;
- `unit_start_running_blindly` initializes the real state consumed by private
  `unit_running_blind`;
- the public impact, detach, death-start, flame-death, and unsuspecting bodies
  use their existing typed engine interfaces;
- rejected bodies have no definition in the final source;
- no dummy caller, linkage promotion, export, or synthetic reference forces
  code emission.

The rebuilt symbol table records `_unit_cause_continuous_melee_damage` and
`_unit_running_blind` as storage-class-3 private definitions. The other six
strict retained owners and `unit_died` are storage-class-2 external
definitions. `unit_damage_aftermath`, `unit_test_spawning`, and
`code_001a0cf0` are absent.

## One-shot discipline and strict retain/remove

The ten candidate bodies, support boundary, declarations, types, and caller
topology were frozen before source emission. A parse-only `/Zs` invocation
passed after adding only the minimum typed declarations and verified that the
pre-existing object file was unchanged. The normal generated target was then
compiled exactly once by Ninja for candidate adjudication:

```text
.\build\local-tools\ninja.exe -f build.ninja build\base\source\units\units.obj
```

That invocation executed one natural VC7 compile edge. The produced object
was copied immutably before comparison. All eight retained owners were strict
on that first object. The two rejected candidates were removed immediately;
the now-unneeded `code_001a0cf0` support was removed with them. No candidate
was tuned or recompiled. The later complete build graph mechanically rebuilt
the already-adjudicated retained source and reconfirmed the same eight strict
bodies; it was a validation rebuild, not a candidate search.

## Pre-commit validation

Only `source/units/units.c` and this new Jonas ledger are tracked changes.
No shared header, protected path, configuration, storage owner, semantic
exception, parked record, completion label, existing Markdown file, Claude
file, deletion, or unrelated source is changed.

- Complete `halobetacache_build` plus `libcmt_build`: pass, 569/569 actions.
  The retained `units.obj` rebuilt once at action 98. Only inherited warnings
  in scenery, `network_game_manager`, TIFF, and protected `ai_debug.c`
  appeared.
- Direct hardened comparison: all eight retained owners are strict across
  every padded byte and all 143 relocation identities.
- Direct target-owner census: 135/189 exact, 11 emitted nonexact, and 43
  absent. All 127 inherited exact owners remain strict.
- Ordinary `units.obj` report: 129/192 functions and 20,484/53,478 meaningful
  code bytes; four data bytes remain credited. The object remains
  nonmatching.
- Semantic audit: 470 units, 4,203 functions evaluated, 3,526 missing-base
  symbols, 4,062 semantic exact, 106 hidden exact / 62,568 hidden code bytes,
  36 ordinary-only, 35 structurally accepted, one rejected, 12 locals
  skipped, 4,123 accepted exact, and zero unit errors.
- Campaign progress: 375/833 objects, 4,105/11,060 functions,
  493,833/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 data bytes.
- Halo progress: 273/468 objects, 3,938/7,574 functions,
  480,919/1,770,166 code bytes, and 1,829,896/3,923,451 data bytes. Libcmt
  remains 102/212 objects, 167/476 functions, 12,914/55,015 code bytes, and
  5,192/8,637 data bytes.
- Object admission: zero candidates and zero revocations; only the inherited
  unrelated `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active entries, zero stale, zero invalid.
- Complete tooling suite: 179/179 tests pass. `git diff --check`, tracked-path
  scope, deleted-path, source-policy, protected-path, and storage checks are
  clean.

No committed-state replay is claimed in the implementation commit. After the
source and this ledger are committed by Jonas Volman and the worktree is
clean, a fresh one-unit accepted-state snapshot, verified same-root object
deletion, normal generated-path rebuild, immediate regression check, direct
strict comparison, complete owner census, and Ninja dry run will be appended
in a separate additive ledger-only commit. No push, amend, history rewrite,
or worktree removal is performed.
