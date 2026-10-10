/* MCC campaign operations. No native function/global table is extended or
 * replaced. State belongs to MCC snapshots, never the Xbox/CE tag data. */
#include "mcc_campaign.h"
#include "mcc_cache.h"
#include "errors.h"
#include "cache/sound_cache.h"
#include "sound/sound_definitions.h"
#include "hs/hs.h"
#include "hs/hs_scenario_definitions.h"
#include "hs/object_lists.h"
#include "game/game.h"
#include "game/cheats.h"
#include "game/players.h"
#include "interface/hud.h"
#include "interface/hud_definitions.h"
#include "memory/data.h"
#include "objects/objects.h"
#include "physics/physics.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "units/units.h"
#include <stdint.h>
#include <string.h>

enum { MCC_NAV_POSITION=1, MCC_NAV_FLAG, MCC_NAV_OBJECT, MCC_NAV_LIMIT=8 };
struct mcc_campaign_nav {
    short kind, team, arrow, reserved;
    long reference;
    char name[64];
    real_point3d position;
    real offset;
};
struct mcc_campaign_state {
    uint32_t version;
    real gravity;
    real dialog_gain;
    uint32_t flags;
    struct mcc_campaign_nav nav[MCC_NAV_LIMIT];
};
typedef char mcc_campaign_state_size[sizeof(struct mcc_campaign_state)==MCC_CAMPAIGN_SNAPSHOT_BYTES ? 1 : -1];
static struct mcc_campaign_state mcc_campaign;
static real mcc_original_gravity;
static real mcc_original_dialog_gain;
static boolean mcc_original_infinite_ammo;
static boolean mcc_campaign_owned;
extern real sound_gain_under_dialog;
extern short const hs_external_global_count;

boolean mcc_campaign_global_settable(short index)
{
    char const *name;
    if (!mcc_cache_tags_loaded() || index<0 || index>=hs_external_global_count) return FALSE;
    name=hs_global_external_get(index)->name;
    return !csstrcasecmp(name,"sound_gain_under_dialog") || !csstrcasecmp(name,"cheat_infinite_ammo");
}

static int mcc_campaign_real(real value) { return value>=-1000000.0f && value<=1000000.0f; }
static real mcc_campaign_argument(long value)
{
    union { long bits; real number; } result;
    result.bits=value; return result.number;
}

void mcc_campaign_begin(void)
{
    if (!mcc_campaign_owned) {
        mcc_original_gravity=global_gravity;mcc_original_dialog_gain=sound_gain_under_dialog;mcc_campaign_owned=TRUE;
        mcc_original_infinite_ammo=cheat.infinite_ammo;
    }
    memset(&mcc_campaign,0,sizeof(mcc_campaign));
    mcc_campaign.version=1; mcc_campaign.gravity=1.0f;
    mcc_campaign.dialog_gain=mcc_original_dialog_gain;
    global_gravity=mcc_original_gravity;
    sound_gain_under_dialog=mcc_original_dialog_gain;
    cheat.infinite_ammo=mcc_original_infinite_ammo;
}

void mcc_campaign_dispose(void)
{
    if (mcc_campaign_owned) {
        global_gravity=mcc_original_gravity;sound_gain_under_dialog=mcc_original_dialog_gain;
        cheat.infinite_ammo=mcc_original_infinite_ammo;
    }
    mcc_campaign_owned=FALSE;
    memset(&mcc_campaign,0,sizeof(mcc_campaign));
}

void mcc_campaign_snapshot(void *out)
{
    mcc_campaign.dialog_gain=sound_gain_under_dialog;
    mcc_campaign.flags=cheat.infinite_ammo ? 1u : 0u;
    memcpy(out,&mcc_campaign,sizeof(mcc_campaign));
}

