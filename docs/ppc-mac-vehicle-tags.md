# Native vehicle tag conversion

`tools/mac_native_vehicle_layout.py` extracts `vehicle_definition` directly
from `source/units/vehicles.c`. Its generated header checks the 0x3f0-byte
size and the offsets of the inherited unit, vehicle flags/type, function
modes, and trailing references. `tools/mac_native_object_schema.py` then emits
the endian conversion schema. Reference groups and nested block limits come
from the existing `tag_schema_objects.c` metadata; no vehicle family metadata
was changed.

The decoder rejects a wrong root type, a non-vehicle object type, vehicle or
function-mode enum values outside their source domains, invalid salted
references, and oversized nested blocks. Its output owns nested block storage
and referenced names. This only converts the definition: the model, animation,
physics, sounds, effects, and other dependencies still need their own loading
and runtime validation before placement or simulation.

The vehicle declaration includes byte arrays `unknown324` and `unknown368`.
They are preserved byte-for-byte as opaque data because a source and port
consumer search found no field-level reads that require endian conversion.
The explicitly unused arrays and padding are omitted from field conversion
descriptors and preserved as bytes; no gameplay consumer was found for them either.

`tools/mac_native_vehicle_checks.sh` checks all five Blood Gulch vehicle
palette roots and the map's 28 vehicle placements. It also tests malformed
root type, salted model reference, vehicle/function-mode enum, and seat-block
count rejection, then confirms decoded references still work after releasing
the map bytes. Passing this probe does not establish that the vehicles can be
spawned or driven.

The Tiger GCC 4.0 PPC and i386 builds passed, and the PPC binary passed all
138 checks on both the PowerBook G4 and iBook G4. The five roots and reference
digests matched on both machines. Logs are `build/mac-audit/vehicle-build-intel.txt`,
`build/mac-audit/powerbook-vehicle.txt`, and `build/mac-audit/ibook-vehicle.txt`.
The tested PPC binary SHA-256 is
`d99a45698d86c55dbb9e7ba2416a06b43b5e459a0ff479ebef36402783a53c98`.
