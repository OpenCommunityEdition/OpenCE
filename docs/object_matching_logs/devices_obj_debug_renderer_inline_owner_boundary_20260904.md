# Devices debug renderer: first-shot strict body withheld for helper ownership (2026-09-04)

## Result

Starting from published commit `4c862ae20ddee514c75e73adc196d6a89260b288`,
one natural reconstruction of `device_render_debug` reproduces all **426
meaningful / 432 padded bytes** and all **27 ordered relocations**. Target and
candidate normalized SHA-256 are
`52c730800fb71562c92ee6190ba88d402404e9fe70dc9d1132f2e116993ea235`.
All 22 inherited exact Devices owners remain exact.

**Do not integrate or credit this packet yet.** The complete emitted-symbol
guard finds a candidate-only **48-byte `_point_from_line3d`** code definition,
which violates the explicit January inline-schedule rule. The new caller is
strict but that does not authorize the extra owner. No production Devices
source/header/config was changed and no matching bytes or object completion
are claimed.

## Evidence and reconstruction

January alone provides the implementation: the HCEA cached function is a later
bare no-op and is not a useful semantic donor. The January function checks the
typed device, then the existing `debug_objects_devices` flag; constructs a
512-byte power/position/group-state debug string; obtains the object origin;
offsets it along `global_up3d` by 0.4; and renders it in white. The current
`DATUM_INDEX_TO_ABSOLUTE_INDEX` macro naturally expresses the target's word
zero-extension for display. Project `real_point3d`, checked `device_get`,
real datum fields and owning render/object declarations are used.

The flag's named January symbol is at file offset 5,842,660, in the genuine
one-byte contribution assigned to synthetic split module 847
(`source/linker_common.c`). The scratch proposal uses only an external
declaration and does not manufacture a new BSS owner. Before production
integration, put the public renderer prototype and appropriate shared flag
declaration in their actual owner header, remove Object Types' caller-local
renderer prototype, and audit the corresponding dependency rebuild. None of
that pending work is bypassed by this isolated function experiment.

The candidate calls the existing ordinary `point_from_line3d`; it does not
manually expand the helper, add inline/assembly directives, change compiler
flags, or alter the math header. That natural call is sufficient for the
strict caller bytes, but VC7 also retains the prohibited out-of-line copy.
There was no spelling, lifetime, declaration-order or register search.

## Reproduction

```text
python tools/campaign/gate.py source/devices/devices --source scratch/devices-debug-natural-proposal-20260904.c --all --forbid-emitted-symbol _point_from_line3d --out scratch/devices-debug-natural-proposal-20260904.obj
```

The guard fails explicitly: code symbol index 10, section 3, extent 48.
A separate read-only `coff_compare.section_infos_equal` inspection proves the
432-byte caller strict and finds no inherited exact loss; the only extra code
owner is `_point_from_line3d`. This inspection does not override the failed
guard.

Frozen physical artifacts:

- scratch proposal source SHA-256:
  `531536331fcfc29df187bdef06e237ccd9de05a3160a91b62bc61c42d2783cdd`;
- scratch proposal object SHA-256:
  `4e5e1298f595787ed6b2a4c18408564f15896d04d130b4cf1e45baf6f03871a3`.

The source proposal is the baseline Devices C with the real render-debug
header included, a temporary scratch-only external flag declaration, and
the body below before its private-code section. Preserve this body as the
evidence-backed candidate; reopen only with authentic shared-helper ownership
or inline-topology evidence satisfying the explicit no-COMDAT rule. Do not
repeat the implementation search or count these bytes as already integrated.

```c
void device_render_debug(
	long device_index)
{
	struct device_datum *device = device_get(device_index);

	if (debug_objects_devices)
	{
		char string[512];
		real_point3d origin;

		csstrcpy(string, "");
		sprintf(string+csstrlen(string), "power %.2f/vel %.2f",
			device->device.power,
			device->device.power_velocity);
		if (device->device.power_group_index != NONE)
		{
			struct device_group_datum *group = datum_get(
				device_groups_data,
				device->device.power_group_index);

			sprintf(string+csstrlen(string), " (group %d desired %.2f)",
				DATUM_INDEX_TO_ABSOLUTE_INDEX(device->device.power_group_index),
				group->actual_value);
		}

		sprintf(string+csstrlen(string), "|nposition %.2f/vel %.2f",
			device->device.position,
			device->device.position_velocity);
		if (device->device.position_group_index != NONE)
		{
			struct device_group_datum *group = datum_get(
				device_groups_data,
				device->device.position_group_index);

			sprintf(string+csstrlen(string), " (group %d desired %.2f)",
				DATUM_INDEX_TO_ABSOLUTE_INDEX(device->device.position_group_index),
				group->actual_value);
		}

		object_get_origin(device_index, &origin);
		point_from_line3d(&origin, global_up3d, 0.4f, &origin);
		render_debug_string_at_point(FALSE, &origin, string, global_real_argb_white);
	}

	return;
}
```
