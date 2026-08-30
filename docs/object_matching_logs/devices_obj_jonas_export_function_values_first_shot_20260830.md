# `devices.obj`: exported function values first-shot boundary

## Result

This isolated strict lane starts from clean cumulative commit
`4a471d91d7591c32c5c8d6dc246c52afbb3edfa6` and fixes exactly one natural,
readable C89 candidate for `_device_export_function_values`.  The candidate
reproduced January's 512-byte padded extent and 21-relocation count, but not
its complete bytes, relocation addresses, or internal jump-table targets.  It
was therefore inverse-reverted wholesale without tuning.  Production
`source/devices/devices.c` is byte-for-byte restored, the function remains
unwritten, and `devices.obj` remains 14/30 strict functions with 1,008/4,688
padded strict bytes.

| Packet | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| January target | 512 | 21 | `7bf0963941ed36b13676536592a3af9d2786f5d4970b56f1129d24c3a35cc968` |
| Frozen candidate | 512 | 21 | `ea38c6208d4906f1ad9c5f17282de67e5005c8483cbc734307527529e3a4bf06` |

After relocation normalization, 388 of 512 bytes differ.  The target's
ordered relocation offsets are:

```text
013 022 055 06c 085 0be 0ea 0f9 11d 123 133 147 160 179 1b2
1e0 1e4 1e8 1ec 1f0 1f4
```

The candidate's offsets are:

```text
013 022 052 069 082 0bb 0e7 0f6 11a 120 130 148 15d 176 1b0
1e0 1e4 1e8 1ec 1f0 1f4
```

Both sequences carry the same external dependency population: checked object
access, tag access, zero/one floating constants, checked machine access,
`device_groups_data`, and `datum_get`, followed by six internal jump-table
entries.  However, the shifted addresses and internal destinations are part
of the strict COFF identity and fail closed.

## Authority and donor evidence

- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January `source/devices/devices.obj` SHA-256:
  `c7cb2a7ec7efa3a2999a68c25d0a52e83ae1d3aa409ece76bf4611469c7c0b26`.
- Pinned `xbox/bin/vc7/CL.Exe` SHA-256:
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- Authenticated HCEA commit:
  `570c83fd9c365dad6f2a3e7041705d5b84c7847c`.
- HCEA `src/blam/devices/device_export_function_values.c` blob:
  `aebd4bb8642155e893b7e551c61e420cffa659d5`; checked-out SHA-256
  `3fe747464dc7a9b347adc299800a50b2efeb8983fffabad4dacc92fa8a4eb386`.
- Current Stian corroboration commit:
  `fc11d861865735877d5b8b4c408b1a0c06dde3cf`, Devices source blob
  `c8600e3d10b4d2616f9f36e21c640e066f48f9dd`.

HCEA supplies the complete typed behavior: four mode slots; power, absolute
power-change, position, absolute position-change, locked state, and delay
cases; the machine/device-group flag tests; and preserving an output slot when
its mode is zero.  January independently fixes the public one-argument cdecl
ABI, PC field offsets, exact switch topology, x87 comparisons, jump table,
and relocation packet.  Stian's later-build raw lift corroborates the
pointer-walk/count loop but is not accepted as PC layout or byte authority.

The same-compiler canonical `vehicle_export_function_values` sibling was also
reviewed before freezing.  Its natural `real *value`, `short *mode`, and
full-width remaining-count loop is the direct source-topology evidence for the
one candidate.  No function-name occurrence exists in any prior Devices
ledger, and all-ref source history contains no earlier production definition
or code-producing attempt.

The complete Devices ledger set reviewed before emission was:

- `devices_obj_jonas_typed_leaf_wave.md`;
- `devices_obj_jonas_dispose_old_map_20260821.md`;
- `devices_obj_jonas_position_setters_wave_20260821.md`;
- `devices_obj_jonas_group_get_new_pair_20260821.md`;
- `devices_obj_jonas_initialize_new_map_first_shot_20260826.md`;
- `devices_obj_jonas_initialize_20260829.md`;
- `devices_obj_jonas_change_only_once_more_20260829.md`;
- `devices_obj_jonas_delete_20260829.md`;
- `devices_obj_jonas_group_actual_value_20260829.md`.

Current Stian/Pastudan `CLAUDE.md` and `AGENTS.md`, campaign methodology,
admission/parked policy, target inventory, and comparator/tooling docs were
reviewed as well.  No donor configuration, raw address, tool exception, or
completion label was imported.

