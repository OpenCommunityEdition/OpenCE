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

/* ---------- private code */