int mcc_campaign_validate(void const *in, unsigned long bytes)
{
    struct mcc_campaign_state state;
    int i,j;
    if (!in || bytes!=sizeof(state)) return 0;
    memcpy(&state,in,sizeof(state));
    if (state.version!=1 || (state.flags&~1u) || !mcc_campaign_real(state.dialog_gain) ||
        !(state.gravity>=-1000.0f && state.gravity<=1000.0f)) return 0;
    for (i=0;i<MCC_NAV_LIMIT;i++) {
        struct mcc_campaign_nav const *nav=&state.nav[i];
        if (nav->kind<0 || nav->kind>MCC_NAV_OBJECT || nav->reserved ||
            !mcc_campaign_real(nav->position.x) || !mcc_campaign_real(nav->position.y) ||
            !mcc_campaign_real(nav->position.z) || !mcc_campaign_real(nav->offset) ||
            !memchr(nav->name,0,sizeof(nav->name))) return 0;
        if (!nav->kind) continue;
        if (nav->team<0 || nav->team>=10 || nav->arrow<0) return 0;
        if (hud_globals && nav->arrow>=hud_globals->waypoint.arrows.count) return 0;
        if (nav->kind==MCC_NAV_FLAG && (nav->reference<0 ||
            nav->reference>=global_scenario_get()->cutscene_flags.count)) return 0;
        if (nav->kind==MCC_NAV_OBJECT && nav->reference==NONE) return 0;
        for (j=0;j<i;j++) {
            struct mcc_campaign_nav const *other=&state.nav[j];
            if (nav->kind==other->kind && nav->team==other->team &&
                (nav->kind==MCC_NAV_POSITION ? !strcmp(nav->name,other->name) : nav->reference==other->reference)) return 0;
        }
    }
    return 1;
}

int mcc_campaign_restore(void const *in, unsigned long bytes)
{
    if (!mcc_cache_tags_loaded() || !mcc_campaign_owned || !mcc_campaign_validate(in,bytes)) return 0;
    memcpy(&mcc_campaign,in,sizeof(mcc_campaign));
    global_gravity=mcc_original_gravity*mcc_campaign.gravity;
    sound_gain_under_dialog=mcc_campaign.dialog_gain;
    /* The native client cheat policy remains authoritative. Only the host
     * runs campaign scripts and decides ammunition consumption. */
    if (game_connection()!=_game_connection_network_client) cheat.infinite_ammo=(mcc_campaign.flags&1u)!=0;
    return 1;
}

static void mcc_campaign_nav_change(short kind, short team, short arrow, long reference,
    char const *name, real_point3d const *position, real offset)
{
    int i,slot=-1;
    if (team<0 || team>=10 || !mcc_campaign_real(offset) ||
        (kind==MCC_NAV_POSITION && (!name || strlen(name)>=64 || !position ||
        !mcc_campaign_real(position->x) || !mcc_campaign_real(position->y) || !mcc_campaign_real(position->z)))) return;
    for (i=0;i<MCC_NAV_LIMIT;i++) {
        struct mcc_campaign_nav *nav=&mcc_campaign.nav[i];
        if (!nav->kind) { if (slot<0) slot=i; continue; }
        if (nav->kind==kind && nav->team==team &&
            (kind==MCC_NAV_POSITION ? !strcmp(nav->name,name) : nav->reference==reference)) { slot=i; break; }
    }
    if (slot<0) { error(_error_silent,"mcc: campaign navigation point capacity reached"); return; }
    if (arrow!=NONE && (!hud_globals || arrow<0 || arrow>=hud_globals->waypoint.arrows.count ||
        (kind==MCC_NAV_FLAG && (reference<0 || reference>=global_scenario_get()->cutscene_flags.count)) ||
        (kind==MCC_NAV_OBJECT && !object_try_and_get(reference)))) return;
    memset(&mcc_campaign.nav[slot],0,sizeof(mcc_campaign.nav[slot]));
    if (arrow!=NONE) {
        struct mcc_campaign_nav *nav=&mcc_campaign.nav[slot];
        nav->kind=kind;nav->team=team;nav->arrow=arrow;nav->reference=reference;nav->offset=offset;
        if (position) nav->position=*position;
        if (name) memcpy(nav->name,name,strlen(name)+1);
    }
}

