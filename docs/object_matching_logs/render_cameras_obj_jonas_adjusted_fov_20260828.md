# `render_cameras.obj` adjusted-FOV tangent recovery

## Result

`_render_camera_get_adjusted_field_of_view_tangent` is strict semantic-COFF
exact. The unit advances from 3/21 to 4/21 exact functions. Seventeen
functions remain unwritten, so the object remains `NonMatching`.

| Function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_render_camera_get_adjusted_field_of_view_tangent` | 24 / 32 | 2 | `185fa13e128f7ac8617c57f87be6d743265c2b5399c1505b1b18651737ffcbef` |

January's two relocations identify `0.5f` and `0.85f`. The existing typed
`tangent(real)` inline supplies the natural x87 operation. The retained
expression therefore states the complete recovered behavior directly:
half the field of view, take its tangent, then apply the 0.85 adjustment.
VC7 emits January's `fld`, `fmul`, `fptan`, `fstp st(0)`, and final `fmul`
sequence exactly. Direct hardened comparison reports `all_equal: true`, and
the whole-unit gate reports four exact with no residual functions.

January object SHA-256 is
`c0194e912ef3b7e9ab6e4ffc9db57c527bd2fc9b011bc00e00c0ddc793cb6974`;
the final rebuilt raw object SHA-256 is
`bc685571f7b553d13bd8834e5deda29245eae22842bdf1ca096a1b5369aacfe0`.

The combined wave passed the complete build graphs, fresh progress and
semantic reports, zero-candidate admission audit, 13/0/0 park validation,
strict board generation, `git diff --check`, and 205/205 tests. Campaign
totals after all three leaves are 573,826 overall code bytes, 560,912 Halo
code bytes, 4,610 semantically accepted functions, and zero unit errors.

The source is one ordinary typed expression. It introduces no assembly,
compiler control, attribute, pragma, qualifier trick, raw address, undefined
behavior, object patch, data claim, semantic exception, or comparator change.
`matrix_math.obj` and every user-protected source are untouched.
