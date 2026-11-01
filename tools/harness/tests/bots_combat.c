#include "harness.h"
#include "bots_combat.h"
#include <math.h>
#include <float.h>
#define TICKS_PER_SECOND 30
#define REAL_MAX FLT_MAX
#define MIN(a,b) ((a)<(b)?(a):(b))
#define PIN(a,l,h) ((a)<(l)?(l):(a)>(h)?(h):(a))
#define NUMBEROF(a) (sizeof(a)/sizeof((a)[0]))
enum { _game_mode_swat = 1, _object_dead_bit = 0, _bots_goal_objective = 1 };
struct bots_bot_combat {boolean head_aim;long throw_until_tick;real throw_lead;};
struct bots_bot { struct bots_bot_combat combat; long player_index, aim_target, aim_until, guard_until_tick; unsigned long random;
    boolean aim_head, aim_feet; short goal_kind; real_point3d goal; };
struct player_datum { long unit_index; };
struct object_datum { long definition_index; struct { real_point3d position; real shield_vitality;
    unsigned long damage_flags; real_vector3d translational_velocity; } object; };
struct data_iterator { long datum_index; };
static struct player_datum players[3] = {{0},{1},{2}};
static struct object_datum units[3];
static int zombies[3], mode, scans, player_data, ground_valid = 1;
static long now = 100;
static char const *weapon_name = "weapons\\pistol\\pistol";
static real_point3d points[2] = {{10,0,0},{100,0,0}};
static struct { short skill;long combat_counts[1];struct bots_bot bots[1]; long roam_point_count; real_point3d *roam_points; } bots_globals;
static long game_time_get(void) { return now; }
static int game_modes_mode(void) { return mode; }
static struct player_datum *player_get(long i) { return &players[i]; }
static struct object_datum *object_get(long i) { return &units[i]; }
static struct object_datum *object_try_and_get(long i) { return i >= 0 && i < 3 ? &units[i] : NULL; }
static struct object_datum *weapon_get(long i) { (void)i; return &units[0]; }
static char const *tag_get_name(long i) { (void)i; return weapon_name; }
static void unit_get_head_position(long i, real_point3d *point) { *point = units[i].object.position; point->z += 1.5f; }
static long bots_surface_below(real_point3d const *point, real_point3d *ground)
{ *ground = *point; ground->z = 0; return ground_valid ? 0 : NONE; }
static boolean infection_unit_is_zombie(long i) { return zombies[i]; }
static void data_iterator_new(struct data_iterator *it, int data) { (void)data; it->datum_index = -1; scans++; }
static void *data_iterator_next(struct data_iterator *it) { return ++it->datum_index < 3 ? &players[it->datum_index] : NULL; }
static boolean bots_ctf_set_goal(struct bots_bot *bot, real_point3d const *point, real moved)
{ (void)moved; bot->goal = *point; bot->goal_kind = _bots_goal_objective; return TRUE; }
struct unit_datum {struct {real shield_vitality,body_vitality;} object;struct {unsigned long flags;} unit;};
enum {_equipment_powerup_health,_equipment_powerup_overshield,_equipment_powerup_active_camouflage,
    _unit_active_camouflaged_bit=0,game_engine_slayer=0};
static struct {struct {int powerup_type;} equipment;} equipment_definition;
#define equipment_definition_get(index) (&equipment_definition)
static struct {int game_engine_index;} variant;
#define game_engine_get_variant() (&variant)
static boolean bots_enhanced_running;
enum {_bots_combat_count_head_aims};
struct bots_tick {long weapon_index,now;struct object_datum *object;real_point3d eye;};
static struct {boolean precision;} weapon_kind={TRUE};
#define bots_weapon_kind(index) (&weapon_kind)
static struct unit_definition {struct {real grenade_velocity;} unit;} unit_definition={{18}};
#define unit_definition_get(index) (&unit_definition)
static real_point3d *bots_tick_eye(struct bots_tick *tick){return &tick->eye;}
#include "under_test.inc"

