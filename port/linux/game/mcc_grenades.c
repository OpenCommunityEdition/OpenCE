/* MCC adds two grenade/ability slots without changing the native unit or
 * save-game layout. Every extra count belongs to a full salted object ID. */
#include "cseries.h"
#include "errors.h"
#include "mcc_cache.h"
#include "mcc_grenades.h"
#include "ai/actors.h"
#include "game/cheats.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/game_globals.h"
#include "game/players.h"
#include "interface/hud_draw.h"
#include "interface/unit_hud_interface_definition.h"
#include "items/equipment.h"
#include "items/equipment_definitions.h"
#include "items/items.h"
#include "items/weapons.h"
#include "objects/objects.h"
#include "scenario/scenario.h"
#include "units/units.h"
#include "units/unit_definitions.h"
#include <string.h>

struct mcc_grenade_inventory {long handle;unsigned char counts[2];};
static struct mcc_grenade_inventory mcc_grenade_inventories[MAXIMUM_OBJECTS_PER_MAP];
static long mcc_grenade_flash_time[MAXIMUM_NUMBER_OF_LOCAL_PLAYERS];

boolean network_game_distributed_client(void);
void network_damage_note_grenade(long unit,short type);

boolean mcc_grenades_active(void) {return mcc_cache_tags_loaded();}

short mcc_grenades_type_count(void)
{
    struct game_globals *globals;
    if (!mcc_grenades_active()) return 0;
    globals=scenario_get_game_globals();
    return globals ? (short)PIN(globals->grenades.count,0,4) : 0;
}

static struct game_globals_grenade *mcc_grenade_definition(short type)
{
    if (type<0 || type>=mcc_grenades_type_count()) return NULL;
    return (struct game_globals_grenade *)scenario_get_game_globals()->grenades.address+type;
}

static struct mcc_grenade_inventory *mcc_grenade_inventory(long unit,int create)
{
    unsigned index=(unsigned long)unit&0xFFFF;
    struct mcc_grenade_inventory *inventory;
    if (unit==NONE || index>=MAXIMUM_OBJECTS_PER_MAP || !unit_try_and_get(unit)) return NULL;
    inventory=&mcc_grenade_inventories[index];
    if (inventory->handle!=unit) {
        if (!create) return NULL;
        inventory->handle=unit;inventory->counts[0]=inventory->counts[1]=0;
    }
    return inventory;
}

void mcc_grenades_reset(void)
{
    unsigned i;
    memset(mcc_grenade_inventories,0,sizeof(mcc_grenade_inventories));
    for (i=0;i<NUMBEROF(mcc_grenade_flash_time);i++) mcc_grenade_flash_time[i]=NONE;
}

void mcc_grenades_clear(long unit)
{
    unsigned index=(unsigned long)unit&0xFFFF;
    if (index<MAXIMUM_OBJECTS_PER_MAP) {
        mcc_grenade_inventories[index].handle=unit;
        mcc_grenade_inventories[index].counts[0]=mcc_grenade_inventories[index].counts[1]=0;
    }
}

short mcc_grenades_get(long unit,short type)
{
    struct unit_datum *object;
    struct mcc_grenade_inventory *inventory;
    if (!mcc_grenades_active() || type<0 || type>=mcc_grenades_type_count() || !(object=unit_try_and_get(unit))) return 0;
    if (type<2) return MAX(object->unit.grenade_counts[type],0);
    inventory=mcc_grenade_inventory(unit,0);
    return inventory ? inventory->counts[type-2] : 0;
}

void mcc_grenades_set(long unit,short type,short count)
{
    struct unit_datum *object;
    struct mcc_grenade_inventory *inventory;
    if (!mcc_grenades_active() || type<0 || type>=mcc_grenades_type_count() || !(object=unit_try_and_get(unit))) return;
    count=(short)PIN(count,0,127);
    if (type<2) object->unit.grenade_counts[type]=(char)count;
    else if ((inventory=mcc_grenade_inventory(unit,1))!=NULL) inventory->counts[type-2]=(unsigned char)count;
}

