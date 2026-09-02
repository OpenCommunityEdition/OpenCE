# `models.obj` Fable salvage audit (2026-09-02)

## Verdict

The unfinished Fable wave is admissible after semantic and ownership cleanup.
`source/models/models.obj` advances from 8/12 to **12/12 strict-exact
functions**, with no residual or unwritten function.  The address-based strict
gain is four functions and **3,808 target padded code bytes**.  The existing
16-byte no-op was renamed from an address placeholder but is not counted as a
new closure.

The isolated audit branch is `agent/fable-models-salvage-20260902`.  Its input
snapshot is commit `ad218b63d`, copied from the preserved Fable commit
`d48e09517`.  January's split object and XDK 3911 are the byte authority.

## Newly exact code

| Function | Target padded bytes |
|---|---:|
| `_render_model_parts` | 1,072 |
| `_model_get_node_matrices` | 320 |
| `_model_node_matrices_from_orientations` | 272 |
| `_render_model` | 2,144 |

Final one-TU gate:

```text
== exact 12  residual 0  unwritten 0  (of 12 listed)
```

`render_model_section` also reproduces the target's complete 1,528-byte
initialized-data section: one relocation and normalized SHA-256
`806638334fff1341360b4dec7db0987871f376bc3999e26ff4bcd3a1755d2955`.

## Names and independent evidence

- The private 1,072-byte renderer is named `render_model_parts` from its
  behavior and the independently recovered HaloCEA function of that name.
- The 16-byte public no-op is
  `model_geometry_part_build_tangent_matrices`.  HaloCEA contains the same
  named model helper as a no-op, so the old `code_00113710` identifier is not
  retained.
- The initialized profile owner is `render_model_section`, matching its
  `"render_model"` payload and the existing profile-section convention.
- HaloCEA's `src/data/default_function_values.c` and the three neighboring
  `default_render_model_*.c` records independently identify the four zeroed
  defaults used by `render_model`.  The January relocation sites establish
  which default belongs to each null-argument fallback.

The later Xbox build orders these defaults differently, so its addresses were
not copied.  Only its names, types, and use semantics are corroborating
evidence; the January target remains authoritative.

## BSS ownership boundary

January contains a 136-byte zero-filled block in this object, ordered as
function values (16), change colors (48), effect (40), and region permutation
indices (32).  The recovered variables use those semantic names and exact
types, are private to `models.c`, and total the same 136 bytes.

VC7's local-BSS allocator does not honor declaration order for these names.  A
bounded probe of all 24 declaration permutations produced the same candidate
order every time: region indices, effect, function values, change colors.
Making them external instead emits common symbols rather than the target BSS
owner.  Consequently this wave does **not** claim strict BSS-symbol-layout
credit.  No aggregate facade, renamed hash-steering variable, custom section,
or other source invention was added merely to force layout.  That is the
house-rule-safe boundary until stronger January source/name evidence exists.

## House-rule reconciliation

- Raw `tag_get` access was replaced by the existing typed
  `shader_definition_get` accessor.
- Model declarations now live in `source/models/models.h`.  Rasterizer model
  declarations moved out of `models.c` into their closest owner header,
  `source/rasterizer/rasterizer_models.h`; `render_sky.c` consumes the model
  owner header instead of carrying duplicate prototypes.
- The fabricated `sky_render_model_record` representation was removed.
  `render_sky` now uses the real `render_lighting` type and its
  `ambient_color` member while retaining its exact function bytes.
- Public/private names are semantic; no `code_<address>`, `data_<address>`, or
  `bss_<address>` identifier is retained for this object.
- Parameters use the campaign's multiline style, no-argument declarations use
  `void`, scalar math uses `real`, and recovered void functions end in an
  explicit `return;`.
- No forced inline/noinline annotation, volatile/register steering, fake call,
  fake condition, raw address overlay, pragma, or inline assembly was added.
  The scratch object emits no `point_from_line3d` owner or COMDAT.

## Verification

- `models.obj`: 12 exact, zero residual, zero unwritten.
- Header blast-radius gates: `render_sky.obj` remains 1/1 exact and
  `objects.obj` remains 121/121 exact.
- `fake_match_scan.py --fail-on-findings` on changed source: zero review leads.
- `git diff --check`: pass.
- Full-tree build, exact-set regression comparison, parked-function validation,
  admission audit, and pytest are required again after canonical integration.

## Do not repeat

- Do not use an invented aggregate or spelling chosen for compiler hash order
  to force the four BSS symbols into January's address order.
- Do not restore the fake sky record or duplicate model/rasterizer prototypes.
- Do not count the semantic rename of the already-exact 16-byte no-op as new
  code progress, or count the 136-byte BSS owner as strictly admitted data.
