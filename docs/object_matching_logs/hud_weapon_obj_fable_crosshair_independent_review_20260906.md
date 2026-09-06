# HUD weapon scripted-crosshair independent review (2026-09-06)

## Verdict

**PASS** the owner/API packet and the unchanged natural setter.  The January
function, isolated first object, and actual canonical object contain the same
48-byte select-one code owner and satisfy the same state/ABI contract.  The
semantic global rename and `static` definition are source-credible and
owner-clean.  Static linkage remains an inference from complete use topology,
not a claim that original source linkage was recovered from January COFF.

No source spelling or body variant was tried.  This review changed no game
source, header, configuration, object, or build output.  It added only the
read-only audit/tooling files cited below.

## Global owner and linkage

`scratch/hud_weapon_crosshair_owner_census_20260906.py` verifies all frozen
hashes before parsing and scans all 833 January split objects.  There are nine
DIR32 relocations, all with addend zero, into `_bss_00453ac4`; every one is in
`source/interface/hud_weapon.obj`:

- allocator: one;
- new-map reset: two;
- scripted setter: one;
- `_code_000c7f40`: two;
- `_code_000c7fb0`: two; and
- `_code_000c8370`: one.

There is one split definition, also in HUD weapon: four-byte `.bss`, value
zero, type zero, external storage 2.  This storage record is a product of the
generic pretrial split and does **not** prove original public linkage.  The
exclusive same-TU data use, public setter boundary, adjacent static HUD owner,
allocator assertion literal `weapon_hud_globals`, and later semantic record
support the canonical source decision:

```c
static struct weapon_hud_globals *weapon_hud_globals = NULL;
```

The schema object emits exactly that four-byte `.bss` owner with static
storage 3, type zero, flags `0xC0300080`, and no relocation.  No COMMON owner
or second definition is introduced.  Both compiled candidate owners have no
raw initialized payload.  The January splitter materializes four zero bytes
for its synthetic BSS section, but its section is still explicitly
uninitialized-data, contains only zeros, and has zero relocations.

The config comparison finds exactly one changed entry: file offset `4537028`
changes from `_bss_00453ac4` to `_weapon_hud_globals`, with `static: true`.
The two 833-object split inventories have identical keys and hashes except
`build/split/source/interface/hud_weapon.obj`.  Thus the in-place metadata
change affects only its authentic owning object.

## Existing-code transition

The owner-only trial changes no instruction byte, size, relocation count/type,
or normalized hash in either incumbent function:

| owner | exact size / relocs | normalized SHA-256 | only relocation transition |
|---|---:|---|---|
| `_hud_initialize_weapon_interface` | `64 / 7` | `3ab5739809ece905f4e4a2078a33e5221a6a8690a7fd7d85f3c3d98c3a4eb01b` | `+23` raw undefined symbol -> defined semantic BSS |
| `_hud_initialize_weapon_interface_for_new_map` | `64 / 7` | `719b9e897833a78791c35b1255ce9abb95efb7ad97084955578c51fd8d24c111` | `+1,+39` raw undefined symbol -> defined semantic BSS |

The frozen four-consumer audit classifies 744 sections unchanged, three as
compiler-local-label spelling only, and these two as the expected definition
transitions.  Header ownership adds no function, helper, COMMON, or forbidden
point owner.  Schema-to-final manifests show only the HUD weapon base object
changed among 572 base objects, and zero of 833 target split objects changed;
the final C delta is only the natural setter body.

## Setter instruction and runtime contract

All three owners are 48 bytes, one DIR32 relocation at `+9`, normalized/raw
SHA-256
`f106592d3b4916e99cc1f27585794057797128e64f6382b7740801c223f2748f`,
select-one, external function storage 2/type 32, and code flags `0x60501020`:

| owner | full object SHA-256 | relocated global |
|---|---|---|
| frozen January target | `760258800d59f9e12750a64cbe63479c7b1ee5a7e577f920328ca46e4fedfbb2` | `_bss_00453ac4` |
| isolated first natural | `9759878cc358721d2cf350db34334da1e1fcbdec899a6355cd958273a8a2b43d` | `_weapon_hud_globals` |
| actual canonical | `75a9e213c4e14d19003b67dce594372f1267e90e544e2ce2ede8dd71240d298b` | `_weapon_hud_globals` |

The public runtime oracle executes the real section bytes after patching only
their genuine DIR32 pointer relocation.  It covers 13 full 32-bit stack-slot
patterns (including nonzero upper bytes with low byte zero) crossed with 12
script-flag seeds: 156 cases per owner, 468 total.  All pass with identical
behavior digest
`e99f58c3f094e5b7abbadfce2c7e9f1a7972ce23b0b945f824df10fbf462361f`.

The measured contract is:

- only the low byte of the stack argument is read; low-byte zero clears bit 0
  and every low-byte nonzero value sets bit 0;
- every other bit in the 32-bit `script_flags` word is preserved;
- every other byte in the seeded `0x1E4` globals object and the surrounding
  mapped guard page is unchanged;
- the pointer owner, function bytes, and caller stack (apart from the normal
  saved-EBP slot) remain unchanged; and
- EBP, EBX, ESI, EDI, and direction flag are preserved.  DF alternates
  deterministically between clear and set within the existing 156 cases.

Three in-memory negative controls are detected: reading the argument's second
byte (`96/156` cases), setting bit 1 instead of bit 0 (`56/156`), and clearing
bits 0 and 1 (`42/156`).  No mutated code is written to an object or proposed
as source.  The focused public test suite passes `8/8`.

## Durable artifacts

- Owner census script:
  `scratch/hud_weapon_crosshair_owner_census_20260906.py`, SHA-256
  `35B558A70DBCF7CCA1766317DC84BD5F87954B3DB6EA5D1E229513458B0FDE44`.
- Owner census result:
  `scratch/hud_weapon_crosshair_owner_census_20260906.json`, SHA-256
  `38EDDB4577168AE662CAE809383C6F26F6DAE834397ADCE6C7C5BEC663B3AC31`.
- Runtime tool: `tools/audit/hud_weapon_crosshair_runtime.py`, SHA-256
  `D7BE6FDC0D6B84D40AD5909989A8D9D744E27615BB4A3702BDC2C1AAC12A9DB7`.
- Runtime tests: `tools/test_hud_weapon_crosshair_runtime.py`, SHA-256
  `DA0AD972E7E0F79564B4FB3D90E6E61C345771E8588BF0055E60076E6A4622B5`.
- Three-owner replay:
  `scratch/hud-weapon-crosshair-runtime-20260906.json`, SHA-256
  `D276DFE752B595D1538A90D937A71A7DCB30B9A13E4819CA01CB5FEDB7CCFE5B`.
- Frozen phase manifests: before
  `5B00C0D375314DD23F4A6A00D6F902B488233F1093D4A8C8676758792B48E605`,
  schema `4D3EE9C7A3B196048B9B518E14CF0EAAA0890CE657DF5F78FCC9AEA6085607E5`,
  and final
  `45799B006EB59194051E7EE83C251F911DA874F3A36AB4E93D40C9A71ED0D731`.
