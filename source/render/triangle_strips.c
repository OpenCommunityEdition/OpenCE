/*
TRIANGLE_STRIPS.C

symbols in this file:
0017D7C0 0070:
	_triangle_strip_iterator_new (0000)
0017D830 0160:
	_triangle_strip_iterator_get_triangle (0000)
002A044C 001e:
	??_C@_0BO@MPPKGOLO@triangle_strip_vertex_indices?$AA@ (0000)
002A046C 0028:
	??_C@_0CI@EBGJOIHP@c?3?2halo?2SOURCE?2render?2triangle_s@ (0000)
002A0494 001a:
	??_C@_0BK@DKBJAIM@iterator?9?$DOvertex_count?$DO?$DN0?$AA@ (0000)
002A04B0 0035:
	??_C@_0DF@FKCNENJG@iterator?9?$DOsignature?$DN?$DN_valid_stri@ (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "triangle_strips.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

void triangle_strip_iterator_new(
	struct triangle_strip_iterator *iterator,
	word const *triangle_strip_vertex_indices,
	short strip_count)
{
	match_assert("c:\\halo\\SOURCE\\render\\triangle_strips.c", 22, iterator);
	match_assert("c:\\halo\\SOURCE\\render\\triangle_strips.c", 23, triangle_strip_vertex_indices);

	iterator->triangle_strip_vertex_indices = triangle_strip_vertex_indices;
	iterator->strip_count = strip_count;
	iterator->vertex_count = 0;
	iterator->signature = _valid_strip_iterator_signature;
}

/* ---------- private code */
