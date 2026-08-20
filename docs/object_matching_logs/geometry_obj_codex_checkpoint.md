# geometry.obj Codex checkpoint

## Scope and provenance

- Unit: `source/math/geometry.c` (`config.json` index 228).
- Baseline: local integration commit `29cb25c58e3dfc71c94dd7392582020bd48893ba`.
- Compiler: XDK 3911 VC7, `/O2 /Oy- /DDEBUG /Dxbox`.
- This checkpoint reconstructs only `geosphere_dispose`. The object remains
  `NonMatching`; no completion label or semantic exception was added.
- The local geometry branch census found no later retained implementation:
  `codex/geometry-consolidation` and `codex/geometry-leaves-2` have the same
  `geometry.c` blob as the baseline (`2a785bfe128bf2d343fe3d6dd37e3b1433c0639b`).

The implementation was reconstructed from independent evidence rather than
copied from another project:

- January target code proves three assertions at original source lines
  117-119, structure pointer members at offsets `+4` and `+8`, and three
  `debug_free` calls at lines 121-123.
- The HCEA corpus at local commit
  `c89106c4964f2df3a98ef7b4035d6750372797cd` independently identifies the
  20-byte `geosphere` layout and the field types used here.
- Aerocatia/demon commit
  `ef8cbebd2d122b8876c9ad4c14e5503afb8b7c7e` corroborates the lifecycle and
  field names. Its GPL source was used only as semantic/provenance evidence;
  the retained January/XDK implementation was derived and measured locally.

## Retained exact result

`_geosphere_dispose` is strict COFF exact:

| Evidence | Target | Candidate |
|---|---:|---:|
| Padded code bytes | 176 | 176 |
| Relocations | 18 | 18 |
| Normalized SHA-256 | `6b0637bddb65eb86f4cfe62570afd1ffdc97f2b32047e0357830c24f6f5b2a21` | same |

All relocation addresses, types, and resolved destinations match, including
three `display_assert` calls, three `system_exit` calls, three `debug_free`
calls, and the owned string references.

The four newly emitted target-owned string COMDATs are also individually
strict-exact with the same storage ownership and no relocations:

| String owner | Bytes | Normalized SHA-256 |
|---|---:|---|
| `c:\\halo\\SOURCE\\math\\geometry.c` | 31 | `9902ce6c2f89e7074183b977711ad9b927c2c21b478d0d48c6417bc35c191f57` |
| `sphere` | 7 | `d0ccc00760247969431f806b9272e52d33469482d6cf320749a500868c43baeb` |
| `sphere->vertices` | 17 | `24198c7e7221f64fcf63338a8cec0e3f9b85c7fda066698660ce07b1cf76aa13` |
| `sphere->triangle_strip_vertex_indices` | 38 | `e99f33428465d15032cf3949a71895b9570ab13f88ce9242dbb6df9736de379c` |

The function is intentionally placed after the existing reconstructed bodies.
Because code is function COMDAT-backed, this does not alter its exact bytes,
but it preserves the pre-existing constant-COMDAT and symbol-evidence order.

## Measured progress and regression evidence

- Geometry moves from 6/30 to 7/30 exact functions.
- Exact logical code moves from 303/12,751 to 466/12,751 bytes: +1 function
  and +163 bytes.
- All six baseline-exact functions remain strict-exact. No previously
  non-exact function changes after the source-order correction.
- The pre-edit fail-closed manifest reports only the intentional four new
  string COMDATs and corresponding symbol-set addition, plus
  `NEWLY_EXACT:_geosphere_dispose`; it reports all six prior functions as
  `still_exact` and `changed_nonexact: []`.
- A clean-commit post-gain snapshot/check freezes the enlarged ownership
  surface because the regression gate deliberately cannot waive ordinary data
  or symbol additions. After forced deletion and rebuild of `geometry.obj`,
  the replay passes with zero failures, zero warnings, zero changed non-exact
  functions, and all seven exact functions reported as `still_exact`.

## Repository gates

- Full `all_source` build: pass (569 candidate objects).
- Strict semantic audit: 470 units scanned, 3,668 functions evaluated,
  3,522 semantic exact, 93 hidden exact / 55,677 bytes, 3,604 accepted exact,
  zero unit errors.
- Admission audit: zero candidates, zero revocations; only the inherited
  `source/shell/shell_xbox` contradiction remains.
- Parked-function audit: 4 active, 0 stale, 0 invalid.
- Complete tooling suite: 179/179 tests passed.
- Object status remains `NonMatching`.
