# `motion_sensor.obj` Jonas census and tick one-shot boundary — 2026-08-29

## Outcome

This isolated lane starts from canonical commit
`a955ee0e7c5af927e4a5e96ef6835be70d43ec0e` and audits
`source/interface/motion_sensor.obj` without touching Units, Vehicles, Matrix
Math, AI Debug, HUD Weapon, or any other active owner lane.

The current source already owns four strict-exact functions. The only emitted
candidate in this lane, the HCEA-backed `_motion_sensor_tick`, was rejected on
its first compile. It matched January's 112-byte padded size and 12-relocation
count, and it used the same relocation-identity multiset, but its branch layout
changed the ordered relocation addresses and 38 normalized bytes. Candidate
source and supporting declarations were fully pruned without a retry. A normal
post-prune rebuild proves all four inherited functions strict exact again.

There is no retained production-source, header, configuration, object-status,
tool, or semantic-ledger change. This additive rejection ledger is the only
tracked result.

## Authority and exhaustive census

- January split object:
  `build/split/source/interface/motion_sensor.obj`, 14,882 raw bytes, SHA-256
  `6c628cd2b2382a10dd53d6317ab2d9a1c2c859b9a5ac70d00b171ea84e0564b8`.
- Canonical source/header Git blobs:
  `8cdea5b99832084b9c1c7506264e8beab079d814` and
  `3367dc0823f504d1a0b4907eb50692dc51e62370`.
- Compiler: repository Xbox VC7 edge, `/O2 /Oy- /DDEBUG /Dxbox`, without any
  flag, compiler, comparator, or tool exception.

Before emission, every applicable `AGENTS.md`, `CLAUDE.md`, campaign/tooling
document, and object ledger was read. The two pre-existing motion-sensor-named
ledgers apply to the distinct
`source/rasterizer/xbox/rasterizer_xbox_motion_sensor.obj`; no prior ledger or
bounded attempt exists for `source/interface/motion_sensor.obj`.

The registered-worktree census covered 1,511 worktrees. Their tracked
`motion_sensor.c` and `.h` material collapses to the canonical history plus an
older pre-map-initialization generation and line-ending materializations.
Complete reachable Git history contains no newer source body than
`c079fc90` for this translation unit.

The object-artifact census covered 2,136 objects at the ordinary base/split
locations and under `build/audit`. They collapse to exactly three signatures:

| Signature class | Copies | Result |
| --- | ---: | --- |
| January split target | 1,047 | all 19 target owners, not a generated donor |
| current generated base | 989 | the same four strict-exact owners |
| older generated base | 100 | three exact owners, before map initialization |

No branch, worktree, audit object, or source blob contains an unintegrated
strict-exact function.

## January fingerprints and inherited ownership

The target has 19 functions and 4,752 padded code bytes. The four inherited
exact functions total 208 padded bytes; the 15 residuals total 4,544 padded
bytes.

| Function | Padded bytes | Relocs | Normalized SHA-256 | Canonical status |
| --- | ---: | ---: | --- | --- |
| `_code_000ca160` | 176 | 16 | `01d1dc91ae477d072bf6e188e3247ff8dfd077a20da72c87d451d504d8aa4f0d` | residual |
| `_code_000ca210` | 80 | 4 | `ab99806431dbd239e3a2ea13e278aaa1504d7234dc45490fd9b16c7a44dbb406` | residual |
| `_code_000ca260` | 352 | 21 | `cb65a91e416950514c828a2fbed685df031a289140a3218812a754128d8ad64e` | residual |
| `_code_000ca3c0` | 96 | 4 | `c28dfa90e22bd48348034ae8cb472a5b6d0ebe45476d8ff47bcdaa672c060a5d` | residual |
| `_code_000ca420` | 16 | 1 | `836539c03b76e334a66e07771c3512e4cdc7fdc32710b9a7831e7eed7a3b6d5c` | residual |
| `_code_000ca430` | 64 | 5 | `e768bb9a6ef81b2c857a039a6678073540dee667d6a9f66d6aaf0f7da149f791` | residual |
| `_motion_sensor_initialize` | 80 | 8 | `b648637c038e52e53aea6d072bd88a06bee771c7d013208a8014c4fc9886d68b` | **exact** |
| `_motion_sensor_dispose` | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` | **exact** |
| `_motion_sensor_initialize_for_new_map` | 96 | 3 | `854473c3b0348b3553af7a2a56bb3831a6ffc358643c19e05a361594b49864ee` | **exact** |
| `_motion_sensor_dispose_from_old_map` | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` | **exact** |
| `_code_000ca540` | 32 | 1 | `fb2265354677a01c9fd7501dd001e7c8ff50ff8725eb91a568cf4d4c25244803` | residual |
| `_code_000ca560` | 112 | 10 | `02831c4d76978889bea0fbdb6c45941bc96272e613d3005cb0d7c7cdd73d0f5e` | residual |
| `_code_000ca5d0` | 224 | 7 | `13cd9e4469147c6d8d340c8f80647e653a8e0ef1bf957b9cfb7b09eef6360f66` | residual |
| `_code_000ca6b0` | 400 | 18 | `67347be9f029e06db7df599121540e3f2d416555d004f40e760cf1065dbbef09` | residual |
| `_code_000ca840` | 1,168 | 43 | `a8c62ddee9e7e5718949844c1e68489d08cc78c86da8f29f842b5d61f450b713` | residual |
| `_code_000cacd0` | 864 | 39 | `b37814ed8aaa3427a5d8e1fe1327e22cc80190383562143e94c35df03f5db06b` | residual |
| `_code_000cb030` | 768 | 40 | `954bdcc559c91d9e79948d1b28ce0a380e2b7cbe48030891ad5da5620dbf4cff` | residual |
| `_motion_sensor_draw_screen` | 80 | 6 | `ff4d0a47fed4d1c778f55de098d405ef04b86045a3e9af97ac224d79fa24a9f9` | residual |
| `_motion_sensor_tick` | 112 | 12 | `895a3fb08305ef485996f1577c5b94e52ee44f5951dae2d1546fac8be26c24ee` | residual |

