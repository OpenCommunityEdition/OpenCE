/*
XBOX_SOUND_CACHE.C

symbols in this file:
001AD980 0030:
	_sound_cache_delete (0000)
001AD9B0 0010:
	_sound_cache_open (0000)
001AD9C0 0040:
	_sound_cache_idle (0000)
001ADA00 0050:
	_sound_cache_sound_new (0000)
001ADA50 0130:
	_sound_cache_sound_delete (0000)
001ADB80 0070:
	_sound_cache_sound_finished (0000)
001ADBF0 0040:
	_sound_cache_sound_hardware_lock (0000)
001ADC30 0030:
	_sound_cache_sound_hardware_unlock (0000)
001ADC60 0040:
	_code_001adc60 (0000)
001ADCA0 00c0:
	_code_001adca0 (0000)
001ADD60 0040:
	_code_001add60 (0000)
001ADDA0 0130:
	_code_001adda0 (0000)
001ADED0 00b0:
	_sound_cache_new (0000)
001ADF80 0060:
	_sound_cache_flush (0000)
001ADFE0 0060:
	_sound_cache_close (0000)
001AE040 0160:
	__sound_cache_sound_request (0000)
001AE1A0 0100:
	_code_001ae1a0 (0000)
001AE2A0 0170:
	_sound_cache_debug_render (0000)
002A7818 0028:
	??_C@_0CI@CEOIDJNM@hardware?5sound?5reference?5count?5f@ (0000)
002A7840 0028:
	??_C@_0CI@ECPIFOOO@c?3?2halo?2SOURCE?2cache?2xbox_sound_@ (0000)
002A7868 0020:
	??_C@_0CA@EAEGBHAP@sound?9?$DOcache_base_address?$DN?$DNNULL?$AA@ (0000)
002A7888 0049:
	??_C@_0EJ@CCGMBLDE@tried?5to?5delete?5sound?5?$CFs?$CI?$CFs?$CJ?5fro@ (0000)
002A78D8 0049:
	??_C@_0EJ@LIMGNEKN@tried?5to?5delete?5sound?5?$CFs?$CI?$CFs?$CJ?5fro@ (0000)
002A7924 0026:
	??_C@_0CG@DKDBDOMH@cache_sound?9?$DOsoftware_reference_@ (0000)
002A794C 0011:
	??_C@_0BB@DNICGKNA@?9?9?9?5finish?5?$CFd?5?$CFs?$AA@ (0000)
002A7960 0033:
	??_C@_0DD@CMHIKMHN@cache_sound?9?$DOsound?9?$DOcache_block_@ (0000)
002A7998 0042:
	??_C@_0EC@GIEKOHPI@tried?5to?5delete?5sound?5?$CFs?$CI?$CFs?$CJ?5fro@ (0000)
002A79DC 0008:
	??_C@_07IFMLPBAP@?$CFs?5?$CI?$CFs?$CJ?$AA@ (0000)
002A79E4 000f:
	??_C@_0P@IBINCFI@d?3?2stabbed?4txt?$AA@ (0000)
002A79F8 0046:
	??_C@_0EG@BAHEDCLE@SOUND?5CACHE?5BLOWN?$CB?$CB?$CB?$CB?5double?9cli@ (0000)
002A7A40 0043:
	??_C@_0ED@KGGMCECB@?$CB?$CB?$CB?$CB?$CB?$CB?$CB?$CB?$CB?$CB?$CB?$CB?$CB?$CB?$CB?$CB?$CB?$CB?$CB?$CB?$CB?$CB?$CB?$CB?$CB?$CB?$CB?$CB?$CB?$CB?$CB?$CB@ (0000)
002A7A84 0029:
	??_C@_0CJ@LADKOEPK@new_cache_sound_index?$DN?$DNcache_blo@ (0000)
002A7AB0 0026:
	??_C@_0CG@ICHLCFN@xbox_sound_cache_globals?4base_ad@ (0000)
002A7AD8 001f:
	??_C@_0BP@BPBJCKGO@xbox_sound_cache_globals?4cache?$AA@ (0000)
002A7AF8 0011:
	??_C@_0BB@KEGBILOA@xbox?5sound?5cache?$AA@ (0000)
002A7B0C 0026:
	??_C@_0CG@BBBHJDMH@xbox_sound_cache_globals?4cache_s@ (0000)
002A7B34 000b:
	??_C@_0L@LPLEFDPA@xbox?5sound?$AA@ (0000)
002A7B40 0038:
	??_C@_0DI@BGGMDELJ@cache_sound?9?$DOsoftware_reference_@ (0000)
002A7B78 0012:
	??_C@_0BC@MNBCOGO@?9?9?9?5request?5?$CFd?5?$CFs?$AA@ (0000)
002A7B8C 001a:
	??_C@_0BK@KPEOMHDP@sound?9?$DOcache_tag_index?$CB?$DN0?$AA@ (0000)
002A7BA8 0013:
	??_C@_0BD@CEJPLKLE@load?5?$HM?$HM?5?$CBreference?$AA@ (0000)
002A7BBC 000f:
	??_C@_0P@FHHOOBEP@load?5?$HM?$HM?5?$CBblock?$AA@ (0000)
004D1088 0110:
	_bss_004d1088 (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "cseries/errors.h"
#include "cache/physical_memory_map.h"
#include "memory/data.h"
#include "memory/lruv_cache.h"
#include "sound/sound_definitions.h"
#include "tag_files/tag_files.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct xbox_cache_sound_datum
{
	byte reserved000[4];
	byte software_reference_count;
	byte hardware_reference_count;
	byte reserved006[2];
	struct sound_permutation *sound;
};

struct xbox_sound_cache_globals_prefix
{
	byte reserved000[0x100];
	struct data_array *cache_sounds;
	byte *base_address;
	struct lruv_cache *cache;
};

typedef char verify_xbox_cache_sound_hardware_reference_count_offset[
	offsetof(
		struct xbox_cache_sound_datum,
		hardware_reference_count) == 0x5 ? 1 : -1];
typedef char verify_xbox_cache_sound_software_reference_count_offset[
	offsetof(
		struct xbox_cache_sound_datum,
		software_reference_count) == 0x4 ? 1 : -1];
typedef char verify_xbox_cache_sound_sound_offset[
	offsetof(
		struct xbox_cache_sound_datum,
		sound) == 0x8 ? 1 : -1];
typedef char verify_xbox_cache_sound_datum_size[
	sizeof(struct xbox_cache_sound_datum) == 0xC ? 1 : -1];
typedef char verify_xbox_sound_cache_sounds_offset[
	offsetof(
		struct xbox_sound_cache_globals_prefix,
		cache_sounds) == 0x100 ? 1 : -1];
typedef char verify_xbox_sound_cache_base_address_offset[
	offsetof(
		struct xbox_sound_cache_globals_prefix,
		base_address) == 0x104 ? 1 : -1];
typedef char verify_xbox_sound_cache_cache_offset[
	offsetof(
		struct xbox_sound_cache_globals_prefix,
		cache) == 0x108 ? 1 : -1];
typedef char verify_xbox_sound_cache_globals_prefix_size[
	sizeof(struct xbox_sound_cache_globals_prefix) == 0x10C ? 1 : -1];

/* ---------- prototypes */

