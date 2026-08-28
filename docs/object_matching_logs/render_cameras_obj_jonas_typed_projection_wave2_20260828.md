# `render_cameras.obj` Jonas typed projection wave 2 — 2026-08-28

## Result

This bounded source-only wave starts from integration commit
`d9249547920c195306b55c40debfd09d0c616107`. It screens four previously
untried camera/projection bodies from independently readable HCEA and preserved
PC source. Two typed ordinary-C functions are strict exact and retained:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_render_camera_screen_to_view` | 256 | 20 | `e77a8e2a091747f035f6eaa403984bae2d352cf92d51a55c24db898a8340a24a` |
| `_render_camera_world_to_screen` | 208 | 18 | `b15f3ba30b323a7dd446fb5bfe55320e2224e0a48f67a5d3b681b4e01b1f7e91` |
| **Wave gain** | **464** | **38** | |

The inherited `_render_camera_new` remains strict exact at 32 padded bytes,
one relocation, and normalized SHA-256
`f7d7dc200fe60f3b60e3a674cf58838b6a3ad2caf443967a5c3f080e099b5b78`.
The object advances from 1/21 to 3/21 exact functions and from 32/10,256 to
496/10,256 padded code bytes. It remains `NonMatching`.

The final retained source Git blob is
`dee6ef1ec090c87e371fc27fcc6a73a69ea138e5`. The January split object SHA-256
is `c0194e912ef3b7e9ab6e4ffc9db57c527bd2fc9b011bc00e00c0ddc793cb6974`.

## Provenance and typed reconstruction

The complete existing render-camera ledger and reachable source history were
read before emission. Current and historical `CLAUDE.md` and `AGENTS.md`
contain no render-camera-specific instruction; their applicable generic C89
and source-shape guidance was followed. The three functions rejected in the
earlier leaf wave were excluded and never re-emitted:

- `_render_frustum_sphere_diameter_in_pixels`
- `_render_camera_triangle_frontfacing`
- `_render_frustum_triangle_visible`

The semantic oracle is HCEA commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`:

- `src/blam/render/render_camera_screen_to_view.c`, Git blob
  `c61f5256d465d99ce507e81d3a2889f8b6d637c6`, payload SHA-256
  `7125fe9280a7cebf5463455deea41f308e88839ce29ff24eef0e10f585b03c78`;
- `src/blam/render/render_camera_world_to_screen.c`, Git blob
  `5d15e57e602e6bb494cb7a17145064f692e9d855`, payload SHA-256
  `bfd01c9735bfd5a28e1da435577f2bf75c446d35e83fe5d486677df6e887611a`.

Preserved PC history independently supplies the January-compatible source
topology: screen-to-view commit `180ba2aee71255337c1d85e1e7941c127970fe02`
and blob `d06bfad71f1634c78a0c503ddcaa3d312ff7f7ae`; world-to-screen commit
`8e54e05e92d59926f9d34d12c9d405a1462dc686` and blob
`38fd1c3e7ad4b84de5a2f969cf4248c5185806da`.

`render_camera_screen_to_view` retains the five January debug assertions and
their source lines, computes the two projected components through named
matrix fields, and stores the `-1.0f` Z component in the preserved PC order.
`render_camera_world_to_screen` retains four typed assertions, transforms the
named world point through `frustum->world_to_view`, then returns the typed
`render_camera_view_to_screen` result. The missing callee declaration is
translation-unit-local and introduces no storage owner.

## Frozen rejection record

The first natural four-body candidate was preserved as
`render_cameras_wave2_first_candidate.obj`. Strict comparison rejected three
bodies and they were removed without spelling, declaration, control-flow, or
compiler-control tuning:

| Function/body | Candidate padded/relocs/hash | January padded/relocs/hash |
| --- | --- | --- |
| `_render_camera_hack_frustum_z` HCEA body | 304 / 18 / `4cdfe41d…` | 288 / 17 / `f9d9dd9b…` |
| `_render_frustum_get_projection_bounds` HCEA body | 208 / 14 / `4ff8c42f…` | 208 / 14 / `525c19ab…` |
| `_render_camera_screen_to_view` HCEA store order | 256 / 20 / `62b840cd…` | 256 / 20 / `e77a8e2a…` |

File-history review then exposed a separately preserved PC source body, not a
spelling permutation. It was emitted once. Its screen-to-view store order was
strict exact; its projection-bounds body remained non-exact at 208 bytes,
14 relocations, and normalized SHA-256 `017cb0c9…`, so projection bounds was
again removed. Reopen the two residual functions only with a new authentic
source topology or debug-scope evidence.

### Follow-up warning-condition donor

A separate bounded follow-up screened the independently preserved PC donor for
`_code_00174fc0`, later named `render_camera_check_warning_condition`. The
donor comes from
`research-cache/stian-halo-complete-history-20260828/src/halo/render/render_cameras.c`,
introducing commit `2de9e367583618a51c240592aba70afd0bb7be9d`, file blob
`6df6626845b1efa9c01d121d670229eba20d5765`, and payload SHA-256
`8b66b63bf0e8a7f8cd80356fc433da9261864569c83333f9e294eeef5a017992`.

The sole code-producing typed-C candidate is rejected and fully removed:

| Measurement | January | Candidate |
| --- | ---: | ---: |
| padded bytes | 160 | 176 |
| relocations | 13 | 13 |
| normalized SHA-256 | `7280c3e18a305d7cb7f5b2173139805adccdd9f64eac43d848bdea943d1ba663` | `fd6eff2cad87236914502058adece1afcda9805eac755e47c0b608fdc9d5c7a1` |

The first opcode proves an ABI/source-boundary mismatch: January begins
`mov si,ax`, carrying the value in the return register, while an ordinary
external typed-C definition must load it from `[ebp+8]`. Every later candidate
offset shifts and the section grows by one padded block. The donor also
materializes separate BSS owners `_render_camera_warnings_initialized` and
`_render_camera_warning_values` where January relocates into
`_bss_004b8b28+272` and `_bss_004b8b28+16`. The frozen candidate object SHA-256
is `a1990b582d549d8e53c6ae0aff0312d4782b69615593f9a29a85c8a72e1a585d`.
Reopen only with an authentic January caller/inlining topology and canonical
storage ownership; an external-body spelling retry cannot repair this ABI.

## Policy and validation

The retained source is readable typed C with one parameter per line and
explicit terminal returns. It uses named structure fields and existing math
interfaces. It contains no assembly, `volatile`, `register`, pragma,
intrinsic, optimizer barrier, raw address or byte-offset access, pointer
reconstruction, punning, undefined behavior, synthetic anchor, byte forcing,
comparator exception, configuration change, or alternate compiler flag.

The isolated retained-only gate reported:

```text
exact 3  residual 0  unwritten 18
```

Its frozen object
`build/audit/render_cameras_wave2_final_retained.obj` has SHA-256
`b9ee212c3bfb0af5cd879bf26c6f94e6b06d6b13f0a795972fcf8bdb799c74ff`.
After copying the identical source blob to cumulative integration, the normal
Ninja production edge rebuilt `build/base/source/render/render_cameras.obj`
successfully and `python tools/campaign/gate.py source/render/render_cameras
--all` independently reproduced the same 3/0/18 result. `git diff --check`
is clean.

Only `source/render/render_cameras.c` and this Jonas-owned ledger are tracked
changes. Units, Vehicles, Claude-active source, shared headers, storage
definitions, configuration, semantic policy, parked records, comparator code,
and object status labels are untouched.
