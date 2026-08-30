# `actor_combat.obj` random-boolean exact boundary (2026-08-30)

## Outcome

The first and only production candidate makes `_random_boolean` strict exact
at 26 meaningful / 32 padded bytes with both ordered relocations exact.  All
11 inherited accepted Actor Combat function packets also remain direct strict
exact.

The candidate is not retained.  The include-compatible macro rename required
to define the header-owned inline identifier changes the fail-closed
whole-translation-unit evidence fingerprint for all 11 inherited functions,
both `.debug$F` sections, and the complete symbol-ownership set.  Under the
current full-section preservation rule, exact function packets alone are not
sufficient.  No adjudication or policy waiver was supplied.  Source and the
selected object were restored exactly, so this commit records evidence only
and claims no new accepted function or byte.

## Freshness and provenance

- Base commit: `6a6dcc41888e93ba0b2b77504b1e205feb0c570d`.
- Accepted/restored source blob:
  `c600965feadfbd2a5c290f1a9c5edbb71b4630bd`.
- Frozen candidate source blob:
  `00040825a85dc3b4842bdcfc1153704913f1cacb`.
- January split object SHA-256:
  `ed7cf731b085a4a3422cdc5e87ecafb1033fce0a705666d03a3b2f81618423c6`.
- Pre-shot object SHA-256:
  `9d101550615417e154679c057c70d069d4ecdf9e531d417a6c0a19ee5bcfde32`.
- Frozen first-shot object SHA-256:
  `62e0e61d7c7edf6cbefbb97e1a379cbe1d3e750cb1fc8c931e00437528bdd46c`.

All four current `actor_combat.obj` ledgers were read in full.  They recover
the six typed combat leaves, three x87 trigonometry wrappers, and two vector
math leaves, and explicitly freeze the unrelated `valid_real` punning family.
None names, attempts, or freezes `_random_boolean`.  Current history contains
only those four waves plus the initial symbol inventory, and a filesystem
census found no production Actor Combat definition of this function.

The authoritative body and ABI already exist in canonical
`source/math/real_math.h` as a byte-sized `boolean random_boolean(void)` inline
which returns `random() > 0x8000`.  The inline `random()` calls
`seed_random(get_global_random_seed_address())` using the owner header's exact
typed prototypes.  Independent HCEA `actor_start_burst` source corroborates
the same seed call and unsigned `> 0x8000` expression.  January target bytes
and relocations remain the acceptance oracle.

Target disassembly is the complete natural expansion:

```text
call get_global_random_seed_address
push eax
call seed_random
mov  cx, 0x8000
add  esp, 4
cmp  cx, ax
sbb  eax, eax
neg  eax
ret
```

The two zero-addend `REL32` relocations are at `+1` to
`_get_global_random_seed_address` and `+7` to `_seed_random`.  No private
callee, hidden register ABI, data owner, or tag/object layout is involved.

## Sole natural candidate

The current include graph already defines `random_boolean` inline.  Following
the object's established trigonometry/vector-wrapper pattern, the candidate
renamed only that inline identifier across `cseries.h`, restored the public
name immediately after the include block, and appended the target-order
definition immediately after `_add_vectors3d`:

```c
boolean random_boolean(
	void)
{
	return random() > 0x8000;
}
```

The body is the canonical header expression unchanged.  It is readable typed
C89 and contains no assembly, `volatile`, `register`, pragma, intrinsic,
attribute, optimizer barrier, raw address/offset access, pointer or union pun,
undefined behavior, byte forcing, synthetic anchor, object patch, compiler
exception, or comparator exception.

The frozen source passed `git diff --check`, the prohibited-construct scan,
and a no-output `/Zs` syntax check.  Exactly one normal XDK 3911 Ninja edge
emitted candidate code.  No declaration, macro, expression, predicate,
constant, source-order, flag, or compiler retry followed.

## Strict comparison

`_random_boolean` is exact at 32 padded bytes, two ordered relocations, and
normalized SHA-256
`3bd7359afa67fecf48892651f71ca0202b6f62c8e2c5bc6a85641440bba3df2a`.
Direct complete-COFF comparison also reports all 11 inherited packets exact:

- `_actor_combat_fire_wildly`;
- `_actor_combat_disable_bursts`;
- `_actor_firing_blindly`;
- `_actor_combat_currently_firing_burst`;
- `_actor_get_weapon_definition`;
- `_actor_combat_get_firing_variant_definition`;
- `_sine`, `_cosine`, and `_tangent`;
- `_cross_product2d`; and
- `_add_vectors3d`.

The pre/post whole-TU manifest nevertheless reports `_random_boolean` only as
`NEWLY_EXACT` and fails on changed accepted evidence for every inherited
function listed above, both `.debug$F` owners, and the symbol set.  The broad
fingerprint transition is caused by changing the inline identifier visible
while the TU's common headers are parsed; append placement alone cannot make
that representation invariant.

Do not repeat or tune the exact body.  Reopen only with a newly authenticated
declaration/include topology that defines the external owner without changing
the inherited compilation context, or as part of an explicitly reviewed
whole-TU ownership transition that re-proves every affected section.

## Restored-state gates

After inverse restoration, the whole-TU manifest reports all 11 inherited
functions `still_exact`, zero changed nonexact siblings, zero failures, and
zero warnings.  Complete Halo/libcmt/report/semantic/progress generation
passes.  Final validation reports:

- semantic audit: 470 units, 4,953 functions evaluated, 4,836 semantic exact,
  4,846 accepted exact, 131 hidden exact / 78,940 hidden code bytes, and zero
  unit errors;
- admission: zero candidates, zero contradictions, zero revocations;
- parks: 12 active, zero stale, zero invalid;
- tooling tests: 212/212 pass;
- protected Units `_unit_preprocess_node_orientations`: exact at 1,920 padded
  bytes, 87 relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`;
- `build/report.json` SHA-256
  `a5c84248c185f532cad281eacedf7d54cc741918cae4ff1a24791f313f0a10ea`;
  and
- `build/semantic_report.json` SHA-256
  `e94ecc24b1f5565187a91d1fc59b61521a4072878ca3c0fabc15e5285a411a63`.

Only this additive ledger is retained.  No production source, header, config,
tool, semantic exception, or parked record changes, and nothing is pushed.
