# ParticleSystems initializer + unattached owner-schema intake (2026-09-06)

## Decision

The minimal shared-owner schema is source-authentic and suitable for a first
natural compile.  It consists of two complete-size tag records with reserved
unknown spans, the touched fields of the already complete-size runtime records,
and the existing bit-one initializer flag.  The public unattached constructor
already has its correct owner prototype.  Its only new include dependency is the
real `objects/object_lights.h` owner of `light_particle`.

This is a **schema/body GO but only a partial-graph admission**.  The two bodies
close their direct private call (`particle_system_new_unattached` -> static
`particle_system_initialize`), but the initializer then calls the still-unwritten
same-object `particle_system_update` target at January `0x48e7f0` (currently
spelled `code_0008e7f0`).  Do not export or otherwise manufacture a private ABI
to hide that dependency, and do not claim a linkable/complete ParticleSystems
object until that real private body is recovered.  The natural inline
`real_local_random_range` use can also emit an uncredited select-any COMDAT; it
must be audited as an owner/link boundary rather than suppressed or counted as a
new exact function.

At the time of this review, root's in-progress trial diff in the three Particle
Systems files matches the schema/body proposal below.  This note records evidence
and scope only; it does not claim the in-progress build result.

## Exact minimal owner proposal

In `source/effects/particle_system_definitions.h`, retain the complete existing
`particle_system_definition` and add these complete-size owners.  January itself
authenticates `old_particle_system_type`; the state-record tag is not present in
any January decorated symbol, so use the independently PDB-authenticated
`particle_system_type_state` spelling rather than inventing
`old_particle_system_type_state`.

```c
struct particle_system_type_state
{
	byte reserved00[0x20];
	real duration_lower_bound;
	real duration_upper_bound;
	byte reserved28[0x98];
};

struct old_particle_system_type
{
	byte reserved00[0x68];
	struct tag_block type_states;
	byte reserved74[0xC];
};

typedef char old_particle_system_type_size_assert[
	sizeof(struct old_particle_system_type) == 0x80 ? 1 : -1];
typedef char old_particle_system_type_type_states_offset_assert[
	offsetof(struct old_particle_system_type, type_states) == 0x68 ? 1 : -1];
typedef char particle_system_type_state_size_assert[
	sizeof(struct particle_system_type_state) == 0xC0 ? 1 : -1];
typedef char particle_system_type_state_duration_lower_bound_offset_assert[
	offsetof(struct particle_system_type_state, duration_lower_bound) == 0x20 ? 1 : -1];
typedef char particle_system_type_state_duration_upper_bound_offset_assert[
	offsetof(struct particle_system_type_state, duration_upper_bound) == 0x24 ? 1 : -1];
```

In `source/effects/particle_systems.h`, expose only fields touched by these
bodies (plus fields already proven and used by the published lifecycle packet):

```c
enum
{
	_particle_system_active_bit,
	_particle_system_initializing_bit,
};

struct particle_type
{
	short state_index;                  /* +0x00 */
	short transition_state_index;       /* +0x02 */
	real time_left_in_state;             /* +0x04 */
	real state_length;                   /* +0x08 */
	byte reserved0C[0x2C];              /* untouched +0x0C..+0x37 */
	boolean states_moving_forward;       /* +0x38 */
	byte reserved39;                    /* untouched +0x39 */
	short particle_count;                /* +0x3A */
	long first_particle_index;           /* +0x3C */
};                                    /* 0x40 */
```

The complete `particle_system_datum` should expose `scale +0x14`,
`color +0x38`, and `lighting +0x48`; leave the unused four-byte physics-state
slot reserved instead of importing the later union:

```c
	/* existing prefix through attachment_index +0x10 */
	short type_state_index;              /* existing, untouched +0x12 */
	real scale;                           /* +0x14 */
	struct location location;            /* +0x18 */
	real_point3d position;                /* +0x20 */
	real_vector3d velocity;               /* +0x2C */
	real_argb_color color;                /* +0x38 */
	real_rgb_color lighting;              /* +0x48 */
	byte reserved54[0x4];                /* later PDB physics state; unused here */
	struct particle_type types[4];        /* +0x58; datum remains 0x158 */
```

Add the corresponding size/offset assertions for every exposed field.  Do not
import the later interpolated-variable, physics-state, particle-state, or shader
records merely to replace either reserved span.

## Natural source/API plan

`source/effects/particle_systems.c` needs:

- `#include "objects/object_lights.h"`; no local `light_particle` declaration;
- a TU-private `static boolean particle_system_initialize(long system_index)`
  declaration and definition;
