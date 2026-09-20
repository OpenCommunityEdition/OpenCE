# Independent review: one-wide player profile update

**Verdict: source and ownership are admissible, with one independently proven
relocation alias.** The ordinary symbolic gate remains a relocation-identity
residual. Count this function only if the unchanged existing accepted-ledger
path admits the integrated object; this review neither changes a comparator
nor supplies a new admission exception.

The reviewer applied `scratch/astra-context-onewide-authenticated.json` to
the frozen `git show 47d2e7b69f1c1277123e979296b6f0a7de00c80b` source and
compiled both old and candidate versions independently through `gate.py`,
using distinct scratch files and the point emitted-symbol guard. Production
source/configuration, Ninja, and commits were left to the root.

Reproduction and complete evidence:

- `scratch/astra-alias-onewide-review.py`;
- `scratch/astra-alias-onewide-review.json`;
- `scratch/astra-alias-onewide-{head,candidate}.{c,obj,gate.txt}`;
- `scratch/astra-alias-onewide-fake.json`.

The reviewed full-TU candidate source SHA-256 is
`660dc236190f6e045fcea0634471ecaff39363931f6aea3ee4d56f3e392d4e33`.

## Exactness and controls

`_player_profile_1wide_list_update` has **1,142 meaningful / 1,152 padded
bytes / 70 relocations** and normalized SHA-256
`c33a03c5370763cce87d6439786cd14c4e6297ae1e4069d83eb4b451b770bdd8`,
equal to January. All relocation addresses and types match. Sixty-nine
semantic identities match ordinarily; the only remaining symbolic difference
is at function offset `0x1A9`, type `IMAGE_REL_I386_DIR32`:

```
January: _local_player_index_for_draw_string_and_hack_in_icons + 0
ours:    _cached_player_profile + 156
```

Fresh January PDB extraction independently publishes the first symbol at
RVA **5,841,532** and the cache at **5,841,376**. Adding 156 reaches the
same destination. The January image operand at file offset **933,897** is
**10,035,836**; subtracting its image base `0x400000` gives **5,841,532**.
This is the compiler's end-of-array pointer comparison for the existing
three-entry cache, not a source read through the neighboring variable.

Using the existing `coff_compare.section_info_resolved` and fail-closed
`image_symbol_addresses`, **all 70 destinations** resolve unambiguously on
both sides and the whole resolved function compares exact. No unknown name
is guessed and no global duplicate name is silently selected. The same pair
already appears in accepted `_player_profile_update_cache_for_nwide_list`
and `_player_profile_3wide_list_update` controls; their independent PDB and
raw-operand evidence is in `scratch/astra-alias-census.json`.

All **45 other function sections are unchanged** from the frozen source,
including every one of the **40 ordinary strict exact controls** and the
three already accepted alias functions. The ordinary gate remains 40 exact /
6 residual because it does not erase the symbolic distinction.

## Full ownership review

The frozen and candidate objects have exactly the same **176 named runtime
owners**, with identical names, offsets, linkage/storage classes, types, and
section flags. There are no added/lost owners and no COMMON symbols. All
runtime noncode sections, matched by owner, retain identical contents and
semantic relocation records.

In particular, the previously noted wide-empty and CRLF literal owners are
unchanged from the current baseline. The 2026-09-15 historical literal-owner
warning does not describe a new owner introduced by this candidate. No new
helper or `_point_from_line3d` definition is emitted. The full candidate
source has zero fake-scan findings. Whole-object admission is not proposed;
other target functions remain residual.

## Source and provenance review

The retry is now a coherent tail self-call after invalid-profile filtering
and selection clamping. This restores the complete validation/top-level
lookup sequence on retry, whereas the inherited local loop skipped the
initial wrapper and definition validation. January's aligned jump-to-top
machine graph independently establishes that topology. Fresh unoptimized
cross-build code calls this function itself at **0x68674E -> 0x686170**.
The source remains an ordinary recursive retry, without a fake dependency or
manual jump target.

The two description strings are first retrieved into named pointer locals,
then formatted. Each local holds the actual helper result consumed by
`usnprintf`; neither is declaration filler or a synthetic lifetime anchor.
The target call order and full relocation proof corroborate the sequence.

The apparently redundant default-description terminator is independently
authenticated. In the first-party 2020/v140 unoptimized executable, the
successful default-description arm writes text[255] at **0x6865F7**, then
branches through **0x686611** to the common tail that writes the same element
again at **0x6866C1**. These are separate source-owned stores, not guessed
duplicate assignments. The valid allocation is 0x200 bytes = 256 Windows
wide characters, so both stores are within the allocated buffer; the final
element exists. Each description access remains guarded by allocation
success. The independent disassembly is preserved at
`scratch/astra-context-player_profile_1wide_list_update-2020.disasm.txt`.

The executable is a later cross-build whose exact source-revision identity
with January is unverified. It authenticates the recurring statement graph
when paired with source/assertion and control-flow correspondence; it is not
used as a January compiler-flag witness. January bytes and relocation
destinations remain authoritative.

The missing-profile empty state, text allocation sizes, default/nondefault
description paths, selected bitmap frame, and name termination are preserved.
No new out-of-bounds index, uninitialized read, volatile access, forced
inline/noinline directive, assembly, foreign prototype, or new type facade
is introduced. Existing short/word conversions and typed tag access are
retained. The function has an explicit `return;`.

## Landing boundary

The source-authenticity and ownership concerns are resolved by this fresh
packet; no user approval or policy exception is inferred. Root can integrate
the candidate, run the unchanged semantic progress/admission machinery, and
credit **+1,142 meaningful / +1,152 padded bytes / +1 function** only when the
resulting accepted ledger includes it. A failure of that unchanged machinery
would leave it zero-credit pending investigation; it would not justify
comparator weakening or target symbol rewriting.