void sound_cache_sound_delete(
	struct sound_permutation *sound);
boolean code_001adc60(
	long block_index);
void code_001adca0(
	long block_index);

/* ---------- globals */

extern short assertion_count;
extern boolean debug_sound_reference_counts;
extern struct xbox_sound_cache_globals_prefix bss_004d1088;

/* ---------- public code */

void sound_cache_delete(
	void)
{
	data_dispose(bss_004d1088.cache_sounds);
	lruv_delete(bss_004d1088.cache);
	bss_004d1088.base_address = NULL;

	return;
}

void sound_cache_open(
	void)
{
	data_make_valid(bss_004d1088.cache_sounds);

	return;
}

void sound_cache_idle(
	void)
{
	lruv_idle(bss_004d1088.cache);
	match_vassert(
		"c:\\halo\\SOURCE\\cache\\xbox_sound_cache.c",
		148,
		assertion_count == 0,
		"hardware sound reference count failure.");

	return;
}

void sound_cache_sound_new(
	long cache_tag_index,
	struct sound_permutation *sound)
{
	match_vassert(
		"c:\\halo\\SOURCE\\cache\\xbox_sound_cache.c",
		158,
		sound->unknown1 == 0,
		"sound->cache_base_address==NULL");
	sound->unknown0 = NONE;
	sound->unknown1 = 0;
	sound->unknown2 = cache_tag_index;

	return;
}

