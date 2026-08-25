# Units vehicle-exit wrappers (Jonas, 2026-08-24)

## Result and scope

This bounded wave started from clean cumulative commit
`5086bb87c2d79dc4c17768c3ba2533b0e5c3dea0`. The frozen one-shot set was:

- `unit_scripting_enter_vehicle`;
- `unit_try_and_exit_seat`;
- `vehicle_scripting_unload`.

The sole natural three-body candidate compile retained the first and third
functions exactly. `unit_try_and_exit_seat` missed strict comparison and was
removed wholesale without a source-shape retry. The final tracked change is
therefore source-only two-body recovery plus this new evidence ledger. There
is no header, configuration, protected-source, writable-storage, semantic
exception, parked entry, completion label, or pre-existing-ledger edit.

## Evidence considered before emission

The January PC executable and split `units.obj` remain byte authority. The
target object is 138,090 bytes with SHA-256
`e6a24ca597147dacff1f06806613d6bcfa4fa48f359dfbf542c34935f674b19f`.
The cumulative baseline object was 132,255 bytes with SHA-256
`c2a13fd3486ed3551654dad47261e5c52c708a40005e9074bbb750f4f40518ba`.

All Claude Units material was treated as evidence rather than a wholesale
patch. This includes the consolidated `units_obj.md`, the seat-scripting and
vehicle-magic ledgers and recovery packet, the final carrier at commit
`deb5554ab8d16da80da934e2aa7081bc7d1f594f`, the full donor/caller census,
the compiler provenance and QFE survey, and the `c2dbg32` and campaign lab
tooling. The exact carrier source blob is
`124f29e32497c091c4163b5faafc58dbf8890161`; its three readable typed bodies
were transplanted without raw offsets, barriers, attributes, puns, or other
code-generation controls.

The clean HCEA semantic-donor tree is
`c168af2e747d3095d9a29418ae401f3a39544863`. The directly authenticated donor
blobs are:

| donor | Git blob | raw blob bytes |
| --- | --- | ---: |
| `src/unit_scripting_enter_vehicle.c` | `f67c79a15dab6602c1beac499e85b017c091b014` | 3,045 |
| `src/unit_try_and_exit_seat.c` | `a06a3ad50e41492790247ac8ce0b1e1458a5a226` | 5,014 |
| `src/vehicle_scripting_unload.c` | `a581c67fc9085d031ca6e8164b043f9bf5597703` | 7,569 |

The HCEA bodies are semantic and type corroboration only. January callers,
COFF relocations, the current `hs.c` evaluator declarations, and the current
public `unit_try_and_exit_seat` declaration establish ordinary cdecl
interfaces. Every retained dependency is an ordinary symbolic call. No
candidate owns `.data`, `.bss`, COMMON, or a new runtime read-only section.

## One-shot adjudication

The tracked diff contained exactly the three fixed bodies. `git diff --check`
and the policy/protected-path scan passed. A single real VC7 `/Zs` parse-only
invocation succeeded and left the copied baseline object byte-identical.
Exactly one natural code-producing `units.obj` candidate compile then ran.
The immutable first-shot object is 133,843 bytes with SHA-256
`aa5ce6a7987f4e5daf0d64aa734f59a444b2ad1398a37c69095058ec4c025b16`.

Strict results were independently adjudicated by padded bytes, normalized
relocation-masked hash, and complete relocation identity/addend sequence:

| function | result | meaningful / padded | relocs | normalized SHA-256 |
| --- | --- | ---: | ---: | --- |
| `_unit_scripting_enter_vehicle` | retain | 296 / 304 | 10 | `8d9c79ec7a14ff4e21b3f39e4a2e88b3f23bbe7918bf867956cb9eeda144b1ec` |
| `_unit_try_and_exit_seat` | remove | 288 candidate / 272 target padded | 12 / 12 | candidate `53ae0908f2b62c959c4cc9a9c57d8e552ae1a6f50af5406cf7f5a0585ffe1480`; target `6b6a6ef87ea90824a136314d30dafc9719b4cb879e26c5fb0b64fa4e07738c83` |
| `_vehicle_scripting_unload` | retain | 251 / 256 | 11 | `3793efad4ac79109a9a8dc95fd6ca3814d0d9d87ae70c1affc07c60959283844` |

