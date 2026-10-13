#ifndef GUN_RACE_H
#define GUN_RACE_H

/* Gun Race Pro rules. The host glue below the GUN_RACE_RULES_ONLY guard
   is compiled into the game; the rule core compiles alone. */

#define GR_MAX_PLAYERS 128
#define GR_MAX_STAGES 11
#define GR_KILLS_PER_GUN 5
#define GR_FINAL_MELEE_KILLS 10

enum
{
	GR_FLAMETHROWER = 0,
	GR_FUEL_ROD,
	GR_ROCKET_LAUNCHER,
	GR_SNIPER,
	GR_SHOTGUN,
	GR_NEEDLER,
	GR_ASSAULT_RIFLE,
	GR_PLASMA_RIFLE,
	GR_PISTOL,
	GR_PLASMA_PISTOL,
	GR_UNARMED,
	GR_WEAPON_COUNT,
	GR_WEAPON_INVALID = GR_WEAPON_COUNT
};

enum
{
	GR_KILL_IGNORED = 0,
	GR_KILL_COUNTED,
	GR_STAGE_ADVANCED,
	GR_MATCH_WON
};

struct gr_player
{
	int connected;
	int finished;
	int valid_kills;
	int stage;
	int stage_kills;
};

struct gr_match
{
	int enabled;
	int match_finished;
	int winner_slot;
	int stage_count;
	int stages[GR_MAX_STAGES];
	struct gr_player players[GR_MAX_PLAYERS];
};

void gr_match_init(struct gr_match *match);
int gr_match_add_stage(struct gr_match *match, int weapon);
int gr_player_join(struct gr_match *match, int slot);
void gr_player_leave(struct gr_match *match, int slot);
int gr_current_weapon(const struct gr_match *match, int slot);
int gr_kills_needed(const struct gr_match *match, int slot);
int gr_kills_to_win(const struct gr_match *match);
int gr_host_kill(struct gr_match *match, int attacker_slot, int victim_slot,
	int event_weapon, int is_melee);
int gr_allow_world_weapon_pickup(const struct gr_match *match);
int gr_allow_grenade_use(const struct gr_match *match);
int gr_unarmed_melee_should_be_lethal(const struct gr_match *match,
	int attacker_slot, int is_melee);
const char *gr_weapon_name(int weapon);

#ifndef GUN_RACE_RULES_ONLY

long gun_race_active(void);
long gun_race_show_scoreboard(void);
void gun_race_on_new_map(void);
void gun_race_on_player_added(long player_index);
void gun_race_on_player_spawn(long player_index);
void gun_race_enforce_player(long player_index);
long gun_race_forbids_weapon_pickup(void);
long gun_race_forbids_grenades(void);
int gun_race_handle_kill(long killing_player_index, long dead_player_index);
void gun_race_format_progress(long player_index, long replicated_kills,
	wchar_t *buffer, long buffer_count);
/* A client's hit, dealt on the host. Melee is the unarmed stage. */
int gun_race_begin_reported_hit(long player_index, long damage_definition_index, int is_melee);
long gun_race_reported_melee_should_kill(long player_index);
void gun_race_push_pending_projectile(long projectile_definition_index);
void gun_race_pop_pending_projectile(void);
void gun_race_bind_effect(long effect_index);
long gun_race_effect_projectile(long effect_index);
void gun_race_begin_projectile(long projectile_definition_index);
void gun_race_begin_melee(void);
void gun_race_end_damage(void);
long gun_race_melee_should_kill(long attacker_unit_index);
void gun_race_observe_score(long player_index, long total_kills);
long gun_race_mark_on_radar(long unit_index);

#endif

#endif
