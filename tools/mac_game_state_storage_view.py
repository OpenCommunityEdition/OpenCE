#!/usr/bin/env python
"""Native view preserving physical-map ownership of the borrowed game buffer."""
import sys
source = open('source/saved games/game_state_xbox.c').read()
old = '\tXPhysicalFree(xbox_game_state_globals.buffer);'
borrow = '\tresult = physical_memory_get_game_state_base_address();'
if source.count(old) != 1 or source.count(borrow) != 1:
    raise SystemExit('Game-state buffer ownership boundary changed')
new = '''\t/* port: allocate_buffer borrows the physical-map owner's allocation.
\t   It remains owned there until physical_memory_free at shell teardown. */
\tmatch_assert(__FILE__, __LINE__, xbox_game_state_globals.buffer == physical_memory_get_game_state_base_address());'''
if len(sys.argv) != 2:
    raise SystemExit('usage: mac_game_state_storage_view.py output.c')
source = source.replace(old,new)
# port: a checkpoint is a copy of the game state in memory, not a 20 MB write
# and read of z:\\savegame.bin (a continue is the persistent storage's file,
# written by game_state_save_to_persistent_storage). HALO_MAC_CHECKPOINT_FILE=1
# is the original file.
write_head = 'boolean game_state_write_to_file(\n\tvoid)'
read_head = 'boolean game_state_read_from_file(\n\tvoid)'
if source.count(write_head) != 1 or source.count(read_head) != 1:
    raise SystemExit('Game-state file boundary changed')
source = source.replace(write_head, 'static boolean mac_game_state_write_to_file_original(\n\tvoid)')
source = source.replace(read_head, 'static boolean mac_game_state_read_from_file_original(\n\tvoid)')
marker = 'boolean game_state_write_core('
if source.count(marker) != 1:
    raise SystemExit('Game-state core boundary changed')
memory = '''/* port: the checkpoint kept in memory (mac_game_state_storage_view.py) */
static boolean mac_game_state_checkpoint_in_memory(void)
{
	const char *value = getenv("HALO_MAC_CHECKPOINT_FILE");
	return !(value && value[0] == '1');
}

static void mac_game_state_checkpoint_prepare(void)
{
	if (mac_game_state_checkpoint_in_memory() && !mac_game_state_checkpoint_image)
	{
		mac_game_state_checkpoint_image = malloc(GAME_STATE_FILE_SIZE);
		if (mac_game_state_checkpoint_image)
			memset(mac_game_state_checkpoint_image, 0, GAME_STATE_FILE_SIZE);
	}
}

boolean game_state_write_to_file(
	void)
{
	if (!mac_game_state_checkpoint_in_memory())
		return mac_game_state_write_to_file_original();
	match_assert(__FILE__, __LINE__, xbox_game_state_globals.buffer_allocated);
	if (!mac_game_state_checkpoint_image)
		mac_game_state_checkpoint_image = malloc(xbox_game_state_globals.buffer_size);
	if (!mac_game_state_checkpoint_image)
		return FALSE;
	memcpy(mac_game_state_checkpoint_image, xbox_game_state_globals.buffer, xbox_game_state_globals.buffer_size);
	xbox_game_state_globals.file_valid_for_read = TRUE;
	return TRUE;
}

boolean game_state_read_from_file(
	void)
{
	if (!mac_game_state_checkpoint_in_memory())
		return mac_game_state_read_from_file_original();
	match_assert(__FILE__, __LINE__, xbox_game_state_globals.buffer_allocated);
	/* (the image is checked and taken as a file's is; it stays for the next revert) */
	return mac_game_state_checkpoint_image && game_state_image_accept(mac_game_state_checkpoint_image, xbox_game_state_globals.buffer_size);
}

'''
source = source.replace(marker, memory + marker)
close = '\tCloseHandle(xbox_game_state_globals.handle);\n'
if source.count(close) != 1:
    raise SystemExit('Game-state close boundary changed')
opened = '\t\txbox_game_state_globals.file_open = TRUE;\n'
if source.count(opened) != 1:
    raise SystemExit('Game-state open boundary changed')
# (the image's pages are made at the map's start, not at the first checkpoint)
gl = 'static struct xbox_game_state_globals_prefix xbox_game_state_globals = { 0 };\n'
if source.count(gl) != 1:
    raise SystemExit('Game-state globals boundary changed')
source = source.replace(gl, gl + '\n/* port: the checkpoint in memory (below) */\nstatic void mac_game_state_checkpoint_prepare(void);\n')
source = source.replace(opened, opened + '\t\tmac_game_state_checkpoint_prepare();\n')
source = source.replace(close, close + '\tfree(mac_game_state_checkpoint_image);\n\tmac_game_state_checkpoint_image = NULL;\n')
decl = 'void game_state_close_file(\n\tvoid)'
if source.count(decl) != 1:
    raise SystemExit('Game-state close boundary changed')
source = source.replace(decl, '/* port: the checkpoint in memory (below) */\nstatic void *mac_game_state_checkpoint_image = NULL;\n\n' + decl)
with open(sys.argv[1], 'w') as output:
    output.write(source)
