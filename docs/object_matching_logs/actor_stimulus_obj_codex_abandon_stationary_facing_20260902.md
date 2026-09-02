# `actor_stimulus.obj` abandon-stationary-facing closure (2026-09-02)

## Result

This bounded source-only lane starts at canonical commit
`fede67af5bc92214dab5ff0f28651fd68736356f`. It reconstructs the previously
unwritten public function `_actor_stimulus_abandon_stationary_facing` in the
already advanced AI family. The retained ordinary C body is strict against
January, so `source/ai/actor_stimulus.obj` advances from 8/22 to 9/22 exact
functions, with zero residuals and 13 functions still unwritten.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_actor_stimulus_abandon_stationary_facing` | 199 | 208 | 11 | `c261d21edfdb46f8fc2261a2d51c441c296333e35a73ac9598731c7f8c276381` |

The hardened comparator proves identical padded bytes and identical ordered
relocation addresses, types, symbolic destinations, and addends. The object
therefore gains 199 meaningful bytes and 208 padded bytes. No completion label
is added because 13 target functions remain absent.

## Selection and authority

The lane deliberately chose an unwritten function rather than tuning a
residual. At baseline, `actor_stimulus.obj` was 8/22 exact with 14 public
functions unwritten, while its AI object family already had substantial typed
source and layout coverage. This 199-byte function was the smallest credible
closure among the HCEA-backed bodies not already rejected by an earlier object
ledger.

- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `actor_stimulus.obj` SHA-256:
  `cd7250bae74664672ed7c7cd20f3063b81b892472524e1d773884f9c2662a19b`.
- Baseline `source/ai/actor_stimulus.c` Git blob:
  `82a3e128abbbaa536bdc1134310acd383f609cda`.
- Retained source payload SHA-256 before commit:
  `b7d95644814ad2972b8d74287d9b86f1921d00628eb3667278c863f848c7caee`.
- Clean HCEA semantic donor commit:
  `570c83fd9c365dad6f2a3e7041705d5b84c7847c`.
- HCEA exact-name donor `src/blam/ai/actor_stimulus_abandon_stationary_facing.c`
  Git blob / payload SHA-256:
  `58b7d14ad90cdc5a75c82bb0f9ac20f69e15286a` /
  `8d990eb88fd545bd5d0216c20cf0dca144ae476dc4a9d0d526f760219f88b3a6`.
- Pre-prototype candidate object SHA-256:
  `ef73b550dad52e59ccd4dd533e9186c1137138a642cc3dda8bf84c256fd91139`.
- Initial clean-replay candidate object SHA-256:
  `9b366ae4ecb83da99996c2aa2679400daffcc2b5f1ab1652b9acf851a67b5084`.
- Post-prototype-audit candidate object SHA-256:
  `ee1166c1d509d6342b77acc20cc6cca4c7b333664b0e0615c9250c17c92f4b2e`.
- Final clean-replay candidate object SHA-256:
  `18b0f57143a45f7776bc3c2c6b98a75cfe2fe350b78d96573acf89c002e08aed`.

The production edge uses the repository's unchanged XDK 3911 Microsoft
32-bit C/C++ compiler and flags. The closest associated header, `actors.h`,
gains only the public function declaration described below. No compiler,
configuration, data owner, exception, completion label, or parked-function
record changes.

## Authenticated behavior and ABI

The exact-name HCEA donor authenticates the complete behavior: resolve the
actor and its character definition; if the actor is holding a defensive
stationary facing and the definition permits abandoning it, clear that state
and arm a timer in ticks; if the target prop exists and is closer than four
world units, raise perceived danger to at least 1.8.

The retained function uses the canonical typed accessors `actor_get`,
`actor_definition_get`, and `prop_get`. January fixes the lookup and field
topology:

- actor data lookup, then definition tag lookup;
- actor defensive-crouch flag at `+0x358` and timer at `+0x35A`;
- definition change-facing stand time at `+0x334`;
- timer conversion after multiplication by `TICKS_PER_SECOND`, through
  `__ftol2`;
- actor target prop at `+0x270`, loaded after the timer conversion;
- prop distance at `+0x11C`, compared with `4.0f`;
- actor perceived danger at `+0x354`, raised with the project `MAX` macro.

The target and retained COMDATs each contain 11 relocations. In order, their
symbolic lookup/call destinations and all relocation sites agree. There is no
private function, anonymous global, new storage, or raw-address dependency to
name. The candidate object defines no `point_from_line3d` COMDAT or symbol.

## Source-shape record

The first scratch-gate spelling followed the donor's earlier prop-index load
and expressed the danger raise with a local temporary. It emitted a natural
192-byte COMDAT against January's 208-byte COMDAT and was not retained.
January disassembly independently shows that the prop index is loaded only
after `__ftol2`; moving its declaration into that later semantic scope removes
an unnecessary live range. The project-owned `MAX` macro directly expresses
the authenticated "raise to at least" operation and reproduces January's
common-store x87 topology. Those two evidence-backed source corrections made
the next scratch gate strict. No spelling search, volatile/register device,
undefined behavior, or byte-forcing construct was used.

The retained source follows campaign house rules: one parameter per line,
explicit terminal `return;`, canonical `real` constants and tick macro, typed
tag/object access, same-line local initialization where practical, and no
manual bitwise replacement for an existing cseries macro. It contains no
assembly, inline request, pragma, intrinsic, raw byte offset, cast-based object
access, pointer/integer reconstruction, representation pun, inactive-union
read, synthetic anchor, or nonsensical code path.

`actor_stimulus.c` has no dedicated header. `source/ai/actors.h` is its closest
associated header and already contains a labeled `prototypes/ACTOR_STIMULUS.C`
section. The new public prototype is therefore declared there, not in an
unrelated caller or translation unit. The header has 41 direct C includers;
Ninja rebuilt 47 direct and indirect dependent objects after the declaration
was added. A complete per-function verdict comparison before and after that
rebuild reports zero gained functions and zero regressions. The declaration
does not introduce a type, structure, inline body, macro, or COMDAT and does
not perturb the January schedule of any dependent object.

## Scope and validation

Only `source/ai/actor_stimulus.c`, the matching public declaration in
`source/ai/actors.h`, and this ledger change. Full validation from the isolated
worktree proves:

- all 572 isolated candidate translation units build successfully;
- focused gate: `actor_stimulus.obj` 9 exact, zero residual, 13 unwritten;
- protected gate: `units.obj` 189 exact, zero residual, zero unwritten;
- complete before/after verdict sweep: exactly one `U -> E` transition and
  zero regressions;
- full pre/post-header verdict sweep: zero transitions and zero regressions
  across all 8,245 measured target functions;
- hardened function comparison: 208/208 padded bytes, 11/11 ordered
  relocations, zero normalized-byte differences;
- fake-match scan: zero review leads;
- no `point_from_line3d` symbol in the rebuilt candidate object;
- semantic audit: 473 units, 5,207 functions evaluated, 5,066 semantic exact,
  134 hidden exact / 80,066 hidden code bytes, 5,077 accepted exact, and zero
  unit errors;
- strict campaign progress: 384/833 linked objects, 5,034/11,060 functions,
  662,939/2,198,102 meaningful code bytes, and 1,870,127/4,176,062 data bytes;
- object-admission audit: one inherited ownership-audit candidate
  (`source/game/player_rumble`), zero contradictions, and zero revocations;
- parked-function validation: 90 active, zero stale, and zero invalid;
- complete tooling suite: 255/255 tests pass;
- `build/report.json` SHA-256:
  `884e9dfa7e07d022a886a4e126992ccb11779059a4f1d748cdd8c2d792013e0b`;
- `build/semantic_report.json` SHA-256:
  `f41a98daf3700fd256eed85b7164fd20777f712d71eaf586d7fe9ab49f8c1057`;
- `git diff --check`: clean.

At clean source-and-ledger commit
`b6de2604ee779d690a357f07f9a7fd46fe1378bb`, the hardened regression gate
wrote a focused manifest for `source/ai/actor_stimulus`. The candidate object
was then deleted and forced through the normal Ninja/VC7 production edge. The
committed-state check reports all nine accepted functions `still_exact`,
`changed_nonexact: []`, no newly exact surprise, no failure, and no warning.
The subsequent prototype-ownership repair triggered the full 47-object rebuild
and whole-tree verdict comparison recorded above. Whole-object raw hashes vary
across compiler invocations, so representative hashes are recorded above; the
hardened gates prove that accepted COMDAT bytes, relocations, non-code
ownership, and symbols are unchanged.

At clean source/header/ledger commit
`ecf44c9ca6c3f5e4b909c995aa330f2c6817dcd3`, a final regression manifest
captured both `actor_stimulus.obj` and the protected `units.obj`. Both
candidate objects were deleted and rebuilt through their normal production
edges. The final check reports all nine accepted `actor_stimulus` functions
and all 189 `units` functions `still_exact`, with empty `changed_nonexact` and
`newly_exact` sets and no failure or warning. This paragraph is a ledger-only
follow-up; no source, header, or build input changed after that replay.