short mcc_grenades_total(long unit)
{
    short n,total=0;
    for (n=0;n<mcc_grenades_type_count();n++) total+=mcc_grenades_get(unit,n);
    return total;
}

short mcc_grenades_next(long unit,short current,short delta)
{
    short total=mcc_grenades_type_count(),n,index;
    if (!total) return NONE;
    if (current<0 || current>=total) current=0;
    for (n=0;n<total;n++) {
        short distance=(short)(n+(delta!=0));
        index=(short)((current+(delta<0 ? total-distance : distance))%total);
        if (mcc_grenades_get(unit,index)>0) return index;
    }
    return NONE;
}

short mcc_grenades_add(long unit,short type,short count)
{
    struct unit_datum *object=unit_try_and_get(unit);
    if (!object || !mcc_grenade_definition(type) || count<0) return 0;
    mcc_grenades_set(unit,type,(short)MIN(mcc_grenades_get(unit,type)+(long)count,127));
    object->unit.current_grenade_index=(char)type;
    object->unit.desired_grenade_index=(char)type;
    return mcc_grenades_get(unit,type);
}

void mcc_grenades_initialize_unit(long unit)
{
    struct unit_datum *object=unit_try_and_get(unit);
    struct unit_definition *definition;
    mcc_grenades_clear(unit);
    if (!object) return;
    definition=unit_definition_get(object->definition_index);
    if (definition->unit.grenade_type>=2)
        mcc_grenades_set(unit,definition->unit.grenade_type,definition->unit.grenade_count);
}

boolean mcc_grenades_pickup(long unit,long equipment)
{
    struct item_datum *item=equipment_get(equipment);
    struct equipment_definition *definition=equipment_definition_get(item->definition_index);
    short type=definition->equipment.grenade_type,count=mcc_grenades_get(unit,type);
    struct game_globals_grenade *grenade=mcc_grenade_definition(type);
    long player;
    if (definition->equipment.powerup_type!=_equipment_powerup_grenade || !grenade ||
        count>=MIN(grenade->maximum_count,127) || !unit_try_and_get(unit)) return FALSE;
    mcc_grenades_set(unit,type,count+1);
    player=player_index_from_unit_index(unit);
    if (player!=NONE && player_get(player)->local_player_index!=NONE) equipment_handle_pickup(equipment);
    object_delete(equipment);
    return TRUE;
}

void mcc_grenades_move_to_hand(long unit)
{
    struct unit_datum *object=unit_get(unit);
    short type=object->unit.current_grenade_index;
    struct game_globals_grenade *grenade=mcc_grenade_definition(type);
    struct object_marker hand;
    struct object_placement_data placement;
    long projectile;
    boolean unlimited=(object->unit.player_index!=NONE &&
        (cheat.infinite_ammo || game_engine_infinite_grenades(object->unit.player_index))) ||
        (object->unit.actor_index!=NONE && actor_has_unlimited_grenades(object->unit.actor_index));
    if (!grenade || grenade->projectile.index==NONE || (!unlimited && !mcc_grenades_get(unit,type) && !network_game_distributed_client())) {
        object->unit.grenade_throw_state=_unit_grenade_throw_ending;return;
    }
    if (!unlimited) mcc_grenades_set(unit,type,mcc_grenades_get(unit,type)-1);
    /* The common marker query supplies node zero as its fallback, including
     * for ability-bearing creatures without a named left-hand marker. */
    object_get_marker_by_name(unit,"left hand",&hand,1);
    object_placement_data_new(&placement,grenade->projectile.index,unit);
    placement.flags|=FLAG(_new_object_never_automatically_delete_bit);
    placement.position=hand.matrix.position;
    placement.forward=object->unit.aiming_vector;
    normalize3d(perpendicular3d(&placement.forward,&placement.up));
    projectile=object_new(&placement);
    object=unit_get(unit);
    if (projectile==NONE) object->unit.grenade_throw_state=_unit_grenade_throw_ending;
    else {
        object_attach_to_node(unit,projectile,hand.node_index);
        object->unit.grenade_object_index=projectile;
        object->unit.grenade_throw_state=_unit_grenade_throw_in_hand;
        network_damage_note_grenade(unit,type);
    }
}