void mcc_campaign_render(short local_player)
{
    struct player_datum *player;
    long player_index;
    real_point3d head;
    int i;
    if (!mcc_cache_tags_loaded() || !mcc_campaign_owned || local_player<0 ||
        local_player>=MAXIMUM_NUMBER_OF_LOCAL_PLAYERS || !hud_globals ||
        hud_globals->waypoint.arrow_bitmap.index==NONE) return;
    player_index=local_player_get_player_index(local_player);
    player=player_index==NONE ? NULL : player_try_and_get(player_index);
    if (!player || !unit_try_and_get(player->unit_index)) return;
    unit_get_head_position(player->unit_index,&head);
    for (i=0;i<MCC_NAV_LIMIT;i++) {
        struct mcc_campaign_nav const *nav=&mcc_campaign.nav[i];
        real_point3d position=nav->position;
        long reference=NONE;
        short screen;
        if (!nav->kind || nav->team!=player->team_index || nav->arrow>=hud_globals->waypoint.arrows.count) continue;
        if (nav->kind==MCC_NAV_FLAG) {
            struct scenario *scenario=global_scenario_get();
            if (nav->reference<0 || nav->reference>=scenario->cutscene_flags.count) continue;
            position=((struct scenario_cutscene_flag *)scenario->cutscene_flags.address)[nav->reference].position;
        } else if (nav->kind==MCC_NAV_OBJECT) {
            real radius;
            struct object_datum *object=object_try_and_get(nav->reference);
            if (!object || TEST_FLAG(object->object.damage_flags,_object_dead_bit)) continue;
            reference=nav->reference;
            object_get_bounding_sphere(reference,&position,&radius);
        }
        position.z+=nav->offset;
        screen=hud_get_nav_point_render_type(local_player,&head,&position,reference);
        custom_render_nav_point(local_player,&position,nav->arrow,screen);
    }
}

static void mcc_local_players_evaluate(short function, long thread, boolean initialize)
{
    long list=object_list_new();
    short local;
    (void)function;(void)initialize;
    if (list!=NONE) for (local=0;local<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS;local++) {
        long index=local_player_get_player_index(local);
        struct player_datum *player=index==NONE ? NULL : player_try_and_get(index);
        struct unit_datum *unit=player ? unit_try_and_get(player->unit_index) : NULL;
        if (unit && !TEST_FLAG(unit->object.damage_flags,_object_dead_bit)) object_list_add(list,player->unit_index);
    }
    hs_return(thread,list);
}

static void mcc_authority_evaluate(short function, long thread, boolean initialize)
{
    short connection=game_connection();
    (void)function;(void)initialize;
    hs_return(thread,connection==_game_connection_local || connection==_game_connection_network_server);
}

static void mcc_position_distance_evaluate(short function, long thread, boolean initialize)
{
    long *args=hs_macro_function_evaluate(function,thread,initialize),object,cursor;
    real_point3d target,origin;
    union { real number; long bits; } result;
    if (!args) return;
    result.number=-1.0f;
    if (function==MCC_HS_CAMPAIGN_FIRST+21) {
        struct scenario *scenario=global_scenario_get();
        short flag=(short)args[1];
        if (flag<0 || flag>=scenario->cutscene_flags.count) {hs_return(thread,result.bits);return;}
        target=((struct scenario_cutscene_flag *)scenario->cutscene_flags.address)[flag].position;
    } else {
        target.x=mcc_campaign_argument(args[1]);target.y=mcc_campaign_argument(args[2]);target.z=mcc_campaign_argument(args[3]);
    }
    if (args[0]!=NONE && datum_try_and_get(object_list_header_data,args[0]) &&
        mcc_campaign_real(target.x) && mcc_campaign_real(target.y) && mcc_campaign_real(target.z)) {
        for (object=object_list_get_first(args[0],&cursor);object!=NONE;object=object_list_get_next(args[0],&cursor)) {
            if (object_try_and_get(object)) {
                real distance;
                object_get_origin(object,&origin);distance=distance3d(&origin,&target);
                if (result.number<0 || distance<result.number) result.number=distance;
            }
        }
    }
    hs_return(thread,result.bits);
}

