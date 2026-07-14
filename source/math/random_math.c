/*
RANDOM_MATH.C

symbols in this file:
000FA8B0 0010:
	_lock_global_random_seed (0000)
000FA8C0 0030:
	_unlock_global_random_seed (0000)
000FA8F0 0040:
	_get_global_random_seed_address (0000)
000FA930 0010:
	_get_random_seed (0000)
000FA940 0010:
	_get_global_local_random_seed_address (0000)
000FA950 0010:
	_random_seed_debug_log (0000)
000FA960 0020:
	_get_number_suitable_for_initializing_random_seed (0000)
000FA980 00c0:
	_random_math_initialize (0000)
000FAA40 0020:
	_random_math_dispose (0000)
000FAA60 0030:
	_real_seed_random (0000)
000FAA90 0040:
	_real_seed_random_range (0000)
000FAAD0 0020:
	_seed_random (0000)
000FAAF0 0030:
	_seed_random_range (0000)
000FAB20 0080:
	_code_000fab20 (0000)
000FABA0 0040:
	_seed_random_direction3d (0000)
000FABE0 0100:
	_seed_random_orientation (0000)
000FACE0 0100:
	_seed_random_vector_in_cone3d (0000)
0027AE88 0031:
	??_C@_0DB@GOKAHAFJ@unmatched?5call?5to?5unlock_random_@ (0000)
0027AEBC 0022:
	??_C@_0CC@CGOEMOHM@c?3?2halo?2SOURCE?2math?2random_math?4@ (0000)
0027AEE0 0044:
	??_C@_0EE@KKMKHIJL@you?5should?5not?5be?5using?5global?5r@ (0000)
0027AF24 001b:
	??_C@_0BL@CJPMPDBB@random_direction_geosphere?$AA@ (0000)
0027AF40 0042:
	??_C@_0EC@LPLKDAPN@index?$DO?$DN0?5?$CG?$CG?5index?$DMrandom_math_gl@ (0000)
0027AF84 002b:
	??_C@_0CL@KJMLMACI@random_math_globals?4random_direc@ (0000)
00456208 0014:
	_bss_00456208 (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "cseries_windows.h"
#include "real_math.h"
#include "game_engine.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct random_math_globals
{
	void *random_direction_table;
	short random_direction_table_size;
	short pad;
	long global_random_seed_lock;
	unsigned long global_random_seed;
	unsigned long global_local_random_seed;
};

/* ---------- prototypes */

unsigned long system_seconds(void);
void code_000fab20(void);

/* ---------- globals */

static struct random_math_globals bss_00456208;

/* ---------- public code */

void lock_global_random_seed(void)
{
	bss_00456208.global_random_seed_lock++;
}

void unlock_global_random_seed(void)
{
	match_vassert(
		"c:\\halo\\SOURCE\\math\\random_math.c",
		41,
		bss_00456208.global_random_seed_lock>0,
		"unmatched call to unlock_random_seed()");
	bss_00456208.global_random_seed_lock--;
}

unsigned long get_random_seed(void)
{
	return bss_00456208.global_random_seed;
}

unsigned long *get_global_random_seed_address(void)
{
	match_vassert(
		"c:\\halo\\SOURCE\\math\\random_math.c",
		56,
		!game_engine_running() || !bss_00456208.global_random_seed_lock,
		"you should not be using global random(); use local random() instead");
	return &bss_00456208.global_random_seed;
}

unsigned long *get_global_local_random_seed_address(void)
{
	return &bss_00456208.global_local_random_seed;
}

void random_seed_debug_log(
	boolean log)
{
}

unsigned long get_number_suitable_for_initializing_random_seed(void)
{
	return system_seconds()^system_milliseconds()^rand();
}

void random_math_dispose(void)
{
	debug_free(
		bss_00456208.random_direction_table,
		"c:\\halo\\SOURCE\\math\\random_math.c",
		200);
}

unsigned short seed_random(
	unsigned long *seed)
{
	*seed = *seed*0x19660D+0x3C6EF35F;
	return *seed>>16;
}

short seed_random_range(
	unsigned long *seed,
	short lower_bound,
	short upper_bound)
{
	return lower_bound+((unsigned long)(upper_bound-lower_bound)*seed_random(seed)>>16);
}

real real_seed_random(
	unsigned long *seed)
{
	*seed = *seed*0x19660D+0x3C6EF35F;
	return (real)(*seed>>16)/65535.0f;
}

real real_seed_random_range(
	unsigned long *seed,
	real lower_bound,
	real upper_bound)
{
	real random= real_seed_random(seed);
	return lower_bound+(upper_bound-lower_bound)*random;
}

__declspec(naked) real_vector3d *seed_random_direction3d(
	unsigned long *seed,
	real_vector3d *direction)
{
	__asm
	{
		push ebp
		mov ebp, esp
		mov ecx, seed
		mov eax, dword ptr [bss_00456208+4]
		push ebx
		push esi
		mov esi, dword ptr [ecx]
		mov ebx, direction
		imul esi, esi, 019660Dh
		add esi, 03C6EF35Fh
		mov dword ptr [ecx], esi
		movsx eax, ax
		shr esi, 16
		imul esi, eax
		shr esi, 16
		call code_000fab20
		pop esi
		pop ebx
		pop ebp
		ret
	}
}

/* ---------- private code */
