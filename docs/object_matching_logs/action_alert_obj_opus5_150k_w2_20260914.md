# action_alert.obj — opus5 150K lane, wave w2 (2026-09-14)

## Baseline and final

| gate | result |
| --- | --- |
| baseline real file | `== exact 11 residual 1 unwritten 0`, guard passes |
| final real file (unchanged) | `== exact 11 residual 1 unwritten 0`, guard passes, park drift 0, fake scan 0 leads |

`git diff --stat -- source/ai/action_alert.c` is empty. No credit is claimed.

## `_action_alert_next_position` (parked, register-allocation)

Target 784 / 18 relocs / nsha `677fd6cc1a9e66de`; real-file candidate 768 / 18 / `4e16c85b544ded33`.

### New evidence

Aligned diff of the real file:

- January preloads `or eax,0xffffffff` before the `vehicle_passenger` test (3 B).
- January's NONE returns are 32-bit (`mov eax,0xffffffff` on the no-position tail, `or eax,-1` in the guard return block at +0x2f8).
- The encounter guard jumps past that block because EAX already holds -1.

Measured VC7 facts:

- `return NONE;` in a `short` function emits a 16-bit `or ax,0xffff`. So does `return result;` when result is constant-propagated (d7).
- Only an assignment to a `short` result variable emits the 32-bit form.

The function is therefore single-exit (brief section 9 single-exit law). The earlier park probes had not tested this with the complete assignment topology.

### Shapes

| shape | result |
| --- | --- |
| v1: init `result = NONE`, three-way `&&` guard, `if (any) {...}`, `result = next` | 784/18 `2c7d5520` (shrink-wrapped pushes, result in ECX) |
| v2: nested guard; v3: result merged into `next_position_index` | identical to v1 |
| v4: no init, `else { result = NONE; }`, `if (!any) result = NONE;` | 768/18 `6ac81fbc` (guards merged into one block, no preload) |
| v5: nested if/else; n1: `else if (encounter == NONE) result = NONE;` | identical to v4 |
| diagnostic d5: guards `return NONE`, init, no-position store, loop `return` | 1 instruction off (`or ax,0xffff`) |
| diagnostic ea: ec without the no-position store | frame 0x24, wrong |
| **ec / d10** | **strict EXACT 784/18 `677fd6cc`; whole TU 12/12 exact** |

The ec / d10 body:

```c
short result = NONE;

if (actor->input.vehicle_passenger || move_position_order == _move_position_order_none)
{
	result = NONE;                       /* redundant with the initialiser */
}
else if (actor->meta.encounter_index != NONE)
{
	/* encounter/squad lookup, swarm assert */
	if (move_position_order == _move_position_order_repeat && current_position_index != NONE)
		result = current_position_index;
	else
	{
		/* availability scan (unchanged) */
		if (!any_position_available)
			result = NONE;               /* redundant with the initialiser */
		else if (move_position_order == _move_position_order_random)
			result = choose_random_array_element(...);
		else
		{
			/* clamp + do/while traversal (unchanged) */
			result = next_position_index;
		}
	}
}
return result;
```

Scratch candidate: `scratch/workers/model_animations_actor_stimulus_etc/aa_ec.c` (patch `aa_ec.patch`). Its checks:

- the 11 siblings stay exact;
- the `_point_from_line3d` guard passes;
- parkcheck reports drift only on the park itself;
- fake scan finds 0 leads (the scanner does not detect redundant stores);
- the function set is unchanged.

### Disposition

This is not proposed as an admissible reopen. Exactness requires two NONE stores that the initialiser already guarantees. Every non-redundant spelling measured returns to a different fixed point (v1-v5, n1, d11). Brief section 5 prohibits redundant stores. Previous verifiers rejected the `_action_vehicle_desirable` and `_action_obey_command_begin` re-stores. `_extract_sequence` (January-proven redundant logic) was also left parked. This is an **owner-ruling item**.

### Reopen criterion

Either of:

- an owner ruling that admits January-proven redundant default-result stores (land the aa_ec.c body with a disclosure comment and unpark);
- a non-redundant spelling that produces the separate 32-bit NONE materialisations.

## Do-not-repeat

v1-v5, n1, d11 (no-init explicit else after the encounter branch), and ea.