void mcc_grenades_drop_extra(long unit,void (*drop_item)(long,long))
{
    short type;
    for (type=2;type<mcc_grenades_type_count();type++) {
        struct game_globals_grenade *grenade=mcc_grenade_definition(type);
        while (mcc_grenades_get(unit,type)>0) {
            struct object_placement_data placement;
            long item=NONE;
            if (grenade && grenade->item.index!=NONE) {
                object_placement_data_new(&placement,grenade->item.index,unit);
                item=object_new(&placement);
            }
            mcc_grenades_set(unit,type,mcc_grenades_get(unit,type)-1);
            if (item!=NONE) {object_disconnect_from_map(item);drop_item(unit,item);}
        }
    }
}

struct number_hud_element_definition {
    struct hud_placement_definition placement;struct hud_color_definition colors;
    char digits;unsigned char number_flags;char fractional_digits;unsigned char pad;long unused[3];
};
struct weapon_hud_overlay_definition {struct tag_reference bitmap;struct tag_block items;};
struct mcc_grenade_hud {
    struct hud_absolute_placement_definition absolute;
    struct static_hud_element_definition background;
    struct {struct static_hud_element_definition background;struct number_hud_element_definition numbers;short cutoff,pad;} counter;
    struct weapon_hud_overlay_definition overlays;
};

void mcc_grenades_hud(short local_player,long unit)
{
    struct unit_datum *object=unit_try_and_get(unit),*parent;
    struct game_globals_grenade *grenade;
    struct mcc_grenade_hud const *hud;
    short count,flags,overlays;
    long weapon,*flash;
    if (!object || local_player<0 || local_player>=NUMBEROF(mcc_grenade_flash_time)) return;
    grenade=mcc_grenade_definition(object->unit.current_grenade_index);
    if (!grenade || grenade->hud_interface.index==NONE) return;
    weapon=unit_inventory_get_weapon(unit,object->unit.current_weapon_index);
    if (weapon_prevents_grenade_throwing(weapon)) return;
    parent=unit_try_and_get(object->object.parent_object_index);
    if (parent && (parent->unit.driver_object_index==unit || parent->unit.gunner_object_index==unit)) return;
    hud=tag_get('grhi',grenade->hud_interface.index);
    count=mcc_grenades_get(unit,object->unit.current_grenade_index);
    flags=(count<=hud->counter.cutoff ? FLAG(_hud_draw_flashing_bit) : 0) |
        (!count ? FLAG(_hud_draw_disabled_bit) : 0) |
        (local_player_count()>1 ? FLAG(_hud_draw_in_multiplayer_bit) : 0);
    flash=&mcc_grenade_flash_time[local_player];
    if (!(flags&FLAG(_hud_draw_flashing_bit))) *flash=NONE;
    else if (*flash==NONE) *flash=game_time_get();
    if (hud->background.interface_bitmap.index!=NONE)
        hud_draw_static_element(local_player,&hud->absolute,&hud->background,flags,*flash);
    if (hud->counter.background.interface_bitmap.index!=NONE)
        hud_draw_static_element(local_player,&hud->absolute,&hud->counter.background,flags,*flash);
    if (hud->counter.numbers.digits)
        hud_draw_numbers(local_player,&hud->absolute,&hud->counter.numbers,count,NONE,flags,*flash,0.0f);
    overlays=(count<=hud->counter.cutoff ? 1 : 0)|(!count ? 2 : 0);
    if (!overlays) overlays|=4;
    overlays|=8;
    if (hud->overlays.bitmap.index!=NONE)
        hud_draw_weapon_overlays(local_player,&hud->absolute,&hud->overlays,overlays,*flash,flags,local_player_count()>1);
}