void sound_cache_sound_delete(
	struct sound_permutation *sound)
{
	if (sound->unknown0 != NONE)
	{
		match_vassert(
			"c:\\halo\\SOURCE\\cache\\xbox_sound_cache.c",
			173,
			((struct xbox_cache_sound_datum *)datum_get(
				bss_004d1088.cache_sounds,
				sound->unknown0))->software_reference_count == 0,
			csprintf(
				temporary,
				"tried to delete sound %s(%s) from the cache while it was playing (soft).",
				tag_get_name(((struct xbox_cache_sound_datum *)datum_get(
					bss_004d1088.cache_sounds,
					sound->unknown0))->sound->unknown3),
				((struct xbox_cache_sound_datum *)datum_get(
					bss_004d1088.cache_sounds,
					sound->unknown0))->sound));
		match_vassert(
			"c:\\halo\\SOURCE\\cache\\xbox_sound_cache.c",
			174,
			((struct xbox_cache_sound_datum *)datum_get(
				bss_004d1088.cache_sounds,
				sound->unknown0))->hardware_reference_count == 0,
			csprintf(
				temporary,
				"tried to delete sound %s(%s) from the cache while it was playing (hard).",
				tag_get_name(((struct xbox_cache_sound_datum *)datum_get(
					bss_004d1088.cache_sounds,
					sound->unknown0))->sound->unknown3),
				((struct xbox_cache_sound_datum *)datum_get(
					bss_004d1088.cache_sounds,
					sound->unknown0))->sound));
		lruv_block_delete(bss_004d1088.cache, sound->unknown0);
	}

	sound->unknown0 = NONE;
	sound->unknown1 = 0;

	return;
}

void sound_cache_sound_finished(
	struct sound_permutation *sound)
{
	struct xbox_cache_sound_datum *cache_sound;

	cache_sound = datum_get(bss_004d1088.cache_sounds, sound->unknown0);
	if (debug_sound_reference_counts)
	{
		error(
			_error_silent,
			"--- finish %d %s",
			cache_sound->software_reference_count,
			cache_sound->sound);
	}
	match_assert(
		"c:\\halo\\SOURCE\\cache\\xbox_sound_cache.c",
		263,
		cache_sound->software_reference_count);
	cache_sound->software_reference_count--;

	return;
}

void sound_cache_sound_hardware_lock(
	struct sound_permutation *sound)
{
	struct xbox_cache_sound_datum *cache_sound;

	/* January Xbox and HCEA both identify unknown0 as cache_block_index. */
	cache_sound = datum_get(bss_004d1088.cache_sounds, sound->unknown0);
	if (cache_sound->hardware_reference_count < UNSIGNED_CHAR_MAX)
	{
		cache_sound->hardware_reference_count++;
	}
	else
	{
		assertion_count++;
	}

	return;
}

void sound_cache_new(
	void)
{
	bss_004d1088.cache_sounds = data_new(
		"xbox sound",
		512,
		sizeof(struct xbox_cache_sound_datum));
	match_vassert(
		"c:\\halo\\SOURCE\\cache\\xbox_sound_cache.c",
		69,
		bss_004d1088.cache_sounds != NULL,
		"xbox_sound_cache_globals.cache_sounds");
	bss_004d1088.cache = lruv_new(
		"xbox sound cache",
		1024,
		12,
		512,
		code_001adca0,
		code_001adc60);
	match_vassert(
		"c:\\halo\\SOURCE\\cache\\xbox_sound_cache.c",
		73,
		bss_004d1088.cache != NULL,
		"xbox_sound_cache_globals.cache");
	bss_004d1088.base_address = physical_memory_get_sound_cache_base_address();
	match_vassert(
		"c:\\halo\\SOURCE\\cache\\xbox_sound_cache.c",
		76,
		bss_004d1088.base_address != NULL,
		"xbox_sound_cache_globals.base_address");

	return;
}

void sound_cache_flush(
	void)
{
	struct data_iterator iterator;
	struct xbox_cache_sound_datum *cache_sound;

	data_iterator_new(&iterator, bss_004d1088.cache_sounds);
	while ((cache_sound = data_iterator_next(&iterator)) != NULL)
	{
		if (cache_sound->software_reference_count == 0 &&
			cache_sound->hardware_reference_count == 0)
		{
			sound_cache_sound_delete(cache_sound->sound);
		}
	}

	return;
}

void sound_cache_close(
	void)
{
	struct data_iterator iterator;
	struct xbox_cache_sound_datum *cache_sound;

	data_iterator_new(&iterator, bss_004d1088.cache_sounds);
	while ((cache_sound = data_iterator_next(&iterator)) != NULL)
	{
		sound_cache_sound_delete(cache_sound->sound);
	}
	data_make_invalid(bss_004d1088.cache_sounds);

	return;
}

/* ---------- private code */
