# Units typed projectile-ray wave (Jonas, 2026-08-24)

## Result

This bounded wave retains exactly one newly strict function in
`source/units/units.c`:

- `unit_adjust_projectile_ray`.

The retained body is ordinary, readable, typed C. It removes the inactive-
union layout reuse present in the Claude exact carrier and instead uses a
scoped `real_point3d` camera position and a disjoint scoped `real_vector3d`
object velocity. The two values have non-overlapping lifetimes, so VC7 reuses
the same natural stack storage without a union or cast.

The candidate was exact on its sole natural full-translation-unit emission:

| body | meaningful | padded | relocations | normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_unit_adjust_projectile_ray` | 193 | 208 | 3 | `b9483057825a0c63739bf76296b17e8d0cf8f9a7435995762673ff54f0acc841` |

No header, configuration, protected source, storage definition, semantic
exception, parked entry, completion label, or pre-existing ledger was edited.
The object remains deliberately `NonMatching`.

## Fixed authority

- Exact cumulative base: `68a32ccf557677e89d0a092b28c68b845e8f7e86`.
- Base Units source blob: `75f4e3b36cadc7af3f13fbbdac90910684c75b10`,
  236,727 raw Git-payload bytes.
- Retained staged Units source blob:
  `482ffd13a13e2268b072dd5389773b6488be7ace`, 237,595 raw Git-payload
  bytes, payload SHA-256
  `4bff914c89623045267bbe8325c1d2461f958572f68938bae80609e5081b7c7a`.
- Immutable January `units.obj`: 138,090 bytes, SHA-256
  `e6a24ca597147dacff1f06806613d6bcfa4fa48f359dfbf542c34935f674b19f`.
- Exact cumulative base object copied before emission: 131,879 bytes,
  SHA-256
  `110ce9b496d00ad771a58092d103f9bc65617afa41d651faf275c4c3bf7c8ee2`.
- Immutable first-shot object: 132,255 bytes, SHA-256
  `60f9e5c6ed43dfa9e42db72da4cb47dbed3e1c8ed8cd9c6610449cc486c717e0`.
- Compiler: XDK 3911 `CL.Exe` 13.00.9254.1, 81,920 bytes,
  SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- January executable: 6,584,672 bytes, SHA-256
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.

The January function owns one 208-byte code COMDAT. Its three ordered REL32
relocations are exact in the retained object:

1. `+0x0F` to `_object_get_and_verify_type`;
2. `+0x44` to `_unit_get_camera_position`;
3. `+0x9A` to `_object_get_velocities`.

The leading-underscore name, six stack arguments, ordinary return, and current
typed helper contracts establish the public C ABI. The function owns no
literal, writable data, BSS, COMMON, or standalone runtime data credit.

## Claude and historical documentation audit

All reachable Units reconstruction and compiler-shape evidence was treated as
source evidence, not as a patch to import wholesale.

The first Codex reconstruction in `38a02c76` and `41ff836e` documented a
192-byte near miss and eleven exhausted ordinary expression/local variants.
That ledger precisely identified the missing January x87 duplicate-value
schedule and prohibited repeating those closed forms.

Claude commit `e80880d4` supplied the decisive natural source correction:
declare the projection before the temporary storage, materialize
`vector_from_points3d`, and spell the projection as the January-ordered scalar
sum `(i + k) + j`. It moved the board from 167 to 174 exact functions without
regression. The consolidated Claude carrier at `deb5554a` has source blob
`124f29e32497c091c4163b5faafc58dbf8890161` (305,068 raw bytes) and preserves
that exact body, but also contains unrelated policy-forbidden barriers, raw
layout accesses, type puns, attributes, and compiler-forcing constructs. It
was therefore never imported wholesale.

Clean HCEA commit `c168af2e747d3095d9a29418ae401f3a39544863`
independently supplies semantic donor blob
`a253d1f28cab0bd3e0ef5776d8abea623eb7df3a` (2,247 bytes). Its typed routine
uses separate camera-position and velocity values and corroborates the six-
argument behavior: optionally replace the direction with the unit aiming
vector, optionally project the origin from the camera, and always project the
unit velocity onto the final direction. January PC code remains the layout,
expression-order, and code-generation authority.

## Policy-clean union removal

The Claude exact body used a point/vector union solely to share a 12-byte stack
slot between two values whose lifetimes do not overlap. Reading alternating
members is outside this campaign's typed-source boundary even though VC7 emits
the desired bytes.

Before production emission, the exact Claude function was isolated with the
Claude `mk_repro.py`/`lab_compile.py` tooling. The unmodified repro first
reproduced January exactly at 208 bytes and zero disassembly differences. The
union was then replaced with:

- a block-local `real_point3d camera_position` used only by origin adjustment;
- a later block-local `real_vector3d object_velocity` used only by velocity
  projection.

The typed repro again produced 208 bytes and zero disassembly differences.
This was a controlled code-generation-neutral policy cleanup, not a source-
shape search on the production object. No point/vector cast, byte overlay,
raw offset, alias violation, inactive-union read, barrier, intrinsic, pragma,
attribute, `register`, `volatile`, assembly, synthetic anchor, or patched
object byte remains.

## Frozen one-shot boundary

The isolated branch and worktree started from the exact cumulative base. The
tracked candidate diff was one 40-line function in `source/units/units.c`.
`git diff --check` passed. A single VC7 `/Zs` parse-only invocation passed and
left the copied cumulative object unchanged at
`110ce9b496d00ad771a58092d103f9bc65617afa41d651faf275c4c3bf7c8ee2`.

Exactly one natural code-producing Units candidate command followed:

```text
ninja -f build.ninja build\base\source\units\units.obj
```

The direct hardened comparator reported equal padded bytes, normalized hash,
relocation count, positions, types, destination identities, and addends. The
body was retained without pruning, alternate spelling, statement reorder,
compiler-flag change, or retry.

## Preservation and full gates

The complete hardened target-code census moved only from 153 to 154 exact
functions out of 189. The 11 emitted nonexact functions and the other 24
absent target functions were unchanged; every inherited exact function was
preserved. A direct runtime-owner A/B of the copied cumulative object and the
candidate found all 207 non-code owner identities common and byte-identical;
only the compiler debug section `.debug$S|anonymous=0` changed. There was no
runtime owner addition, removal, or mutation.

The following pre-commit gates passed:

- the complete 568-edge Halo plus libcmt build;
- strict semantic audit: 470 units, 4,222 functions evaluated, 4,081 semantic
  exact, 110 hidden exact, 4,142 accepted exact, and zero unit errors;
- campaign progress: 4,120 accepted functions and 499,246 accepted code bytes;
- object admission: zero candidates and zero revocations, with only the
  inherited `source/shell/shell_xbox` completion-label contradiction;
- parked-function validation: three active, zero stale, zero invalid;
- the complete Python tooling suite: 179/179 tests.

The pre-commit report was first generated with already authenticated local
copies because the fresh worktree's ignored `build/tools` directory was
absent. During the later committed snapshot, Ninja correctly rejected those
copies as not recorded by this worktree's build log and attempted the pinned
download rules. The sandboxed attempt failed before changing the report or
object. The command was then rerun with approved network access and refreshed
exactly the configured `csplit` v0.0.2 and `objdiff-cli` v3.3.1 release
artifacts. No tracked tooling or configuration changed.

## Committed replay

The source and initial new ledger were committed together as
`4c9c2845b7325d79b797743c4b2a388b056b81a5` (`Recover typed Units
projectile ray`). It preserves these committed payload identities:

- `source/units/units.c`: blob
  `482ffd13a13e2268b072dd5389773b6488be7ace`, 237,595 bytes, payload
  SHA-256
  `4bff914c89623045267bbe8325c1d2461f958572f68938bae80609e5081b7c7a`;
- initial ledger: blob `4461a5e153f80a19962b732f95623ec82ec64f7c`,
  7,875 bytes, payload SHA-256
  `25dc4b1a82175cf0de9609f0b34e1a94826d74ba466d77c49ec72995d2c5055c`.

At that clean commit, the Units regression snapshot was written to the ignored
audit directory. The manifest is 5,800,991 bytes, SHA-256
`dc170bb9dfaa61e576caef7e4b2cd2f3775f4a35c5e68c79eb4711616cc17cca`,
and pins commit `4c9c2845b7325d79b797743c4b2a388b056b81a5`.

The generated object path was resolved to the current worktree, verified to
remain under that root, hashed, removed with literal-path semantics, and
confirmed absent. One ordinary production rebuild followed:

```text
[1/1] CL build\base\source\units\units.obj
```

The replay object is 132,255 bytes, SHA-256
`ec4611c269823bf720cdccff3296e43b97ff998e8a277d7a1abceb21e80f0f7d`.
The raw SHA differs from the immutable first-shot object only through normal
COFF emission metadata; the strict function, relocation, and runtime-owner
views are identical.

The committed regression check returned `ok: true`, 154 `still_exact`, zero
failures, zero warnings, zero `newly_exact`, and zero `changed_nonexact`.
Independent direct checks repeated 154 exact, 11 emitted nonexact, and 24
absent functions across all 189 target code owners. The projectile function
remained `all_equal: true`. All 207 first-shot non-code identities were present
in the replay with zero addition, removal, or acceptance-view change. A final
Ninja dry run reported no work.

This replay evidence is recorded by one additive ledger-only commit. The
implementation commit is not amended, rebased, or rewritten; no source changes
occur after the strict first-shot retain.