## Frozen candidate and measured boundary

The pre-shot production source blob was
`a1f8cca13095337ec8bfb0967c9f451d7a3925b0`, with worktree SHA-256
`2f16f0495de09620f6d7d4d0227863a10b9248de6603cc3df4084617a6607ddb`.
The immutable candidate source blob was
`265613de4b610157b17c28704a62a49b8fa9ab43`, SHA-256
`c318649780260e445d5d073339bf5c7dc72d1f605c5a7fcf93300059de8adc61`.
Its sole production object has whole-file SHA-256
`7c618ba675f29e914d16b8da8a6e8cd8bcb0c90fe98de0886a13f4ca38130018`.

The candidate used only named canonical fields plus compile-time size/offset
assertions.  It declared the missing mode enum locally, used the typed
`device_get`, `machine_get`, definition accessor, and `datum_get` contracts,
and followed the HCEA behavior in a C89 pointer-walk loop.  It contained no
assembly, `volatile`, `register`, pragma, intrinsic, attribute, barrier, raw
address or raw-offset access, pointer/integer reconstruction, representation
pun, undefined behavior, inactive-union access, byte forcing, compiler
control, or tool exception.

The miss is a code-generation lifetime boundary, not a semantic or layout
ambiguity:

- January uses a 16-byte frame.  It keeps the definition in `EBX`, spills the
  mode and output pointers, and uses the remaining two slots for the loop
  count and shared result/conversion scratch.
- The natural typed candidate uses a 20-byte frame.  It keeps the mode pointer
  in `EBX`, spills the definition and output pointers, and allocates an
  additional delay-scale slot.
- The candidate's first two external calls match exactly, but its switch body
  begins three bytes earlier.  Later machine/group scheduling and the
  out-of-line delay arm diverge further before both jump tables settle at
  `+0x1E0`.
- January's delay arm uses the definition field directly through x87 and
  reuses the shared conversion scratch; the readable candidate materializes
  both `scale` and converted `raw` locals.

No declaration-order, local-lifetime, switch, delay-expression, pointer/index,
branch, or register experiment followed.  Reopen only with new authenticated
same-build source evidence that fixes January's four-slot allocation and
out-of-line delay expression before compilation; do not sweep or tune this
candidate.

## Restoration and validation

All function, enum, layout-assertion, and accessor-support additions were
removed by inverse patch.  The restored source again hashes to Git blob
`a1f8cca13095337ec8bfb0967c9f451d7a3925b0`; `git diff --exit-code` for
`source/devices/devices.c` returns zero.  Rebuilding the restored TU returns
the campaign board to 14/30 exact functions, 1,008/4,688 padded bytes, and no
definition of `_device_export_function_values`.

The full Halo/libcmt/report/semantic/progress gates reproduce the retained
Actors exact gain and the pre-lane totals:

- semantic audit: 470 units, 4,919 evaluated, 4,802 semantic exact,
  4,812 accepted exact, 131 hidden exact, and zero unit errors;
- progress: 384/833 objects, 4,773/11,060 functions,
  606,875/2,198,102 meaningful code bytes, and
  1,856,050/4,176,062 data bytes;
- Halo: 282/468 objects, 4,606/7,574 functions, and
  593,961/1,770,166 meaningful code bytes.

The same restored tree also passed every proportional campaign guard:

- object-admission audit: zero candidates, contradictions, and revocations;
- parked validation: 12 active, zero stale, zero invalid;
- protected Units `_unit_preprocess_node_orientations`: strict exact at 1,920
  padded bytes and 87 ordered relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`;
- tooling tests: 212 passed (the only warning was the sandbox denying pytest's
  optional cache directory);
- `build/report.json`: 1,599,057 bytes, SHA-256
  `B9ECFA128C6E67E4D36B1DC1041CD53558AFEE06338AC0F623A3F7CBBF2E8B22`;
- `build/semantic_report.json`: 3,267,795 bytes, SHA-256
  `E5BB3F3990965E7527B94E6A8A414DC23541D3627E32D4FBE057CD4A17E2EC53`;
- `git diff --check`: clean.

Only this additive boundary ledger is retained; no production source,
configuration, exception, park, completion label, protected path, or target
artifact changes.  Nothing is pushed.
