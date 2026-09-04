# `object_lights.obj` interrupted-Opus current-canonical re-audit (2026-09-04)

## Result

The interrupted Opus Object Lights packet is already reconciled in canonical
commit `3bfde7bd1595ddda3b207b7ec1eee48b2b6f9dd7`.  Re-auditing the live dirty
donor against current canonical finds no additional admissible source, code,
or data gain.  This packet therefore records a deliberate zero-delta closure:

- admitted padded code: **0 bytes**;
- admitted meaningful code: **0 bytes**;
- admitted functions: **0**;
- admitted data: **0 bytes**;
- strict regressions: **0**.

The current production source remains the best house-rule-compliant version.
It gates at **35 exact / 5 residual / 3 unwritten**, representing **7,104
padded** and **6,853 meaningful** strict-exact code bytes.  Object Lights owns
no credited runtime data.  The five coherent residuals remain validated parks;
the three unwritten target functions remain visible work rather than being
forced into the object with false linkage.

## Scope and immutable inputs

- Isolated base: `b175ed2be11d636bc9636d8c8cd17c7f3b9a3b9f`.
- Isolated branch/worktree:
  `agent/object-lights-dirty-reaudit-20260904` at
  `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\worktrees\object-lights-dirty-reaudit-20260904`.
- Read-only donor: `C:\halo-worktrees\opus-small-families-30k-20260902`,
  branch `opus/small-families-30k-20260902`, committed tip
  `eeedd72e5ba0fc1761519f68cad241fe606320e7`, plus its dirty
  `source/objects/object_lights.c`.
- Current source blob: `10cffc1beba6d7554c16b1b4c5c4bbf0fdef3e40`;
  stable file SHA-256
  `912735f9a8835264e7d23053e0fde793aafff60c74a2e3a8e81e9545efe4ae29`.
- Donor dirty source blob:
  `c2ca4a92f2848aba61296c0da2e9ce0f696f53d7`; stable file SHA-256
  `3e8a7119d3ffdc3627d2b6f6a3bd6ea08c93c4efd2439539321efc1f8556b9ba`.
- Preserved donor object:
  `scratch/w6_object_lights.obj`, SHA-256
  `dea72d667ade3a1c74d4b2c2dddae0f78d44435a6f5297bd1b503fac62a6bbeb`.