static void mcc_gravity_evaluate(short function, long thread, boolean initialize)
{
    long *args=hs_macro_function_evaluate(function,thread,initialize);
    if (args) {
        real scale=mcc_campaign_argument(args[0]);
        if (scale>=-1000.0f && scale<=1000.0f && mcc_campaign_owned && game_connection()!=_game_connection_network_client) {
            mcc_campaign.gravity=scale;global_gravity=mcc_original_gravity*scale;
        }
        hs_return(thread,0);
    }
}

static void mcc_physics_reset_evaluate(short function, long thread, boolean initialize)
{
    (void)function;(void)initialize;
    if (mcc_campaign_owned && game_connection()!=_game_connection_network_client) {
        mcc_campaign.gravity=1.0f;global_gravity=mcc_original_gravity;
    }
    hs_return(thread,0);
}

static void mcc_breadcrumbs_active_evaluate(short function, long thread, boolean initialize)
{
    (void)function;(void)initialize;
    /* This is feature availability, not whether a marker currently exists. */
    hs_return(thread,TRUE);
}

/* MCC integer operators use 32-bit values, including a zero-filling right
 * shift. Use unsigned operations so high bits and overflow are defined on
 * both native x86 and the Android guest. Nonnegative counts wrap at 32;
 * negative counts produce an empty mask/result. */
static void mcc_bits_evaluate(short function, long thread, boolean initialize)
{
    long *args=hs_macro_function_evaluate(function,thread,initialize);
    uint32_t value,mask,result;
    short shift;
    if (!args) return;
    value=(uint32_t)args[0];shift=(short)args[1];
    mask=shift<0 ? 0u : (UINT32_C(1) << ((unsigned)shift&31u));
    switch (function-MCC_HS_CAMPAIGN_FIRST) {
    case 13: result=value&(uint32_t)args[1];break;
    case 14: result=value|(uint32_t)args[1];break;
    case 15: result=shift<0 ? 0u : value << ((unsigned)shift&31u);break;
    case 16: result=shift<0 ? 0u : value >> ((unsigned)shift&31u);break;
    case 17: result=(value&mask)!=0;break;
    case 18: result=(boolean)args[2] ? value|mask : value&~mask;break;
    default: result=0;break;
    }
    hs_return(thread,(long)result);
}

static void mcc_print_if_evaluate(short function, long thread, boolean initialize)
{
    long *args=hs_macro_function_evaluate(function,thread,initialize);
    void hs_print(char const *message);
    if (!args) return;
    if ((boolean)args[0] && args[1]) hs_print((char const *)args[1]);
    hs_return(thread,0);
}

static void mcc_impulse_predict_evaluate(short function, long thread, boolean initialize)
{
    long *args=hs_macro_function_evaluate(function,thread,initialize);
    struct sound_definition *sound;
    long range,permutation;
    if (!args) return;
    /* Preload without creating a playing sound, acquiring references or
     * choosing a random permutation. The boolean requests a blocking load. */
    if (args[0]!=NONE) {
        sound=sound_definition_get(args[0]);
        for (range=0;range<sound->pitch_ranges.count;range++) {
            struct sound_pitch_range *pitch=(struct sound_pitch_range *)sound->pitch_ranges.address+range;
            for (permutation=0;permutation<pitch->permutations.count;permutation++)
                _sound_cache_sound_request((struct sound_permutation *)pitch->permutations.address+permutation,
                    (boolean)args[1],TRUE,FALSE);
        }
    }
    hs_return(thread,0);
}

