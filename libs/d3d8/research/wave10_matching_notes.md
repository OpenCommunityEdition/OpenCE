# Continuation 010 — combiner reconstruction and original private identities

## Counts and distinct proof categories

The incoming checkpoint009 was freshly rebuilt in this continuation workspace:
350 sections /44,611 padded /42,253 NOP-excluded bytes. All14 stages passed.
Its original source, candidate objects, progress reports and replay log are frozen
under evidence/wave10_baseline. Cumulative checkpoints must never be added together.

Current source compiles27 C++ units and contains59 authored source/header/data files.
Thirty newly reconstructed combiner functions reproduce4,496 padded /4,292
NOP-excluded code bytes. No new SDK wrapper or recovered data is called an algorithm.

There are TWO explicit verification categories:

* **356 raw-symbol comparator matches /45,107 padded /42,703 NOP-excluded bytes.**
  This preserves all350 incoming matches and adds six combiner functions /496 bytes.
  The six comprise the public AlphaOutput function and five PDB-authenticated
  PRIVATE ENTRY-ONLY pairs. Entry pairing does not rename referenced relocations.
  Cumulatively,347 are same-name comparisons and nine are entry-only pairs.
* **24 ADDITIONAL combiner functions /4,000 padded /3,842 NOP-excluded bytes** pass
  the IDENTICAL comparator in an explicitly scoped, original-PDB-recovered symbol
  identity view, AND independently reproduce their original executable bytes after
  candidate relocation in memory. These are NOT raw-symbol comparator passes and
  must not silently enter a raw-symbol or canonical ledger as such.

Together these are380 distinct reproduced target sections /49,107 padded /
46,545 NOP-excluded bytes,62.30% of78,827 padded D3D8 code bytes. The target remains
29 objects and685 contributions.29,720 padded bytes remain unmatched. The expanded
census has67 nonexact entries and238 without a paired candidate symbol; some absent
pairs represent not-emitted helpers rather than absent source text.

`evidence/progress_combined.json` remains the unmodified RAW-gate census.
`evidence/progress_verified.json` is the separately labeled expanded census.
Neither implies whole-object, canonical, final-image or hardware completion.
Function contributions can include jump tables and padding. NOP-exclusion is not
automatically the canonical meaningful-byte definition. All350 baseline target
and candidate function fingerprints are preserved; no regression is waived.

## Source work

`combiner.cpp` contains32 complete functions: alpha/color input selection,
complement/expansion, select/modulate/add/subtract/blending/premodulate/dot-product,
multiply-add/lerp, output configuration and their constructor dispatchers.
Thirty reproduce; two are retained NONEXACT. Types, declarations, named input
selectors and shift modes are owned by combiner_internal.h. Existing code/source
outside this combiner family was not changed. No old TU options or build.py changed.

The related GPL-3.0-or-later RXDK source at pinned commit
29e018674a3443f9df5b5cb4478bb9c54d8a2a3a supplies semantic corroboration, not the target
ABI or byte proof. Its later premodulate implementation removed tests that the
January target still performs. Restoring those genuine conditions closes the
color premodulate body and exposes the alpha boundary case below.

The four-input shift table is a real16-byte writable table with values24,16,8,0.
It matches the original data separately; it receives ZERO code credit.
The source uses an authentic descriptive name, not `_data` plus an address.

Original null-texture fallback, first-stage diffuse versus later-stage current
inputs, specular-needed state, argument save/restore, temporary-result selection,
shift/bias limitations and signed/complement map values are preserved. Valid
callers supply texture stages0..3, combiner stages0..7, input selectors0..3,
supported texture operations and stable single-threaded device state.

## Why private-symbol comparison needed another proof

The supplied portable split labels24 private procedures `_code_<address>` and the
shift table `_data_<address>`. The original PDB has their authentic source names,
logical types, code sections/offsets and lengths. Putting address placeholders in
new source would violate house rules; treating every equal-size helper as an alias
would be unsound. Neither is done.

`verify_wave10_combiner.py` performs these independent checks:

1. Pins the original EXE hash and checks its NB10 signature/age against the PDB.
   Reads original PE section mappings rather than assuming a load base or that
   file offsets and virtual addresses are interchangeable.
2. Finds the unique original d3d8/combiner module. Authenticates all32 procedure
   records against PRE-REVIEWED names, return/argument types, calling convention,
   original local/global record kind, section, offset and procedure length.
   Candidate names are dictated by those reviewed signatures, not byte similarity.
3. Authenticates the shift table's unique S_LDATA32 record, unsigned-long[4] type,
  16-byte extent, source declaration, original location, contents and candidate
   contents. Four type-zero empty padding records provide no naming evidence and
   are explicitly not used for ownership. The unchanged strict parser is retained.
