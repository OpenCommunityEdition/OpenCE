#!/usr/bin/env python
"""Generate typed native storage from the decompiled units that declare it."""
import sys

def exact(source, value, name):
	if source.count(value) != 1:
		raise ValueError('%s declaration count changed' % name)

game = open('source/game/game_engine.c').read()
stage = '''struct game_engine_stage
{
	char map_name[64];
	struct game_variant variant;
};'''
exact(game, stage, 'game_engine_stage')

ui = open('source/interface/ui_widget_game_data_input_functions.c').read()
playlist = '''struct playlist_profile
{
	wchar_t name[12];
	enum game_engine_type engine_type;
	boolean teams;
	byte padding1D[0x47];
	word flags;
	byte padding66[2];
};'''
player = '''struct cached_player_profile_entry
{
	long profile_index;
	struct player_profile profile;
};'''
variant = '''struct cached_variant_profile_entry
{
	long profile_index;
	struct playlist_profile profile;
};'''
exact(ui, player, 'cached_player_profile_entry')
exact(ui, variant, 'cached_variant_profile_entry')
exact(ui, playlist, 'playlist_profile')
exact(ui, 'extern struct cached_player_profile_entry cached_player_profile[3];', 'cached_player_profile')
exact(ui, 'extern struct cached_variant_profile_entry cached_variant_profile[3];', 'cached_variant_profile')
event_handlers = open('source/interface/ui_widget_event_handler_functions.c').read()
exact(event_handlers, 'extern byte cached_variant_profile[0x144];', 'cached variant profile extent')

transport = open('source/bungie_net/network/transport_endpoint_set_winsock.c').read()
exact(transport, 'TRANSPORT_NONCE_LENGTH = 8,', 'transport nonce length')
exact(transport, 'extern XNADDR global_address;\nextern XNKID global_key_id;\nextern XNKEY global_key;\nextern byte global_nonce[TRANSPORT_NONCE_LENGTH];', 'transport common declarations')

cache = open('source/cache/cache_files.c').read()
exact(cache, 'extern struct cache_file_tag_instance *global_tag_instances;', 'global tag instances')
progress = open('source/interface/progress_bar.c').read()
exact(progress, 'extern unsigned long current_time;', 'current time')
sound = open('source/sound/sound_dsound_xbox.c').read()
exact(sound, 'extern HRESULT interrupt_result;', 'DirectSound interrupt result')

body = '''/* Typed zero storage for source declarations that had only pooled COMMON
   in the Xbox image. This creates no initialized subsystem or service. */
#include "engine_xdk_prefix.h"
#include "game/game_engine.h"
#include "saved games/player_profile.h"
#include "saved games/playlist_profile.h"

%s
struct game_engine_stage global_stage;

%s
%s
%s
struct cached_player_profile_entry cached_player_profile[3];
struct cached_variant_profile_entry cached_variant_profile[3];

struct XNADDR global_address;
struct XNKID global_key_id;
struct XNKEY global_key;
byte global_nonce[8];
struct cache_file_tag_instance;
struct cache_file_tag_instance *global_tag_instances;
unsigned long current_time;
HRESULT interrupt_result;

typedef char mac_startup_stage_size[sizeof(struct game_engine_stage)==0xA8?1:-1];
typedef char mac_startup_profiles_size[sizeof(cached_player_profile)==0x9C?1:-1];
typedef char mac_startup_variants_size[sizeof(cached_variant_profile)==0x144?1:-1];
typedef char mac_startup_xnaddr_size[sizeof(struct XNADDR)==12?1:-1];
typedef char mac_startup_xnkey_size[sizeof(struct XNKEY)==16?1:-1];
typedef char mac_startup_xnkid_size[sizeof(struct XNKID)==8?1:-1];
typedef char mac_startup_nonce_size[sizeof(global_nonce)==8?1:-1];
typedef char mac_startup_long_size[sizeof(current_time)==4?1:-1];
typedef char mac_startup_hresult_size[sizeof(interrupt_result)==4?1:-1];
''' % (stage, playlist, player, variant)

if len(sys.argv) != 2:
	sys.stderr.write('usage: mac_startup_globals_view.py output.c\n')
	sys.exit(2)
open(sys.argv[1], 'w').write(body)
print('generated source-checked typed startup COMMON storage')