static void mcc_nav_evaluate(short function, long thread, boolean initialize);
static boolean mcc_sleep_parse(short function, long expression)
{
    extern struct data_array *hs_syntax_data;
    struct hs_syntax_node *call=datum_try_and_get(hs_syntax_data,expression),*predicate,*argument;
    (void)function;
    if (!call || !(predicate=datum_try_and_get(hs_syntax_data,call->data))) return FALSE;
    if (predicate->next_node_index==NONE) return TRUE;
    argument=datum_try_and_get(hs_syntax_data,predicate->next_node_index);
    return argument && argument->next_node_index==NONE && hs_parse(predicate->next_node_index,_hs_type_script);
}

boolean hs_macro_function_parse(short function,long expression);
struct mcc_campaign_definition { struct hs_function_definition function;short rest[6]; };
static struct mcc_campaign_definition mcc_campaign_functions[]={
    {{_hs_type_object_list,0,"local_players",hs_macro_function_parse,mcc_local_players_evaluate,NULL,NULL,0,{0}},{0}},
    {{_hs_type_boolean,0,"game_is_authoritative",hs_macro_function_parse,mcc_authority_evaluate,NULL,NULL,0,{0}},{0}},
    {{_hs_type_void,0,"sleep_forever",mcc_sleep_parse,mcc_sleep_forever_evaluate,NULL,NULL,0,{0}},{0}},
    {{_hs_type_real,0,"objects_distance_to_position",hs_macro_function_parse,mcc_position_distance_evaluate,NULL,NULL,4,{_hs_type_object_list}},{_hs_type_real,_hs_type_real,_hs_type_real}},
    {{_hs_type_void,0,"physics_set_gravity",hs_macro_function_parse,mcc_gravity_evaluate,NULL,NULL,1,{_hs_type_real}},{0}},
    {{_hs_type_void,0,"physics_constants_reset",hs_macro_function_parse,mcc_physics_reset_evaluate,NULL,NULL,0,{0}},{0}},
    {{_hs_type_boolean,0,"breadcrumbs_nav_points_active",hs_macro_function_parse,mcc_breadcrumbs_active_evaluate,NULL,NULL,0,{0}},{0}},
    {{_hs_type_void,0,"breadcrumbs_activate_team_nav_point_position",hs_macro_function_parse,mcc_nav_evaluate,NULL,NULL,7,{_hs_type_navpoint}},{_hs_type_enum_team,_hs_type_real,_hs_type_real,_hs_type_real,_hs_type_string,_hs_type_real}},
    {{_hs_type_void,0,"breadcrumbs_deactivate_team_nav_point",hs_macro_function_parse,mcc_nav_evaluate,NULL,NULL,2,{_hs_type_enum_team}},{_hs_type_string}},
    {{_hs_type_void,0,"breadcrumbs_activate_team_nav_point_flag",hs_macro_function_parse,mcc_nav_evaluate,NULL,NULL,4,{_hs_type_navpoint}},{_hs_type_enum_team,_hs_type_cutscene_flag,_hs_type_real}},
    {{_hs_type_void,0,"breadcrumbs_deactivate_team_nav_point_flag",hs_macro_function_parse,mcc_nav_evaluate,NULL,NULL,2,{_hs_type_enum_team}},{_hs_type_cutscene_flag}},
    {{_hs_type_void,0,"breadcrumbs_activate_team_nav_point_object",hs_macro_function_parse,mcc_nav_evaluate,NULL,NULL,4,{_hs_type_navpoint}},{_hs_type_enum_team,_hs_type_object,_hs_type_real}},
    {{_hs_type_void,0,"breadcrumbs_deactivate_team_nav_point_object",hs_macro_function_parse,mcc_nav_evaluate,NULL,NULL,2,{_hs_type_enum_team}},{_hs_type_object}},
    {{_hs_type_long_integer,0,"bitwise_and",hs_macro_function_parse,mcc_bits_evaluate,NULL,NULL,2,{_hs_type_long_integer}},{_hs_type_long_integer}},
    {{_hs_type_long_integer,0,"bitwise_or",hs_macro_function_parse,mcc_bits_evaluate,NULL,NULL,2,{_hs_type_long_integer}},{_hs_type_long_integer}},
    {{_hs_type_long_integer,0,"bitwise_left_shift",hs_macro_function_parse,mcc_bits_evaluate,NULL,NULL,2,{_hs_type_long_integer}},{_hs_type_short_integer}},
    {{_hs_type_long_integer,0,"bitwise_right_shift",hs_macro_function_parse,mcc_bits_evaluate,NULL,NULL,2,{_hs_type_long_integer}},{_hs_type_short_integer}},
    {{_hs_type_long_integer,0,"bit_test",hs_macro_function_parse,mcc_bits_evaluate,NULL,NULL,2,{_hs_type_long_integer}},{_hs_type_short_integer}},
    {{_hs_type_long_integer,0,"bit_toggle",hs_macro_function_parse,mcc_bits_evaluate,NULL,NULL,3,{_hs_type_long_integer}},{_hs_type_short_integer,_hs_type_boolean}},
    {{_hs_type_void,0,"print_if",hs_macro_function_parse,mcc_print_if_evaluate,NULL,NULL,2,{_hs_type_boolean}},{_hs_type_string}},
    {{_hs_type_void,0,"sound_impulse_predict",hs_macro_function_parse,mcc_impulse_predict_evaluate,NULL,NULL,2,{_hs_type_sound}},{_hs_type_boolean}},
    {{_hs_type_real,0,"objects_distance_to_flag",hs_macro_function_parse,mcc_position_distance_evaluate,NULL,NULL,2,{_hs_type_object_list}},{_hs_type_cutscene_flag}}
};