4. Builds an IN-MEMORY target metadata view restoring only those independently
   recovered private names. Original target/candidate files, instruction bytes,
   section flags/storage classes and the comparator remain untouched. The same
   comparator still requires full size, normalized bytes and ordered relocations.
5. Resolves candidate relocations independently at original PDB procedure/data
   addresses. The one public external, D3D__TextureState, is mapped using untouched
   original symbol metadata. I386 DIR32/REL32 addends and patch locations are
   applied in memory; unsupported, overlapping or unowned relocations fail closed.
   Every entire relocated function contribution must equal the ORIGINAL EXE
   bytes, not merely the reconstructed target. Every target split contribution
   also must round-trip to those original bytes.
6. Rejects deliberately changed instructions, sizes, relocation kinds/positions/
   addends, wrong actual callee bindings, wrong table addresses and table contents.
   A failed early test redirected an INLINED, unreferenced callee; the final test
   first asserts its chosen callee is actually referenced. Failed attempts remain
   preserved, not counted as successful negative controls.

This yields30 body proofs, six already in the raw gate and24 additional. The two
nonexact bodies are REQUIRED to fail both the restored-identity gate and literal
original-image comparison. They are not renamed into success. The original PDB
logical convention does not by itself prove an optimizer's private register ABI;
matching real call-site/callee machine bytes is required as well.

The24-body category is new and intentionally prominent in reports. This is a
proof of the function bytes under original symbol placement, NOT proof that this
incomplete source links at those addresses, that a nonexact callee works, or that
the complete original object layout has been recovered. No global alias policy or
semantic-match waiver was added. Canonical admission must review this evidence.

## Two nonexact functions and rejected shortcuts

**AlphaPremodulate:192 target /208 candidate bytes.** The target can access
`TextureState[stage+1]` even when a point-sprite remapping makes stage3 relevant.
The retained source explicitly checks `stage < D3DTSS_MAXSTAGES-1`; otherwise it
uses the existing select-argument fallback. We will not count an out-of-bounds
read as source-compliant merely because removing the guard matches instructions.
The unguarded historical trial is retained ONLY in experiments, not production.
This guard intentionally changes the original boundary behavior; it has zero
matching credit. The matched constructor caller does not certify this callee.

**ColorOutput:240 target /240 candidate,35 normalized differing positions.**
The remaining output-word/register-lifetime differences did not close with the
saved plausible operation-order and word-snapshot trials. The ordinary reference-
shaped source is retained and parked; no dummy work, volatile carrier, register
constraint, assembly or forced inlining was added.

Device Init investigation is retained as research, NOT a completed source body.
The earlier `add_device_init.py` draft is not a replayable/approved implementation
of the January target: the original has kernel/FS and cache-control details not
resolved by that draft. It was NOT applied to accepted source. Do not run it to
replace current source or count Init as complete. See the experiments note.

## House rules and provenance

Current published rules blob6bee6e0c6b67e3799527fbf96880e11bc59b6508 was checked.
New signatures put every parameter on its own line and return explicitly. Public
prototypes and enums stay in the associated combiner owner. Private functions
use authentic descriptive names. Correct enum constants appear in switches;
DWORD arithmetic retains unsigned shift semantics. No new inline assembly,
force-inline/no-inline attributes, optimizer pragmas, synthetic volatile locals,
raw CPU instruction emission, object patches or compiler-option changes appear.
Ordinary private helpers are called by the real constructor graph, not fake
emission anchors. Halo tag/object macros do not apply to SDK combiner resources.

The source-policy scan is only lexical. New-source review is not a formal proof
of all inherited UB, concurrency, ownership or historical authenticity. The
inherited BSF/SFENCE helper sites remain unchanged and disclosed. No original
executable, extracted target routine or GPU program was run. No bespoke runtime
or GPU tests are claimed. Preserve GPL/RXDK notices and license; this is not
clean-room or automatically compatible with a CC0 canonical source license.

## Rebuild/recovery and outstanding gates

The wave010 replay rebuilds all27 units, runs the unchanged raw comparator,
existing/five new entry-only pairs,44 comparator tests, earlier frozen-input and
baseline gates,1,829 raw-code/relocation mutations, three source patch recoveries,
and the new350-body baseline plus combiner PDB/original-image proof. Its additional
identity proof rejects396 mutations/incorrect bindings. These two test populations
are separately reported, not advertised as exhaustive behavioral coverage.

All59 current source files are bound to current candidate manifests together with
the compiler/frontend/backend, generated SDK overlay, options and build script.
Old clean-replay records are historical. The current final record belongs in
`evidence/wave10/clean_replay.json`; a missing or failed record does not mean success.
All working source and experiments are saved. Canonical/GitHub/Bink/Claude remain
untouched. Native splitting, complete data/BSS/COMDAT/extra-section ownership,
full inherited-source admission, final linking, GPU/concurrency and canonical
consumer regression remain open. No whole object is admitted.