- Current split target SHA-256:
  `3e7fbead5c3f4c68bc9fbeadac0b23525830c9dc401012b44cdc0d0e90c3fb00`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1`;
  `CL.Exe` SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.

The campaign methodology, throughput lessons, acceleration playbook, campaign
tooling README, object-ledger rules, every tracked Object Lights ledger, and
the unfinished-Opus backlog were read before classification.  No applicable
`AGENTS.md` or `CLAUDE.md` exists in this exact canonical or donor checkout;
the separate lift-oriented `work/halo/CLAUDE.md` was consulted only as
background and does not override this repository's January exact-match rules.

## Donor replay and the three apparent extra matches

The raw dirty donor is not a current-canonical candidate.  Compiling it through
`gate.py --source` fails before code generation because it redeclares
`struct structure_surface`, which canonical now owns in
`structure_bsp_definitions.h`.  It also carries use-site prototypes, broad
header assumptions, old anonymous symbol names, an unformatted macro, raw
`datum_get` uses where a typed accessor exists, `unsigned short` where the
project `word` type exists, and stale local enum ownership.  Those are exactly
the header-position and source-ownership problems corrected by `3bfde7bd1`.

The preserved donor object remains useful read-only evidence.  Comparing its
target function sections under the current semantic aliases reports **38
exact / 4 residual / 1 unwritten** and 8,160 exact padded bytes.  Relative to
current canonical, its three apparent extra matches total 1,056 padded / 1,046
meaningful bytes:

| Function | Meaningful / padded | January storage | Donor storage | Disposition |
| --- | ---: | ---: | ---: | --- |
| `_find_point_lights_for_object_in_cluster` | `551 / 560` | private, class 3 | private, class 3 | not new; exact only in the donor's superseded declaration-position context |
| `_render_debug_light` | `271 / 272` | private, class 3 | external, class 2 | rejected wrong-linkage emission |
| `_light_get_bounding_sphere` | `224 / 224` | private, class 3 | external, class 2 | rejected wrong-linkage emission |

The first function's body is already canonical and is naturally retained by
its real dynamic-object caller.  Under the corrected narrow owner interfaces
it has January's 560-byte envelope and all 31 relocations, but an ordinary
scheduler residual (candidate hash
`3c814b365210b7757eb6ba5dee596c15ec8fefe476a335ad299af138e89876a2`)
instead of the target hash
`39d22fa1133941bdd6a0b52cfe5ff4e0df3e109e21e6318f46774740811f96be`.
The earlier reconciliation already performed the required transitive header
sweep and parked this exact declaration-position tradeoff.  The preserved
object supplies no new source provenance, so the park's reopen criterion is
not satisfied.

The latter two bodies are also already present in canonical with authenticated
private linkage.  VC7 correctly omits them while their real same-TU caller,
`lights_preprocess_scene`, remains unwritten.  Making them external reproduces
their instruction bytes but contradicts January storage class and exists only
to force standalone emission.  No dummy caller, header export, `noinline`
control, or wrong linkage is admitted.

## Canonical source retained over donor source

Current canonical already contains every admissible implemented body from the
dirty donor and improves its ownership and naming:

- `_shade_scalar`, `_shade_vector2d`, and `_should_render_lights` replace
  address-derived names, with January-corroborated private/public linkage;
- typed `light_get`, `light_definition_get`, lens-flare, shader-environment,
  object, and object-definition accessors replace repeated raw retrieval and
  casts;
- `word` and project `real` types replace ad-hoc primitive spellings;
- foreign declarations live in narrow owner interfaces rather than in
  `object_lights.c`;
- `structure_surface` has one real owner rather than a TU-local duplicate;
- object-definition flags use the established owner enum constants;
- `render_debug_light` and `light_get_bounding_sphere` retain their correct
  private linkage and honest source groundwork;
- the January `scale_vector3d` inline schedule remains intact and neither the
  current nor preserved donor object emits `point_from_line3d`.

No code-address or BSS-address names, volatile/register controls, pragmas,
barriers, forced-inline annotations, raw offsets, representation puns,
undefined behavior, artificial reachability, or nonsensical byte carriers are
present in the retained source.

## Existing parks and remaining work

No park is reopened because the dirty donor contains no evidence newer than
the already-audited packet:

| Function | Candidate / target padded | Relocations | Existing disposition |
| --- | ---: | ---: | --- |
| `_light_unmarked` | `80 / 96` | `8 / 8` | honest HCEA predicate; artificial exact control-flow respelling rejected |
| `_light_mark` | `96 / 96` | `8 / 8` | coherent marker helper; natural schedule residual |
| `_find_point_lights_for_object_in_cluster` | `560 / 560` | `31 / 31` | instruction-scheduling park |
| `_build_distant_lights` | `1008 / 1024` | `78 / 78` | x87 scheduling park |
| `_lights_prepare_for_object_static` | `832 / 832` | `17 / 17` | point/store and x87 scheduling park |

The only honest continuation frontier is the real caller packet:
`_lights_preprocess_scene` (2,400 padded bytes), private
`_render_debug_light` (272), and private `_light_get_bounding_sphere` (224).
The dirty donor and its generated Object Lights candidates contain no
`lights_preprocess_scene` body.  Reopen this packet only with a coherent caller
reconstruction grounded in January calls/relocations and independent source
provenance; then measure the three-function private-ABI cluster together.

## Validation

- Focused current-canonical gate: 35 exact / 5 residual / 3 unwritten.
- Board: 7,104 / 12,608 padded bytes and 35 / 43 strict functions.
- Ordinary report: 6,853 / 12,294 meaningful bytes and 35 / 43 functions;
  zero matched runtime data bytes in this unit.
- Raw dirty-donor compile: expected fail-closed duplicate
  `structure_surface` definition; no donor source was copied or edited.
- Preserved-object audit: 38 apparent exact / 4 residual / 1 unwritten;
  exactly the three non-admissible sections classified above.
- Full `ninja all_source progress semantic_progress`: pass; 473 semantic units,
  6,471 functions evaluated, 6,102 semantic exact, 6,120 accepted, zero unit
  errors, and 878,503 / 2,198,102 meaningful code bytes overall at the isolated
  base.
- Changed-source fake-match scan: zero review leads.
- Current and donor-object symbol scans: no `point_from_line3d` owner.
- Rename-stable before/after snapshots each contain 8,245 target functions and
  6,093 strict-exact sections; their comparison reports zero gains, zero padded
  bytes, and zero regressions.
- Park validation: 222 active, zero stale, and zero invalid.
- Admission audit: zero candidates, zero contradictions, zero revocations, and
  the two inherited explicit rejections.
- `units.obj` sentinel: `_unit_preprocess_node_orientations` remains exact at
  1,920 padded bytes.
- Tooling suite: 261 passed.

## Do not repeat

- Do not bulk-copy the dirty donor file or restore its local foreign
  prototypes and duplicate structures.
- Do not expose the two private helpers to claim 496 padded bytes.
- Do not trade the corrected narrow owner interfaces for the donor context
  merely to recover the 560-byte declaration-position match.
- Do not retry the operand-order or if/return spellings of `light_unmarked`
  without new authoritative source evidence.
- Do not count the preserved donor object as canonical progress; exact code
  bytes with wrong symbol storage or noncompliant declaration ownership are not
  admissions.