int main(int argc, char **argv)
{
    struct bots_bot *bot = &bots_globals.bots[0];
    real_point3d eye = {0,0,2}, aim = {5,0,1}, position = {0,0,0};
    unsigned long random;
    CHECK(argc == 2, "case");
    bot->aim_target = NONE;
    units[1].object.position.x = 5;
    units[2].object.position.x = 100;
    bots_globals.roam_point_count = 2; bots_globals.roam_points = points;
    if (!strcmp(argv[1], "skill-mapping"))
    {
        /* The pre-merge four named difficulties are a compatibility contract. */
        short levels[4] = {_bots_skill_easy,_bots_skill_normal,_bots_skill_heroic,_bots_skill_legendary};
        real expected[4][5] = {{.15f,0,.35f,0,15},{.40f,.4f,.50f,.25f,25},
            {.70f,.8f,.65f,.35f,30},{1,1,.80f,.45f,40}};
        int i;
        for(i=0;i<4;i++) {
            CHECK(enhanced_skills[levels[i]].head==expected[i][0], "named head chance shifted");
            CHECK(enhanced_skills[levels[i]].feet==expected[i][1], "named feet chance shifted");
            CHECK(enhanced_skills[levels[i]].heal==expected[i][2], "named heal threshold shifted");
            CHECK(enhanced_skills[levels[i]].heal_fighting==expected[i][3], "named combat heal threshold shifted");
            CHECK(enhanced_skills[levels[i]].powerup_reach==expected[i][4], "named powerup reach shifted");
        }
        for(i=0;i<NUMBER_OF_BOTS_SKILLS;i++) {
            static real heal[6]={.35f,.35f,.50f,.65f,.725f,.80f};
            static real fighting[6]={0,0,.25f,.35f,.40f,.45f};
            static real reach[6]={15,15,25,30,35,40};
            struct unit_datum unit={{0,0},{0}};
            bot->aim_until=0;bot->random=12345;
            enhanced_bots_aim(0,(short)i,1,0,&eye,&aim);
            CHECK(bot->aim_head==(i>=_bots_skill_normal), "aim consumer uses wrong row");
            CHECK(bot->aim_feet==(i>=_bots_skill_veteran), "feet consumer uses wrong row");
            unit.object.body_vitality=fighting[i]+.09f;
            CHECK(enhanced_bots_retreat(&unit,(short)i,now), "retreat threshold too low");
            unit.object.body_vitality=fighting[i]+.11f;
            CHECK(!enhanced_bots_retreat(&unit,(short)i,now), "retreat threshold too high");
            equipment_definition.equipment.powerup_type=_equipment_powerup_health;
            unit.object.body_vitality=heal[i]-.01f;
            CHECK(enhanced_bots_powerup_score(&unit,0,5,(short)i,FALSE,FALSE)<REAL_MAX,"heal threshold too low");
            unit.object.body_vitality=heal[i]+.01f;
            CHECK(enhanced_bots_powerup_score(&unit,0,5,(short)i,FALSE,FALSE)==REAL_MAX,"heal threshold too high");
            unit.object.body_vitality=fighting[i]-.01f;
            if(fighting[i]>0)CHECK(enhanced_bots_powerup_score(&unit,0,5,(short)i,TRUE,FALSE)<REAL_MAX,"combat heal threshold too low");
            unit.object.body_vitality=fighting[i]+.01f;
            CHECK(enhanced_bots_powerup_score(&unit,0,5,(short)i,TRUE,FALSE)==REAL_MAX,"combat heal threshold too high");
            equipment_definition.equipment.powerup_type=_equipment_powerup_overshield;
            CHECK(enhanced_bots_powerup_score(&unit,0,reach[i],(short)i,FALSE,FALSE)<REAL_MAX,"powerup reach too short");
            CHECK(enhanced_bots_powerup_score(&unit,0,reach[i]+1,(short)i,FALSE,FALSE)==REAL_MAX,"powerup reach too long");
        }
        CHECK(enhanced_skills[_bots_skill_recruit].head==enhanced_skills[_bots_skill_easy].head &&
            enhanced_skills[_bots_skill_recruit].powerup_reach==enhanced_skills[_bots_skill_easy].powerup_reach,
            "Recruit enhanced policy");
        CHECK(fabs(enhanced_skills[_bots_skill_veteran].head-.85f)<.0001f &&
            fabs(enhanced_skills[_bots_skill_veteran].feet-.9f)<.0001f &&
            fabs(enhanced_skills[_bots_skill_veteran].heal-.725f)<.0001f &&
            fabs(enhanced_skills[_bots_skill_veteran].heal_fighting-.4f)<.0001f &&
            enhanced_skills[_bots_skill_veteran].powerup_reach==35, "Veteran interpolation");
    }
    else if (!strcmp(argv[1], "aim-precedence"))
    {
        struct bots_tick tick={0,100,&units[0],{0,0,2}};
        bots_globals.skill=_bots_skill_normal;
        bots_level_aim_point(bot,&tick,1,&aim);
        CHECK(aim.z==1.5f,"base Normal head selection changed");
        bots_enhanced_running=TRUE;aim.z=1;
        bots_level_aim_point(bot,&tick,1,&aim);
        bot->random=1234567;bot->aim_until=0;
        enhanced_bots_aim(0,_bots_skill_normal,1,0,&eye,&aim);
        CHECK(!bot->aim_head&&aim.z==1,"base head aim bypassed enhanced chance");
        bot->random=12345;bot->aim_until=0;
        enhanced_bots_aim(0,_bots_skill_normal,1,0,&eye,&aim);
        CHECK(bot->aim_head&&aim.z==1.5f,"enhanced head choice lost");
        bot->combat.throw_until_tick=101;bot->combat.throw_lead=1;
        units[1].object.translational_velocity.i=.1f;aim.x=5;
        bots_level_aim_point(bot,&tick,1,&aim);
        CHECK(aim.x>5&&aim.x<=8.5f,"enhanced mode disabled bounded grenade lead");
    }
    else if (!strcmp(argv[1], "precision") || !strcmp(argv[1], "swat"))
    {
        if (!strcmp(argv[1], "swat")) { mode = _game_mode_swat; weapon_name = "assault rifle"; }
        else units[1].object.shield_vitality = 1;
        enhanced_bots_aim(0, _bots_skill_legendary, 1, 0, &eye, &aim);
        CHECK(aim.z == (mode ? 1.5f : 1.0f), "shield/SWAT aim rule");
        random = bot->random;
        units[1].object.shield_vitality = 0;
        enhanced_bots_aim(0, _bots_skill_legendary, 1, 0, &eye, &aim);
        CHECK(aim.z == 1.5f && random == bot->random, "head choice rerolled during burst");
    }
    else if (!strcmp(argv[1], "explosive"))
    {
        weapon_name = "rocket launcher";
        enhanced_bots_aim(0, _bots_skill_legendary, 1, 0, &eye, &aim);
        CHECK(aim.z == 0.1f, "grounded rocket target not aimed at feet");
        units[1].object.position.z = 3; aim.z = 1;
        enhanced_bots_aim(0, _bots_skill_legendary, 1, 0, &eye, &aim);
        CHECK(aim.z == 1, "airborne target aimed at ground");
        units[1].object.position.z = 0; ground_valid = 0;
        enhanced_bots_aim(0, _bots_skill_legendary, 1, 0, &eye, &aim);
        CHECK(aim.z == 1, "missing ground accepted");
    }
    else if (!strcmp(argv[1], "infection"))
    {
        zombies[0] = 1;
        CHECK(bots_infection_goal(bot, &players[0], &position) && bot->goal.x == 5, "zombie ignored nearest human");
        bots_infection_goal(bot, &players[0], &position);
        CHECK(scans == 1, "role snapshot rescanned within tick");
        zombies[1] = 1; now += 30;
        CHECK(bots_infection_goal(bot, &players[0], &position) && bot->goal.x == 100, "converted target retained");
        zombies[0] = 0; bot->goal_kind = 0; now += 30;
        CHECK(bots_infection_goal(bot, &players[0], &position) && bot->goal.x == 100, "human failed to regroup");
    }
    else if (!strcmp(argv[1], "alone"))
    {
        zombies[1] = zombies[2] = 1; units[2].object.position.x = 6;
        CHECK(bots_infection_goal(bot, &players[0], &position) && bot->goal.x == 100, "last human approached zombies");
    }
    else CHECK(0, "case");
    return 0;
}