static uint32_t mcc_grenade_u32(unsigned char const *p)
{
    return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}

uint32_t mcc_grenades_snapshot(void *out,uint32_t capacity)
{
    unsigned i,count=0;
    unsigned char *bytes=out;
    uint32_t size;
    for (i=0;i<MAXIMUM_OBJECTS_PER_MAP;i++) {
        struct mcc_grenade_inventory const *inventory=&mcc_grenade_inventories[i];
        if ((inventory->counts[0] || inventory->counts[1]) && unit_try_and_get(inventory->handle)) count++;
    }
    size=8+count*8;
    if (!out) return size;
    if (capacity<size) return 0;
    memcpy(bytes,"GMC4",4);bytes[4]=1;bytes[5]=0;bytes[6]=(unsigned char)count;bytes[7]=(unsigned char)(count>>8);
    bytes+=8;
    for (i=0;i<MAXIMUM_OBJECTS_PER_MAP;i++) {
        struct mcc_grenade_inventory const *inventory=&mcc_grenade_inventories[i];
        uint32_t handle=(uint32_t)inventory->handle;
        if (!(inventory->counts[0] || inventory->counts[1]) || !unit_try_and_get(inventory->handle)) continue;
        bytes[0]=(unsigned char)handle;bytes[1]=(unsigned char)(handle>>8);bytes[2]=(unsigned char)(handle>>16);bytes[3]=(unsigned char)(handle>>24);
        bytes[4]=inventory->counts[0];bytes[5]=inventory->counts[1];bytes[6]=bytes[7]=0;bytes+=8;
    }
    return size;
}

int mcc_grenades_snapshot_validate(void const *in,uint32_t bytes)
{
    unsigned char const *data=in;
    unsigned char seen[MAXIMUM_OBJECTS_PER_MAP];
    unsigned i,count;
    if (!in || bytes<8 || memcmp(data,"GMC4",4) || data[4]!=1 || data[5]) return 0;
    count=data[6]|((unsigned)data[7]<<8);
    if (count>MAXIMUM_OBJECTS_PER_MAP || bytes!=8+count*8) return 0;
    memset(seen,0,sizeof(seen));
    for (i=0;i<count;i++) {
        unsigned char const *record=data+8+i*8;
        uint32_t handle=mcc_grenade_u32(record),index=handle&0xFFFF;
        if (handle==0xFFFFFFFFu || index>=MAXIMUM_OBJECTS_PER_MAP || seen[index] ||
            record[4]>127 || record[5]>127 || record[6] || record[7]) return 0;
        seen[index]=1;
    }
    return 1;
}

int mcc_grenades_restore(void const *in,uint32_t bytes)
{
    unsigned char const *data=in;
    unsigned i,count;
    if (!mcc_grenades_snapshot_validate(in,bytes)) return 0;
    count=data[6]|((unsigned)data[7]<<8);
    for (i=0;i<count;i++) {
        unsigned char const *record=data+8+i*8;
        if (!unit_try_and_get((long)mcc_grenade_u32(record)) ||
            (record[4] && mcc_grenades_type_count()<3) || (record[5] && mcc_grenades_type_count()<4)) return 0;
    }
    mcc_grenades_reset();
    for (i=0;i<count;i++) {
        unsigned char const *record=data+8+i*8;
        uint32_t handle=mcc_grenade_u32(record);
        struct mcc_grenade_inventory *inventory=&mcc_grenade_inventories[handle&0xFFFF];
        inventory->handle=(long)handle;inventory->counts[0]=record[4];inventory->counts[1]=record[5];
    }
    return 1;
}
