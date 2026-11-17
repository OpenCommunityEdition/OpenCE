#include "harness.h"
#define SET_FLAG(f, b, v) ((v) ? ((f)|=(unsigned)FLAG(b)) : ((f)&=(unsigned)~FLAG(b)))
#include "enums.inc"
struct game_variant { short game_engine_index; struct { unsigned long flags; } universal_variant; };
struct network_game { struct game_variant variant; byte cooperative_flags; };
static int fake_base, fake_override, fake_server;
int config_boolean(char const *name)
{
	if (!strcmp(name, "network.vehicle_riding"))
		return fake_base;
	if (!strcmp(name, "server.vehicle_riding_override_gametype"))
		return fake_override;
	return -1;
}
int halo_server_mode(void) { return fake_server; }
#include "under_test.inc"
struct row { int multiplayer, gametype_rule, host_option, entry_rule, dedicated, override, expected; };
/* written out row by row from the rule's prose: multiplayer takes the gametype's rule, or the host's option where a
dedicated server overrides; else the co-op entry's rule where it gives one, else the host's option */
static struct row const grid[96] = {
	{ 0, 0, 0, NONE , 0, 0, 0 },
	{ 0, 0, 0, NONE , 0, 1, 0 },
	{ 0, 0, 0, NONE , 1, 0, 0 },
	{ 0, 0, 0, NONE , 1, 1, 0 },
	{ 0, 0, 0, FALSE, 0, 0, 0 },
	{ 0, 0, 0, FALSE, 0, 1, 0 },
	{ 0, 0, 0, FALSE, 1, 0, 0 },
	{ 0, 0, 0, FALSE, 1, 1, 0 },
	{ 0, 0, 0, TRUE , 0, 0, 1 },
	{ 0, 0, 0, TRUE , 0, 1, 1 },
	{ 0, 0, 0, TRUE , 1, 0, 1 },
	{ 0, 0, 0, TRUE , 1, 1, 1 },
	{ 0, 0, 1, NONE , 0, 0, 1 },
	{ 0, 0, 1, NONE , 0, 1, 1 },
	{ 0, 0, 1, NONE , 1, 0, 1 },
	{ 0, 0, 1, NONE , 1, 1, 1 },
	{ 0, 0, 1, FALSE, 0, 0, 0 },
	{ 0, 0, 1, FALSE, 0, 1, 0 },
	{ 0, 0, 1, FALSE, 1, 0, 0 },
	{ 0, 0, 1, FALSE, 1, 1, 0 },
	{ 0, 0, 1, TRUE , 0, 0, 1 },
	{ 0, 0, 1, TRUE , 0, 1, 1 },
	{ 0, 0, 1, TRUE , 1, 0, 1 },
	{ 0, 0, 1, TRUE , 1, 1, 1 },
	{ 0, 1, 0, NONE , 0, 0, 0 },
	{ 0, 1, 0, NONE , 0, 1, 0 },
	{ 0, 1, 0, NONE , 1, 0, 0 },
	{ 0, 1, 0, NONE , 1, 1, 0 },
	{ 0, 1, 0, FALSE, 0, 0, 0 },
	{ 0, 1, 0, FALSE, 0, 1, 0 },
	{ 0, 1, 0, FALSE, 1, 0, 0 },
	{ 0, 1, 0, FALSE, 1, 1, 0 },
	{ 0, 1, 0, TRUE , 0, 0, 1 },
	{ 0, 1, 0, TRUE , 0, 1, 1 },
	{ 0, 1, 0, TRUE , 1, 0, 1 },
	{ 0, 1, 0, TRUE , 1, 1, 1 },
	{ 0, 1, 1, NONE , 0, 0, 1 },
	{ 0, 1, 1, NONE , 0, 1, 1 },
	{ 0, 1, 1, NONE , 1, 0, 1 },
	{ 0, 1, 1, NONE , 1, 1, 1 },
	{ 0, 1, 1, FALSE, 0, 0, 0 },
	{ 0, 1, 1, FALSE, 0, 1, 0 },
	{ 0, 1, 1, FALSE, 1, 0, 0 },
	{ 0, 1, 1, FALSE, 1, 1, 0 },
	{ 0, 1, 1, TRUE , 0, 0, 1 },
	{ 0, 1, 1, TRUE , 0, 1, 1 },
	{ 0, 1, 1, TRUE , 1, 0, 1 },
	{ 0, 1, 1, TRUE , 1, 1, 1 },
	{ 1, 0, 0, NONE , 0, 0, 0 },
	{ 1, 0, 0, NONE , 0, 1, 0 },
	{ 1, 0, 0, NONE , 1, 0, 0 },
	{ 1, 0, 0, NONE , 1, 1, 0 },
	{ 1, 0, 0, FALSE, 0, 0, 0 },
	{ 1, 0, 0, FALSE, 0, 1, 0 },
	{ 1, 0, 0, FALSE, 1, 0, 0 },
	{ 1, 0, 0, FALSE, 1, 1, 0 },
	{ 1, 0, 0, TRUE , 0, 0, 0 },
	{ 1, 0, 0, TRUE , 0, 1, 0 },
	{ 1, 0, 0, TRUE , 1, 0, 0 },
	{ 1, 0, 0, TRUE , 1, 1, 0 },
	{ 1, 0, 1, NONE , 0, 0, 0 },
	{ 1, 0, 1, NONE , 0, 1, 0 },
	{ 1, 0, 1, NONE , 1, 0, 0 },
	{ 1, 0, 1, NONE , 1, 1, 1 },
	{ 1, 0, 1, FALSE, 0, 0, 0 },
	{ 1, 0, 1, FALSE, 0, 1, 0 },
	{ 1, 0, 1, FALSE, 1, 0, 0 },
	{ 1, 0, 1, FALSE, 1, 1, 1 },
	{ 1, 0, 1, TRUE , 0, 0, 0 },
	{ 1, 0, 1, TRUE , 0, 1, 0 },
	{ 1, 0, 1, TRUE , 1, 0, 0 },
	{ 1, 0, 1, TRUE , 1, 1, 1 },
	{ 1, 1, 0, NONE , 0, 0, 1 },
	{ 1, 1, 0, NONE , 0, 1, 1 },
	{ 1, 1, 0, NONE , 1, 0, 1 },
	{ 1, 1, 0, NONE , 1, 1, 0 },
	{ 1, 1, 0, FALSE, 0, 0, 1 },
	{ 1, 1, 0, FALSE, 0, 1, 1 },
	{ 1, 1, 0, FALSE, 1, 0, 1 },
	{ 1, 1, 0, FALSE, 1, 1, 0 },
	{ 1, 1, 0, TRUE , 0, 0, 1 },
	{ 1, 1, 0, TRUE , 0, 1, 1 },
	{ 1, 1, 0, TRUE , 1, 0, 1 },
	{ 1, 1, 0, TRUE , 1, 1, 0 },
	{ 1, 1, 1, NONE , 0, 0, 1 },
	{ 1, 1, 1, NONE , 0, 1, 1 },
	{ 1, 1, 1, NONE , 1, 0, 1 },
	{ 1, 1, 1, NONE , 1, 1, 1 },
	{ 1, 1, 1, FALSE, 0, 0, 1 },
	{ 1, 1, 1, FALSE, 0, 1, 1 },
	{ 1, 1, 1, FALSE, 1, 0, 1 },
	{ 1, 1, 1, FALSE, 1, 1, 1 },
	{ 1, 1, 1, TRUE , 0, 0, 1 },
	{ 1, 1, 1, TRUE , 0, 1, 1 },
	{ 1, 1, 1, TRUE , 1, 0, 1 },
	{ 1, 1, 1, TRUE , 1, 1, 1 },
};
#define OTHER_BITS (FLAG(_network_game_cooperative_no_player_collisions_bit) | FLAG(_network_game_cooperative_no_ally_friendly_fire_bit) | FLAG(_network_game_cooperative_dedicated_bit))
#define RIDING FLAG(_network_game_cooperative_vehicle_riding_bit)
static struct game_variant variant_of(int engine, int riding)
{
	struct game_variant variant = { 0 };
	variant.game_engine_index = (short)engine;
	if (riding)
		variant.universal_variant.flags = FLAG(_game_variant_vehicle_riding_bit);
	return variant;
}
static void settings(int base, int override, int server, int entry)
{
	fake_base = base;
	fake_override = override;
	fake_server = server;
	network_game_port_vehicle_riding_entry((short)entry);
}
static int rides(struct network_game const *game) { return (game->cooperative_flags & RIDING) != 0; }
int main(int argc, char **argv)
{
	CHECK(argc == 2, "case");
	char const *case_name = argv[1];
	CASE("decide-grid") {
		for (int n = 0; n < 96; n++) {
			struct row const *r = &grid[n];
			boolean got = vehicle_riding_decide(r->multiplayer, r->gametype_rule, r->host_option, r->entry_rule, r->dedicated, r->override);
			CHECK(got == r->expected, "row %d: multiplayer %d gametype %d host %d entry %d dedicated %d override %d: got %d, expected %d",
				n, r->multiplayer, r->gametype_rule, r->host_option, r->entry_rule, r->dedicated, r->override, got, r->expected);
		}
	}
	else CASE("bit-six") {
		CHECK(_game_variant_vehicle_riding_bit == 6, "riding bit is not 6");
		CHECK(_game_variant_generic_starting_equipment_bit == 5, "starting equipment bit moved");
	}
	else CASE("start-coop-uses-host-option") {
		struct network_game game = { variant_of(game_engine_none, 1), 0 };
		settings(1, 0, 0, NONE);
		network_game_port_vehicle_riding_start(&game);
		CHECK(rides(&game), "co-op with the option on does not ride");
		settings(0, 0, 0, NONE);
		network_game_port_vehicle_riding_start(&game);
		CHECK(!rides(&game), "co-op with the option off rides on the gametype's bit");
	}
	else CASE("start-mp-uses-gametype") {
		struct network_game game = { variant_of(game_engine_slayer, 1), 0 };
		settings(1, 0, 0, NONE);
		network_game_port_vehicle_riding_start(&game);
		CHECK(rides(&game), "a riding slayer does not ride");
		game.variant = variant_of(game_engine_slayer, 0);
		network_game_port_vehicle_riding_start(&game);
		CHECK(!rides(&game), "a silent slayer rides on the host option");
	}
	else CASE("start-overwrites-stale-bit") {
		struct network_game game = { variant_of(game_engine_none, 0), 0 };
		settings(1, 0, 0, NONE);
		network_game_port_vehicle_riding_start(&game);
		CHECK(rides(&game), "co-op, option 1: not on");
		game.variant = variant_of(game_engine_slayer, 0);
		network_game_port_vehicle_riding_start(&game);
		CHECK(!rides(&game), "a silent slayer after co-op kept the co-op game's riding");
		game.variant = variant_of(game_engine_slayer, 1);
		network_game_port_vehicle_riding_start(&game);
		CHECK(rides(&game), "a riding slayer: not on");
		settings(0, 0, 0, NONE);
		game.variant = variant_of(game_engine_none, 0);
		network_game_port_vehicle_riding_start(&game);
		CHECK(!rides(&game), "co-op with option 0 after a riding slayer kept its riding");
		game.variant = variant_of(game_engine_slayer, 1);
		network_game_port_vehicle_riding_start(&game);
		CHECK(rides(&game), "a riding slayer again: not on");
		game.variant = variant_of(game_engine_slayer, 0);
		network_game_port_vehicle_riding_start(&game);
		CHECK(!rides(&game), "a silent slayer after a riding slayer kept its riding");
	}
	else CASE("start-dedicated-override") {
		struct network_game game = { variant_of(game_engine_slayer, 1), 0 };
		settings(0, 1, 1, NONE);
		network_game_port_vehicle_riding_start(&game);
		CHECK(!rides(&game), "override on, base 0: a riding gametype rides");
		game.variant = variant_of(game_engine_slayer, 0);
		settings(1, 1, 1, NONE);
		network_game_port_vehicle_riding_start(&game);
		CHECK(rides(&game), "override on, base 1: a silent gametype does not ride");
		settings(1, 0, 1, NONE);
		network_game_port_vehicle_riding_start(&game);
		CHECK(!rides(&game), "override off, base 1: a silent gametype rides");
		game.variant = variant_of(game_engine_none, 0);
		network_game_port_vehicle_riding_start(&game);
		CHECK(rides(&game), "override off, co-op, base 1: not on");
	}
	else CASE("start-listen-host-ignores-override") {
		struct network_game game = { variant_of(game_engine_slayer, 0), 0 };
		settings(1, 1, 0, NONE);
		network_game_port_vehicle_riding_start(&game);
		CHECK(!rides(&game), "a listen host's override took the base setting over a silent gametype");
	}
	else CASE("start-keeps-other-flags") {
		for (int riding = 0; riding < 2; riding++) {
			for (int others = 0; others < 2; others++) {
				struct network_game game = { variant_of(game_engine_slayer, riding), (byte)(others ? OTHER_BITS : 0) };
				settings(0, 0, 0, NONE);
				network_game_port_vehicle_riding_start(&game);
				CHECK((game.cooperative_flags & OTHER_BITS) == (others ? OTHER_BITS : 0), "riding %d others %d: bits 0-2 are now 0x%x",
					riding, others, game.cooperative_flags & OTHER_BITS);
				CHECK(rides(&game) == riding, "riding %d others %d: bit 3 is %d", riding, others, rides(&game));
			}
		}
	}
	else CASE("start-stock-flags-silent") {
		struct network_game game = { variant_of(game_engine_slayer, 0), 0 };
		game.variant.universal_variant.flags = FLAG(_game_variant_vehicle_riding_bit) - 1;
		settings(1, 0, 0, NONE);
		network_game_port_vehicle_riding_start(&game);
		CHECK(!rides(&game), "a gametype with bits 0-5 set rides");
	}
	else CASE("start-coop-entry-rule") {
		struct network_game game = { variant_of(game_engine_none, 0), 0 };
		settings(0, 0, 1, TRUE);
		network_game_port_vehicle_riding_start(&game);
		CHECK(rides(&game), "entry TRUE, base 0: not on");
		settings(1, 0, 1, FALSE);
		network_game_port_vehicle_riding_start(&game);
		CHECK(!rides(&game), "entry FALSE, base 1: rides");
		settings(1, 0, 1, NONE);
		network_game_port_vehicle_riding_start(&game);
		CHECK(rides(&game), "entry NONE, base 1: not on");
		settings(0, 0, 1, NONE);
		network_game_port_vehicle_riding_start(&game);
		CHECK(!rides(&game), "entry NONE, base 0: rides");
		settings(0, 0, 1, 7);
		network_game_port_vehicle_riding_start(&game);
		CHECK(rides(&game), "an entry rule other than NONE and FALSE is not taken as TRUE");
	}
	else CASE("start-entry-rule-multiplayer-ignored") {
		struct network_game game = { variant_of(game_engine_slayer, 0), 0 };
		settings(0, 0, 1, TRUE);
		network_game_port_vehicle_riding_start(&game);
		CHECK(!rides(&game), "a silent slayer rides on a co-op entry's rule");
		game.variant = variant_of(game_engine_slayer, 1);
		settings(0, 0, 1, FALSE);
		network_game_port_vehicle_riding_start(&game);
		CHECK(rides(&game), "a riding slayer is turned off by a co-op entry's rule");
	}
	else CASE("entry-rule-read") {
		struct server_entry entry;
		memset(&entry, 0, sizeof(entry));
		CHECK(server_entry_riding_rule(&entry) == NONE, "an entry with no rules gives %d", server_entry_riding_rule(&entry));
		strcpy(entry.rules[0].key, "lives");
		entry.rules[0].value = 3;
		strcpy(entry.rules[1].key, "vehicle_riding");
		entry.rules[1].value = 1;
		strcpy(entry.rules[2].key, "teams");
		entry.rules[2].value = 0;
		entry.rule_count = 3;
		CHECK(server_entry_riding_rule(&entry) == TRUE, "vehicle_riding 1 among other rules gives %d", server_entry_riding_rule(&entry));
		entry.rules[1].value = 0;
		CHECK(server_entry_riding_rule(&entry) == FALSE, "vehicle_riding 0 gives %d", server_entry_riding_rule(&entry));
		entry.rule_count = 1;
		strcpy(entry.rules[0].key, "radar");
		entry.rules[0].value = 1;
		CHECK(server_entry_riding_rule(&entry) == NONE, "an entry with only radar 1 gives %d", server_entry_riding_rule(&entry));
		strcpy(entry.rules[1].key, "vehicle_riding");
		entry.rules[1].value = 1;
		CHECK(server_entry_riding_rule(&entry) == NONE, "a rule past the count was read: %d", server_entry_riding_rule(&entry));
	}
	else CASE("entry-rule-not-inherited") {
		struct server_entry entry;
		struct network_game game = { variant_of(game_engine_none, 0), 0 };
		memset(&entry, 0, sizeof(entry));
		fake_override = 0;
		fake_server = 1;
		/* (what the director does for each entry it applies, then the start of a co-op game) */
		fake_base = 0;
		strcpy(entry.rules[0].key, "vehicle_riding");
		entry.rules[0].value = 1;
		entry.rule_count = 1;
		network_game_port_vehicle_riding_entry(server_entry_riding_rule(&entry));
		network_game_port_vehicle_riding_start(&game);
		CHECK(rides(&game), "entry 1, a10:coop+riding=true, base 0: not on");
		entry.rule_count = 0;
		network_game_port_vehicle_riding_entry(server_entry_riding_rule(&entry));
		network_game_port_vehicle_riding_start(&game);
		CHECK(!rides(&game), "entry 2, a10:coop with no rule, base 0: took entry 1's rule");
		fake_base = 1;
		entry.rules[0].value = 0;
		entry.rule_count = 1;
		network_game_port_vehicle_riding_entry(server_entry_riding_rule(&entry));
		network_game_port_vehicle_riding_start(&game);
		CHECK(!rides(&game), "entry 3, a10:coop+riding=false, base 1: rides");
		entry.rule_count = 0;
		network_game_port_vehicle_riding_entry(server_entry_riding_rule(&entry));
		network_game_port_vehicle_riding_start(&game);
		CHECK(rides(&game), "entry 4, a10:coop with no rule, base 1: took entry 3's rule");
	}
	else return 2;
	return 0;
}