- the already-owned public
  `long particle_system_new_unattached(long, real_point3d const *,
  real_vector3d const *, real_argb_color const *, real)` definition; and
- the config rename at file offset 587328 from `_code_0008f640` to
  `_particle_system_initialize` with `"static": true`, co-located with the
  body.  HCEA's PDB explicitly records this helper as
  `static unsigned char particle_system_initialize(long)`; canonical
  `boolean` is the matching byte return type.

The natural body semantics are fixed by January:

1. get the system datum and `pctl` definition;
2. resolve `system->location` from `system->position`, then set flag bit 1;
3. iterate `definition->types.count` with a `short` index;
4. retrieve each 0x80-byte `old_particle_system_type`, and address the
   corresponding 0x40-byte runtime `particle_type` at system offset +0x58;
5. for nonzero state count set state 0, transition `NONE`, forward `TRUE`,
   particle count 0, and first handle `NONE`;
6. only for positive state count retrieve state 0 with element size 0xC0,
   sample its two duration floats with `real_local_random_range`, and store the
   same result to runtime +0x04 and +0x08;
7. a zero state count sets `success = FALSE` but does not stop later type
   initialization; and
8. on success, invoke the existing update dependency with `(0.001f,
   system_index)`, then return the byte result explicitly.

The public constructor's January order is likewise fixed: allocate; if valid,
set definition and detached-object `NONE`, copy position, velocity, and all four
color floats, store the by-value 32-bit `scale`, set active bit 0, call
`light_particle(&system->position, &system->lighting, &diffuse, FALSE)`, call
the private initializer, and on failure delete the just-allocated datum and
return `NONE`.  There is no January render-enable precheck; that is later-HCEA
behavior and must not be imported.

The first natural declaration chronology should remain ordinary C89.  In
particular, the HCEA-authentic loop body declares the tag `type_definition`
before the runtime `type`.  Fable's reported strict initializer was a second
compile obtained by reversing those declarations to steer ESI; do not replay
that order as first-natural exactness evidence.

## January byte/layout proof

Frozen target:

`scratch/particle-systems-natural-init-before-20260906/files/build/split/source/effects/particle_systems.obj`

SHA-256:
`25cc3edef1c9b54ba12f677a88a0b5b93aff8f66fcdff7b29d8faa532b939e56`.
The containing before manifest is
`scratch/particle-systems-natural-init-before-20260906.json`, SHA-256
`78a20ee5df5cdcf11d1966f5e165a749936d4d2c135e21bf47c194d2fbacc7c1`.

Reproducer:

```powershell
python tools/audit/dump_coff_disasm.py `
  scratch/particle-systems-natural-init-before-20260906/files/build/split/source/effects/particle_systems.obj `
  _code_0008f640
python tools/audit/dump_coff_disasm.py `
  scratch/particle-systems-natural-init-before-20260906/files/build/split/source/effects/particle_systems.obj `
  _particle_system_new_unattached
```

Initializer instruction proof:

- `+0x3B/+0x3E/+0x44`: read flags +4, OR 2, store flags +4;
- `+0x41`: system definition +0x5C; `+0x47`: `types.count`;
- `+0x60` pushes 0x80 to `tag_block_get_element_with_size`;
- `+0x68` shifts the index by six and `+0x6C` addresses system +0x58;
- `+0x75/+0x78`: state block count/address at tag type +0x68;
- `+0x85`, `+0x8A`, `+0x8E`, `+0x92`, `+0x98`: runtime writes at
  +0/+2/+0x38/+0x3A/+0x3C;
- `+0xA0` pushes 0xC0 for state 0; `+0xAD/+0xB0` load upper/lower
  floats at state +0x24/+0x20;
- `+0xC3/+0xC9` call the local-seed and real-range owners;
- `+0xCD/+0xD0` store one x87 result at runtime +4 and +8; and
- `+0x101` pushes `0x3A83126F` (0.001f), followed by the call to the
  target at `0x48e7f0`.

Constructor instruction proof:

- stores definition +8, detached object -1 at +0xC, point at +0x20,
  vector at +0x2C, four color dwords at +0x38, scale argument dword at +0x14,
  and ORs active bit 0 at +4;
- `+0x90..+0x9C` passes FALSE, a local diffuse RGB, system lighting +0x48,
  and system position to `_light_particle` in the real four-argument ABI; and
- `+0xA0..+0xBE` calls the private initializer and, only on false, calls
  `datum_delete(particle_systems, system_index)` before returning -1.

