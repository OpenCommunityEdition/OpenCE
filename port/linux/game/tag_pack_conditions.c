/*
TAG_PACK_CONDITIONS.C

The names a tag pack's manifest may want it by at start-up
("condition:<name>"), beside "always" and "setting:<key>" (tag_pack.c):
each a question only code can answer. OpenCE has none yet; a build adds its
own here.
*/

#include "cseries.h"

#include "tag_pack.h"

struct tag_pack_condition const tag_pack_conditions[] =
{
	{ NULL, NULL },
};