struct hs_function_definition *mcc_campaign_function(short index)
{
    index=(short)(index-MCC_HS_CAMPAIGN_FIRST);
    return index>=0 && index<NUMBEROF(mcc_campaign_functions) ? &mcc_campaign_functions[index].function : NULL;
}
short mcc_campaign_find(char const *name)
{
    short i;
    for (i=0;i<NUMBEROF(mcc_campaign_functions);i++)
        if (!csstrcasecmp(name,mcc_campaign_functions[i].function.name)) return (short)(MCC_HS_CAMPAIGN_FIRST+i);
    return NONE;
}

static void mcc_nav_evaluate(short function, long thread, boolean initialize)
{
    long *args=hs_macro_function_evaluate(function,thread,initialize);
    if (!args) return;
    if (mcc_campaign_owned && game_connection()!=_game_connection_network_client) switch (function-MCC_HS_CAMPAIGN_FIRST) {
    case 7: {
        real_point3d position;
        position.x=mcc_campaign_argument(args[2]);position.y=mcc_campaign_argument(args[3]);position.z=mcc_campaign_argument(args[4]);
        mcc_campaign_nav_change(MCC_NAV_POSITION,(short)args[1],(short)args[0],NONE,(char const *)args[5],&position,mcc_campaign_argument(args[6]));break;
    }
    case 8: { real_point3d unused={0};mcc_campaign_nav_change(MCC_NAV_POSITION,(short)args[0],NONE,NONE,(char const *)args[1],&unused,0);break; }
    case 9: mcc_campaign_nav_change(MCC_NAV_FLAG,(short)args[1],(short)args[0],(short)args[2],NULL,NULL,mcc_campaign_argument(args[3]));break;
    case 10: mcc_campaign_nav_change(MCC_NAV_FLAG,(short)args[0],NONE,(short)args[1],NULL,NULL,0);break;
    case 11: mcc_campaign_nav_change(MCC_NAV_OBJECT,(short)args[1],(short)args[0],args[2],NULL,NULL,mcc_campaign_argument(args[3]));break;
    case 12: mcc_campaign_nav_change(MCC_NAV_OBJECT,(short)args[0],NONE,args[1],NULL,NULL,0);break;
    }
    hs_return(thread,0);
}