## Donor topology and admissibility

HCEA commit `aa635a70b2c3285ade3f53c2f3f94de759ce7cd4` supplies readable typed semantic
bodies for the whole subsystem. Its `src/motion_sensor_tick.c` blob is
`aa75fcfbc9a99f6f5032e099253f94beb9dc2aeb`. The reorganized HCEA tree at
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` independently preserves the same
typed tick semantics in blob `054357711d81fb2af7694263de933f3402c6b913`.
HCEA is a later PPC/2-player build, so it is semantic and layout evidence
rather than x86 byte authority.

The local symbol atlas independently fixes the names and object ownership for
the January executable itself. In
`research/halo-symbol-atlas/symbols/halo_ce/4cc87b45...9520.jsonl`, the exact
tier assigns `_tiny_point2d_set`, `_blip_type_get`,
`_get_motion_sensor_data`, `_blip_begin`, `_should_draw_object`,
`_motion_sensor_update`, `_update_motion_sensor`, `_render_motion_sensor`,
`_motion_sensor_draw_screen`, and `_motion_sensor_tick` to
`motion_sensor.obj` at the addresses in this ledger. Independent atlas build
`b49631768...dc8f` additionally names `_render_blip` in the same sequence.

The later exact-PDB atlas `4d62cff2...be62.jsonl` supplies decorated type
evidence for `blip_type_get(long,long)`,
`motion_sensor_blip_set_type_and_size(long,long,motion_sensor_blip *)`,
`render_blip(motion_sensor_datum *,char,char,const real_point2d *,real,real,real)`,
`should_draw_object(long)`, `update_motion_sensor`, `render_motion_sensor`, and
`motion_sensor_tick`. It authenticates names and prototypes, not January
bytes.

Independent Stian/Pastudan PC `src/halo/interface/event_manager.c` history
contains the same source sequence at a constant `+0x10C80` address shift. The
same-build atlas proof above makes this a verified donor correspondence rather
than an address-order inference:

| January | Independent PC name / address | HCEA semantic name |
| --- | --- | --- |
| `_code_000ca160` | `tiny_point2d_set` / `0xDADE0` | `tiny_point2d_set` |
| `_code_000ca210` | `FUN_000dae90` / `0xDAE90` | `tiny_point2d_get` |
| `_code_000ca260` | `FUN_000daee0` / `0xDAEE0` | `blip_type_get` |
| `_code_000ca3c0` | `0xDB040` | `motion_sensor_blip_set_type_and_size` |
| `_code_000ca420` | `0xDB0A0` | `blip_size_get` |
| `_code_000ca430` | `FUN_000db0b0` / `0xDB0B0` | `get_motion_sensor_data` |
| `_code_000ca540` | `0xDB1C0` | `should_track_object` |
| `_code_000ca560` | `FUN_000db1e0` / `0xDB1E0` | `blip_begin` |
| `_code_000ca5d0` | `FUN_000db250` / `0xDB250` | `should_draw_object` |
| `_code_000ca6b0` | `render_blip` / `0xDB330` | `render_blip` |
| `_code_000ca840` | `motion_sensor_update` / `0xDB4C0` | `motion_sensor_update` |
| `_code_000cacd0` | `update_motion_sensor` / `0xDB950` | `update_motion_sensor` |
| `_code_000cb030` | `FUN_000dbcb0` / `0xDBCB0` | `render_motion_sensor` |
| `_motion_sensor_draw_screen` | `FUN_000dbfb0` / `0xDBFB0` | `motion_sensor_draw_screen` |
| `_motion_sensor_tick` | `FUN_000dc000` / `0xDC000` | `motion_sensor_tick` |

Relevant Pastudan provenance includes `edf8749b` (tiny set, explicitly
`ported:false`), `2beed9ef` (tiny get, Unicorn proof), `c6717782` (render-state
setup, Unicorn proof), `94409cee` (eligibility, Unicorn proof), `3c0a92b8`
(draw, Unicorn proof), `849f3e9f` (tick, Unicorn proof), `6146af8c` (map
initialization, 100% VC71 score), and `e0959598` (initialization, Unicorn
proof). Those sources are useful topology evidence, but their raw addresses,
raw offsets, register scaffolds, integer/pointer reconstructions, naked-code
history, and verification-only `x87_fmod` spelling are not admissible
production C.

## Frozen one-shot measurement

The single production candidate adapted HCEA's natural tick body. Supporting
typed aggregates modeled January's six contiguous initialized floats
`{ 1, 0, -0.75, 1, 1, 1.1 }` and its pointer-plus-sweep BSS pair without raw
offsets. The candidate used HCEA's natural condition:

```c
if (sweep_time >= 2.0374999f)
	sweep_theta = 0.40000001f;