The target normalized facts retained by the donor audit are initializer
278 meaningful / 288 padded bytes, 9 relocations, SHA
`ca7322edb56c3275a3480eed1dfbfd87edfb8152a11ff78a201a35fceee09409`,
and constructor 205 / 208 bytes, 8 relocations, SHA
`39b66f0d516aefd4eee70e8ddb5c9ce80138d8a1f11f467f688738ef348b8e38`.

## Name/type corroboration and its limits

January atlas file
`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research/halo-symbol-atlas/symbols/halo_ce/4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520.jsonl`
(SHA-256 `851802ac508bbcf7cd2896c6cfddd1e0f81510b7b9ef9ea02f39f913c123578e`)
records at lines 1165-1166 the exact C++ parameter names
`particle_system_datum *`, `particle_type *`, `ps_particle_datum *`, and
`old_particle_system_type *`; lines 1173, 1177, and 1180 name
`particle_system_update`, `particle_system_initialize`, and
`particle_system_new_unattached`.

The January `cachebeta.pdb` SHA-256 is
`8480f0c44fc7b5acba5775c02053d1a62c6794ab8d646661106985cbe46d7bc5`.
Direct DIA `-type` queries for the four records return no UDT data, so it is not
a field-name source.

HCEA Release PDB:

`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research/hcea_jun2011_prototype/payload/Halo CE Anniversary (Jun 24 2011)/HCEX_Release.pdb`

SHA-256:
`0fd11b084fd2169cafe071fd8c3e4083d6e9564f01e7964f6c26e5a03eee4dff`.
Reader:
`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research/tools/DIA2Dump/x64/Release/DIA2Dump.exe`, SHA-256
`5be3b890fb5c3b7c74b7c3ce7c275a70f48813c388d3611c41e69c0ac0626243`.

Replay:

```powershell
$dia = 'C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research/tools/DIA2Dump/x64/Release/DIA2Dump.exe'
$pdb = 'C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research/hcea_jun2011_prototype/payload/Halo CE Anniversary (Jun 24 2011)/HCEX_Release.pdb'
& $dia -type particle_system_type $pdb
& $dia -type particle_system_type_state $pdb
& $dia -type particle_type $pdb
& $dia -type particle_system_datum $pdb
& $dia -sym light_particle $pdb
& $dia -sym particle_system_initialize $pdb
```

Those queries independently corroborate every exposed offset/type and the
`light_particle(const real_point3d *, real_rgb_color *, real_rgb_color *,
unsigned char)` ABI.  The canonical owner is
`source/objects/object_lights.h` (frozen-before SHA-256
`76b4b8e950e0d75cfcd0c0712e9921da85622aa7e28ec63941cb44f855afd8eb`),
with its real definition in `source/objects/object_lights.c`.  HCEA's later
initializer is not source chronology for January: it initializes state to -1
and adds `any_particle_seeded`; January initializes state to 0 and treats an
empty system type block as success.  Only the type names and agreeing layouts
are imported.

## Actual header-consumer closure

The frozen before manifest's `.ninja_deps` capture gives the exact direct
closure:

- `particle_systems.h`: `effects.obj`, `particle_systems.obj`, `game.obj`,
  `objects.obj`, `scenario.obj`;
- `particle_system_definitions.h`: `effects.obj`, `particle_systems.obj`,
  `objects.obj`.

Thus the union is exactly five base objects.  The previous real Ninja schema
run confirms those five compiler steps in that order in
`scratch/particle-systems-fable-pair-schema-ninja-20260906.log`; the later
`object_lights.h` include is a text change only in `particle_systems.c`, already
inside that five-object closure.  Freeze/compare all five rather than assuming
the two definition-header consumers bound the blast.

The frozen five-consumer all-section comparator is:

```powershell
python scratch/frozen_owner_packet_compare_20260905.py `
  scratch/<before>.json scratch/<after>.json scratch/<new-output-directory>
```

Comparator SHA-256 is
`039a5e47f4d178543587fe886c316f69e034120db69d5645d4da98e49f44ab38`;
its packet library is
`e700b3ffc4849d96f04f6327a4a3da7cdf1ab6eeccbf706828dd81f00aef320b`
and all-section library is
`76d1978e49e62cfd7b6734ee578644729857e92158c533036b8777ab7b783f50`.

## Explicit exclusions

- No later-only `any_particle_seeded`, render-enabled precheck, state -1
  initializer, interpolated-variable fields, physics union, shaders, or
  particle-state records.
- No declaration-order reversal for strict shaping, source variants, forced
  inline/noinline, manual math wrapper, raw-offset access, or fake external
  declaration for a private missing body.
- No credit for the inline random helper and no complete-object/link claim while
  the real `particle_system_update` graph remains unwritten.
- No Opus-reserved implementation scope is touched.
