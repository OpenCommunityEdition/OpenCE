/*
OBJECT_SHADOWS.C

symbols in this file:
0012B870 0010:
	_code_0012b870 (0000)
0012B880 0050:
	_code_0012b880 (0000)
0012B8D0 0080:
	_object_build_shadow (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "objects/objects.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct object_shadow
{
	real object_bounding_radius;
	real_rectangle3d bounds;
	short count;
	short pad;
};

/* ---------- prototypes */

void _ReadWriteBarrier(void);
#pragma intrinsic(_ReadWriteBarrier)

/* ---------- globals */

/* ---------- public code */

__declspec(naked) void *code_0012b870(void)
{
	__asm
	{
		push NONE
		push eax
		call object_get_and_verify_type
		add esp, 8
		ret
	}
}

void code_0012b880(long object_index, void const *context, struct object_shadow *shadow)
{
	while (object_index != NONE)
	{
		struct object_datum *object = object_get(object_index);

		object_get(object_index);
		code_0012b880(object->object.first_child_object_index, context, shadow);
		object_index = object->object.next_object_index;
	}
}

boolean object_build_shadow(long object_index, void const *context, struct object_shadow *shadow)
{
	struct object_datum *object = object_get(object_index);
	struct object_definition *definition = object_definition_get(object->definition_index);

	shadow->object_bounding_radius = definition->object.bounding_radius;
	shadow->bounds.x0 = REAL_MAX;
	shadow->bounds.x1 = -REAL_MAX;
	shadow->bounds.y0 = REAL_MAX;
	shadow->bounds.y1 = -REAL_MAX;
	shadow->bounds.z0 = REAL_MAX;
	shadow->bounds.z1 = -REAL_MAX;
	shadow->count = 0;
	shadow->pad = 0;

	object_get(object_index);
	code_0012b880(object->object.first_child_object_index, context, shadow);
	if (shadow->count > 0)
	{
		_ReadWriteBarrier();
		return TRUE;
	}

	_ReadWriteBarrier();
	return FALSE;
}

/* ---------- private code */