else
	sweep_theta = 1.f / ((sweep_time + 0.0625f) * sweep_scale);
```

Exactly one code-producing edge ran:

```text
[1/1] CL build\base\source\interface\motion_sensor.obj
```

The ignored frozen candidate object is
`build/audit/motion_sensor_tick_hcea_one_shot/candidate.obj`, raw SHA-256
`2a87781db7f19f089a15832388190b4f5408b3372883042902ba1a23cb3594d5`.

| Artifact | Padded bytes | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| January `_motion_sensor_tick` | 112 | 12 | `895a3fb08305ef485996f1577c5b94e52ee44f5951dae2d1546fac8be26c24ee` |
| one-shot candidate | 112 | 12 | `6ce7b9849ac4e3cc4468f4f2ec75b2d505445835f00b8afbc7b77979457421c7` |

After relocation masking, 38 of 112 padded bytes differ. The relocation
identity multiset is equal, including two references each to
`_motion_sensor_globals+4` and `_code_000ca840`, but ordered relocation
addresses and destination sequence are not equal. January falls through into
the reciprocal expression for `phase < threshold` and branches to the 0.4
store; the HCEA spelling makes VC7 fall through into the 0.4 store and branches
to the reciprocal expression.

The typed runtime aggregate also changed only
`_motion_sensor_initialize`'s assertion-expression string from
`"motion_sensor_globals"` to `"motion_sensor_globals.state"`. Its code bytes,
size, and other relocation identities remained equal, but strict relocation
identity therefore correctly rejected that inherited owner during the
candidate compile. The other three inherited owners remained exact.

Pastudan commit `849f3e9f` independently records the PC control-flow form
`if (phase < threshold) reciprocal; else 0.4`, but it was discovered during
the parallel donor census after the frozen HCEA candidate was already
compiled. Adapting it after observing this miss would be a forbidden
source-shape retry, so no second candidate was emitted.

## Prune, replay, and fixed boundary

All candidate code, aggregates, declarations, includes, and header changes
were removed with a source patch. The restored `.c` and `.h` Git blobs are
byte-for-byte the canonical blobs listed above. A normal post-prune rebuild
ran one baseline edge and produced raw object SHA-256
`19a4c3b1d1bd3522ef94eeccd2917f9e3716309ea122e0430172916da69b2eea`;
path-bearing debug material makes raw object hashes worktree-local. Hardened
direct comparison again proves the same four inherited owners strict exact,
with no source or header diff.

Do not retry the HCEA tick spelling, invert its branch after this measurement,
or emit another standalone tick candidate without an explicit coordinator
reopen backed by stronger original-source/debug-scope evidence.

The remaining small helpers are not honest standalone leaves for production
compilation: January passes their arguments through private EAX/ECX/ESI/EDI
ABIs, and several out-of-line twins have no surviving call relocation.
Recover them only with an authenticated same-TU caller cluster that lets VC7
derive the private ABI naturally. The best bounded render-side cluster is
`_code_000ca430`, `_code_000ca560`, `_code_000ca6b0`, `_code_000cb030`, and
`_motion_sensor_draw_screen` (1,424 padded bytes), after the nested player,
sensor, HUD, and rasterizer types are independently authenticated. The large
update-side donors remain PPC semantic lifts or inadmissible raw-register/raw-
offset PC scaffolds and are not natural one-shot candidates yet.

The retained tracked diff is documentation only. It introduces no assembly,
`volatile`, `register`, pragma, intrinsic, attribute, optimizer barrier, raw
address/offset access, punning, undefined behavior, forced emission, byte
forcing, object patch, comparator exception, or alternate compiler/tool path.