The rejected body has the same relocation identities and count, but its
relocations after `_unit_open` are three bytes late and the section is one
alignment block larger. The cumulative header graph lacks the typed
short-return animation-permutation declaration present in the Claude carrier
topology. This is recorded as a future declaration/topology blocker, not
repaired after emission. The body was removed in full and no retry or tuning
occurred.

The normal retained-source build is 133,262 bytes with SHA-256
`203161794d1a6cd6a8b577db0172227ac8bf499e675f5309db77861edf0009fc`.
Both retained bodies remain strict and the rejected symbol is absent.

## Preservation and gates

The complete hardened function census advances from 154 to 156 exact of 189,
with the same 11 emitted nonexact bodies and 22 absent bodies. Every inherited
exact function is preserved. The direct target data census is 123 exact, zero
present-nonexact, and six absent of 129; the 1,564-byte `.data` block and the
four-byte `_unit_globals` BSS compare directly exact. Runtime-owner A/B shows
all 207 baseline non-code identities present and unchanged under the strict
cross-object acceptance view; only `.debug$S` changes. There is no runtime
owner addition, removal, or mutation.

Pre-commit verification passed:

- all 568 Halo plus libcmt build edges;
- semantic audit: 470 units, 4,224 functions evaluated, 4,083 semantic exact,
  110 hidden exact, 4,144 accepted exact, and zero unit errors;
- campaign progress: 375/833 objects, 4,122/11,060 functions, and 499,793
  accepted code bytes;
- object admission: zero candidates and zero revocations, with only the
  inherited `shell_xbox` completion-label contradiction;
- parked-function validation: three active, zero stale, zero invalid;
- the complete tooling suite: 179/179 tests.

The configured csplit v0.0.2 and objdiff-cli v3.3.1 binaries were reused from
the already authenticated typed-projectile worktree. Their SHA-256 values are
`708f957c37caa66df90ade35d1e65e2617e2cc2c8c2ccf68d8bf5cb20e752` and
`090987aa22c0fe9b7d252b2b44c2c0c92c5dd3e9b5965d353060802226a13677`.
The January executable SHA-256 remains
`4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.

## Committed replay

The source and initial ledger were committed together as
`c4439ca0c3da324f1fe25c95b56e760e05892d23` (`Recover Units vehicle exit
wrappers`). The committed source blob is
`45e28394c8073ab4eebea1892855d7665b29ca6a` (239,929 bytes), and the initial
ledger blob is `eeb9a9daace7b682876aab11c0cb92f361e946dc` (6,249 bytes).

At that clean commit, the Units regression snapshot was written to the
ignored audit directory. The 5,822,224-byte manifest has SHA-256
`69f7b3bfde165bd3c347ae9cdf8b08d76d395703e7c720ccaf6ca2f631fc072c`
and pins the exact implementation commit.

The generated object path was resolved and proven to stay under this
worktree, its pre-delete SHA-256 was recorded, it was removed with literal
path semantics, and absence was verified. One ordinary production rebuild
then ran exactly:

```text
[1/1] CL build\base\source\units\units.obj
```

The replay object is 133,262 bytes with SHA-256
`5549ed55262b957bc14358cb756c7e03ca5f6991c8f4f75149cf2f9d2be235b1`.
The committed regression check returned `ok: true`, exactly 156
`still_exact`, zero failures, zero warnings, zero `newly_exact`, and zero
`changed_nonexact`. Independent checks repeated both retained functions as
`all_equal`, kept `unit_try_and_exit_seat` absent, reproduced the complete
156/11/22 function census and 123/0/6 target-data census, and preserved the
runtime-owner inventory. A final Ninja dry run reported no work. The tracked
worktree was clean before this additive replay-only ledger update.
