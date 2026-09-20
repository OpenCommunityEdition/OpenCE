# `_point_from_line3d` January ownership and inline-schedule evidence (2026-09-19)

## Scope and result

This was a scratch-only investigation of the shared January
`_point_from_line3d` schedule.  No production source, header, symbol, or park
configuration was changed.

The strongest result is negative but decisive: the January executable's one
selected body is consistent with normal VC7 `IMAGE_COMDAT_SELECT_ANY`
first-copy selection, while every tested legitimate VC7 spelling that lets a
caller inline the helper also emits a raw object-local COMDAT copy.  Therefore
the current requirement that *no later raw object may contain the COMDAT* is
not a source-level reconstruction of the observed January link.  Satisfying it
requires either artificial source suppression/manual expansion, both rejected
by the house rules, or a linker-aware accounting rule which models selection.

No coordinated production packet is proposed from this pass.

## Primary evidence

### January PDB link order and owner census

The census was run from the dedicated research tree:

```
C:\halo-worktrees\fable5-comdat-research-20260917\tools\campaign\comdat_census.py
C:\halo-worktrees\fable5-comdat-research-20260917\tools\campaign\pdb_link_order.py
```

against:

```
C:\Users\isabe\Downloads\Halo Combat Evolved (Jan 14, 2002 prototype)\cachebeta.pdb
```

The durable current-tree result is:

```
scratch/point_comdat_census_current.json
```

It proves:

- selected January `_point_from_line3d` file offset: 5,504;
- sole defining DBI module: `source/ai/action_charge`, module 470, true link
  rank 2;
- the selected-owner module has one external use, `_action_charge_perform`;
- there are no candidate emitters earlier than the owner;
- all reconstructed duplicate emitters are later in link order;
- 18 January modules record undefined references to the selected helper,
  including `source/items/projectiles`;
- the first-copy verdict is `FIRST-COPY-CONSISTENT`.

This corrects the earlier use of configuration order as link order.  In the
PDB order, `action_charge` precedes `items`, `projectiles`, `weapons`, and
`light_volumes`.

The pre-existing independent audit
`docs/object_matching_logs/shared_inline_helper_admission_audit_20260904.md`
also records the selected January helper as a 48-byte exact body with
normalized SHA-256
`9b763841f8519177ac2d3fd471226972b9fa92cf1e9061e9f92359077dc82741`.

### Same-compiler storage/linkage matrix

The fixtures are:

```
scratch/point_inline_fixture.c
scratch/point_inline_late_fixture.c
scratch/point_f1.obj ... scratch/point_f6.obj
scratch/point_f2_ob2.obj
scratch/point_f5_ob2.obj
scratch/point_late.obj
```

They were compiled with the repository's VC7/XDK compiler under the January
`/O2 /Oy- /Ob1` schedule, with `/Ob2` controls.  All of these spellings emitted
an out-of-line `_point_from_line3d` code section even when the call was fully
inlined:

1. ordinary `__inline`;
2. `static __inline`;
3. `extern __inline`;
4. an earlier external prototype followed by an `__inline` definition;
5. `static __forceinline`;
6. external `__forceinline`;
7. caller before the definition and another caller after it;
8. the `/Ob2` controls.

The force-inline variants do not solve emission and would be inadmissible in
any event.  No pragma, renamed duplicate, manual expansion, or forced
inline/noinline form was used or admitted.

### January uses the same normal pattern for `cross_product3d`

The separate result:

```
scratch/cross_product3d_comdat_census.json
```

shows the same first-copy topology for `_cross_product3d`: selected owner
`source/ai/action_obey`, true link rank 7, 44 later reconstructed emitters, and
13 January undefined-referrer modules.  The current header's selected
`action_obey` body is byte exact at 64 bytes with normalized SHA-256
`660da515fee7a623e6230a78b71749517ee5ad376db4126e5c8fe79414d2623a`.

This is an independent control demonstrating that the final executable keeps
one selected inline-helper copy while later object files can legitimately have
participated in COMDAT selection.

## Projectiles probe

The clean natural source probe replaced the four hand-expanded point
calculations in `_projectile_collision_test_line` with ordinary
`point_from_line3d` calls.  Its evidence is:

```
scratch/projectiles_original_point_inline.json
scratch/projectiles_original_point_inline.obj
```

Result:

- January function: 512 bytes;
- natural helper candidate: 528 bytes;
- natural helper candidate emits a 48-byte `_point_from_line3d` COMDAT;
- it therefore fails both strict bytes and the present raw-object emission
  guard.

A corpus scan of 207 preserved `projectiles.obj` candidates found no exact
copy.  The closest clean structural probe is 512 bytes / 10 relocations, but
the normalized bytes remain different.

The remaining projectiles mismatch is not only the point helper.  The January
body's inlined cross-product schedule corresponds to direct component stores,
whereas the canonical `cross_product3d` source uses three scalar temporaries.
A scratch header-wide direct-store trial produced the 512-byte projectiles
shape, but a full all-consumer rebuild caused **37 existing exact regressions**
for one 480-byte gain.  The experiment was fully restored; the restored stable
snapshot has zero delta from its baseline.  A global header rewrite is thus
rejected.

## Debug-database limits

- The January PDB supplies authoritative DBI module/link order and public
  symbol ownership.
- A DIA name query found no local/type/inline symbol record for
  `point_from_line3d`.
- The available IDA database/public-name exports and Ghidra disassembly add no
  source-level storage declaration or inline-site record.
- The HCEA source is useful as a semantic donor, but its standalone external
  implementation is from a different build and is not evidence for January's
  header topology.

## Inference, explicitly separated from the primary evidence

The smallest inference consistent with all observations is that January used
an ordinary header inline definition and LINK selected the first eligible
COMDAT (`action_charge`), discarding equivalent later copies.  The selected
executable and PDB cannot by themselves prove the exact header spelling, but
the VC7 matrix rules out the hoped-for source spelling that both inlines and
omits the raw later-owner section.

Consequently, the faithful next step is a reviewed **linker-aware COMDAT
admission/accounting change**, not a source hack: model true PDB link order,
require byte-identical `SELECT_ANY` bodies, credit only the selected owner, and
reject any earlier, non-identical, or non-`SELECT_ANY` copy.  Until the owner
explicitly changes the current raw-object no-surplus rule, natural helper-call
landings remain blocked and receive zero exact credit.

## Rejected forms

- per-TU external-declaration macros plus manual component expansion;
- private/renamed duplicate helpers;
- `__forceinline`, `__declspec(noinline)`, or pragma steering;
- a global direct-store rewrite of `cross_product3d` (37 regressions);
- treating final-executable absence of discarded copies as proof that the
  original raw object files did not emit them.
