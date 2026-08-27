# DirectSound attenuation closure rejection (2026-08-27)

## Outcome

The first and only ordinary code-producing candidate made both public
attenuation wrappers strict exact, but their sole same-TU callee
`_code_001b8c20` remained nonexact. Under the campaign's accepted-only and
link-closure rules, neither wrapper is independently admissible. The entire
three-function addition was removed and
`source/sound/sound_dsound_xbox.c` was restored byte-for-byte to its accepted
`f54ec267` source blob.

The result is zero retained functions, zero retained code or data bytes, no
configuration or completion-label change, and no regression to the already
exact `_dsound_angle_from_angle` and `_dsound_get` owners.

## Frozen evidence and provenance

- Base commit: `f54ec267f3c4476fcd6044159014181297328116`.
- Accepted/pre-wave source blob:
  `918e3b9b51f896607f53052e434cb99a906c92d8`.
- Frozen candidate source blob:
  `f4e2090f6c52867d1daaa7226ee5bef44f71b864`.
- January split object: 53,937 bytes, SHA-256
  `C7B04E9B80B2A397FB3C5562129C2C5B0681F483A659A05B6076D057FDEA7AAC`.
- Baseline candidate object: SHA-256
  `973D7FB8B821D07C7813DAAF35BD9F2169B082B7F732BF819BE078EAB340DE6F`.
- Preserved first-shot object: SHA-256
  `E7AB24001B79E39CE31756185E3233F10A3219A4ED843D8A5FBF67CFF89F555A`.
- Compiler: Microsoft XDK 3911 CL 13.00.9254.1 with the unchanged natural
  `/O2 /Oy- /DDEBUG /Dxbox` flags and include graph.

The complete Claude campaign documentation and every registered Claude
worktree were searched before emission. Campaign worktrees contained only the
January symbol skeleton. The separate readable Claude corpus at
`C:/halo-worktrees/claude-ai-conversation-2` reconstructs the semantic helper
as `sound_dsound_gain_to_volume`: validate gain in `[0,1]`, return `-10000`
for zero, otherwise compute `2000 * log10(gain) + ceiling` and clamp to
`[-10000, ceiling]`. Its source body and the independent Stian source at
commit `fc11d861865735877d5b8b4c408b1a0c06dde3cf` agree on that behavior.
The January strings, literals, calls, and relocation order independently
corroborate it. HCEA commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` has a later platform backend and
does not contain these original-Xbox DirectSound conversion owners.

All later sources are semantic and prototype evidence only. January PC padded
bytes and ordered relocation identity, address, type, addend, and ownership
remain the sole admission authority.

## Candidate packet

The frozen packet used ordinary C89-compatible typed source:

- external `long code_001b8c20(float gain, long ceiling)` with the exact
  target assertion text, path, line `0x23`, `display_assert`, and
  `system_exit(-1)`;
- literal `2000.0 * log10(gain) + ceiling`, avoiding the raw address present
  in the later reconstruction;
- `dsound_occlusion_from_occlusion` and
  `dsound_obstruction_from_obstruction` as direct
  `code_001b8c20(1.0f - value, 0)` wrappers.

Every parameter occupied its own line and every function ended in an explicit
return. No assembly, `register`, `volatile`, optimizer barrier, pragma,
intrinsic, attribute, raw address, pointer/integer reconstruction, pointer or
union pun, undefined behavior, synthetic anchor, object patch, comparator
exception, or byte-forcing construct was used.

## Strict first-shot result

| Function | Target meaningful / padded | Candidate padded | Relocations | Target / candidate normalized SHA-256 | Result |
| --- | ---: | ---: | ---: | --- | --- |
| `_code_001b8c20` | 129 / 144 | 128 | 9 / 9 | `e6515d86dbe247f05b5844d8512569eba700206378adb7543cc3f01fceae5dc1` / `43b626d6bdc621e44c0304fa9c32f8b5ee1868120dd90d77c62bfa0df2689a9c` | rejected |
| `_dsound_occlusion_from_occlusion` | 28 / 32 | 32 | 2 / 2 | `42423371b8b73b2bca33b69bb5857cf4b9ee10188d746a55b12dc0cda2b0c253` / identical | exact, not link-closed |
| `_dsound_obstruction_from_obstruction` | 28 / 32 | 32 | 2 / 2 | `42423371b8b73b2bca33b69bb5857cf4b9ee10188d746a55b12dc0cda2b0c253` / identical | exact, not link-closed |

The helper preserves all nine destination identities and addends, including
the two float constants, both assertion strings, `display_assert`,
`system_exit`, the `2000.0` double, and `__ftol2`. Its relocation addresses
diverge, however. January pushes ESI at `+0x06`, caches `ceiling` in ESI at
`+0x0D`, and uses the positive range-test topology. The candidate omits that
nonvolatile lifetime, reloads `ceiling` into ECX near the tail, and emits a
different but semantically equivalent floating-predicate branch topology.
The mismatch is broad compiler topology, not a missing literal or dependency.

Each exact wrapper has a zero-addend DIR32 relocation at `+0x05` to
`__real@3f800000` and a zero-addend REL32 at `+0x13` to
`_code_001b8c20`. Because that callee is source-owned by the replacement TU
and is nonexact, the wrappers cannot be retained as an accepted-only closure.

## One-shot discipline and reopen condition

`git diff --check` and a no-output production `/Zs` check passed before
emission. Exactly one normal selected-object compiler edge produced the
candidate. No declaration, macro, condition, lifetime, local-variable,
clamping, flag, or compiler retry followed the result.

Do not repeat this explicit later-source body. Reopen only with genuinely new
original-era source/PDB evidence or an independently exact donor that proves
the original positive-assert and clamp macro topology strongly enough to
freeze a new wave. A future candidate must make `_code_001b8c20` itself exact
before either already-proven wrapper can be retained.

The accepted source is restored to blob
`918e3b9b51f896607f53052e434cb99a906c92d8`. No push, amend, history rewrite,
or protected-source edit is performed.
