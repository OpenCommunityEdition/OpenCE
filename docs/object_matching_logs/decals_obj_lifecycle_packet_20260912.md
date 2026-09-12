# `decals.obj`: lifecycle initializer and unlock packet (2026-09-12)

## Outcome

This packet restores the three coherent January lifecycle bodies
`decals_initialize`, `decals_initialize_for_new_map`, and `decals_unlock`.
Their shared ownership boundary is now complete: the full `decal_geometry`
record was independently authenticated by the retained collision
reconstruction, and `effects/decals.h` already owns all three public
prototypes.

The one-shot unlock diagnostic latches are named
`decals_unlock_locked_count_reported` and
`decals_unlock_permanent_count_reported`.  January proves their one-byte
storage at image offsets `0x43D57E` and `0x43D57F` and their use in the two
specific count diagnostics.  The descriptive spellings are semantic campaign
names, not a claim of original PDB identifier recovery.

The integrated packet also retires the earlier zero-credit park for
`decals_delete_permanent_from_cluster`. Its natural typed body is unchanged,
but restoring the genuine same-TU lifecycle and projection predecessors in
January/source order supplies the authentic compiler-allocation context that
the earlier proper-header migration had removed. The candidate now agrees
with January in all **392 meaningful / 400 padded bytes**, all 29 ordered
relocations, and normalized SHA-256
`b3eeb427eeef1e02f793286b1011a5b89822e041f0b442759259255e1117220a`.
This is not an unrelated declaration perturbation: the restored predecessors
have January RVAs and assertion source lines before the delete routine, use
their correct `decals.h` owner, and are themselves independently reconstructed.
No source-shape tuning was applied to the deletion body.

## January authority and semantic corroboration

The January COFF sections establish the allocation sizes, assertion lines,
field offsets, call graph, reset order, and exact latch destinations.  In
particular:

- the decal data pool has 2,048 elements of `sizeof(struct decal_datum)`
  (`0x38`);
- the globals allocation is `sizeof(struct decal_globals)` (`0x280C`);
- `identifier_zero_invalid` is enabled after allocating the data pool;
- new-map initialization fills the five-by-512 first-index table with `NONE`,
  clears the disconnected head and both counters, validates the data array,
  and initializes the rasterizer map state;
- unlock clears every locked flag, optionally clears every permanent flag,
  repairs inconsistent aggregate counts after one diagnostic per count type,
  and always resets both scratch-geometry counters.

HCEA independently corroborates this behavior, but was used only as semantic
reference.  January remains authoritative for its DEBUG assertions, exact
layout, diagnostics, and code generation.  The earlier Fable intake at
`f9dd42218b3325e1b3a63d7860e5309f69fd3d8e` provides an independent prior
compile of the same natural bodies.

## Isolated grouped gate

One PID-safe whole-TU compile was performed for the complete three-function
packet:

```text
python -B tools/campaign/gate.py source/effects/decals --all \
  --forbid-emitted-symbol _point_from_line3d \
  --out scratch/decals_lifecycle_packet.obj
```

Results:

| function | meaningful | padded | relocs | result |
|---|---:|---:|---:|---|
| `_decals_initialize` | 155 | 160 | 18 | strict exact |
| `_decals_initialize_for_new_map` | 165 | 176 | 18 | strict exact |
| `_decals_unlock` | 364 | 368 | 29 | strict exact after regenerated split metadata |

The three lifecycle functions add **684 meaningful / 704 padded exact bytes**.
The root integration regenerated the split object after applying the two new
static symbol names, so January's aggregate `_bss_00435cb0 + 30926/+30927`
destinations and the candidate's semantic owners now compare as the same
relocations. The normalized SHA-256 is
`bd2b7cb08fd653e4fd630a88d731ee8823d3c94e1e5daec9eb2d548feeb09927`.

Together with the naturally restored 392-byte deletion owner described above,
this coherent lifecycle context contributes **1,076 meaningful / 1,104 padded
strict bytes in four functions**. The final combined Decals gate reports 26
exact / 5 residual / 2 unwritten and passes the `_point_from_line3d`
emitted-symbol guard. The only fake-match scan finding in the combined file is
the separately documented low-level color-packing assembly; this lifecycle
packet introduces no pragma, assembly, intrinsic, volatile qualifier, forced
inline, raw address name, untyped tag/object access, or compiler-steering
source.

The root integration subsequently passed the full Ninja build and stable
repository sweep with no regression; aggregate publication evidence is in the
sixth 10K batch ledger.
