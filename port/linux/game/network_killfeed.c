/* Co-op has no competitive game engine to announce player deaths.
   Only the host formats a death; every peer uses the normal multiplayer HUD. */
#include "cseries.h"
#include "game/game.h"
#include "game/game_globals.h"
#include "game/players.h"
#include "networking/network_game_globals.h"
#include "objects/objects.h"
#include "objects/damage.h"
#include "objects/damage_effect_definitions.h"
#include "units/units.h"
#include "tag_groups.h"
#include "network_coop.h"
#include "network_distributed.h"
#include "network_killfeed.h"
#include "interface/hud_messaging.h"
#include <string.h>

static long killfeed_dead_units[HALO_PORT_MAXIMUM_OBJECTS_PER_MAP];
/* The retail damage tag's categories (falling and vehicle collision). */
enum { KILLFEED_FALLING = 1, KILLFEED_VEHICLE = 9 };

void network_killfeed_reset(void)
{
    unsigned i;
    for (i = 0; i < HALO_PORT_MAXIMUM_OBJECTS_PER_MAP; i++) killfeed_dead_units[i] = NONE;
}

static boolean killfeed_present(struct player_datum const *player)
{
    return player && !player->quit_out_of_game;
}

/* Same 63-character HUD buffer as competitive multiplayer; no console color
   escapes or profile text may change its formatting. Keep Unicode names. */
enum { KILLFEED_TEXT_LENGTH = 63 };
struct killfeed_message
{
    struct distributed_message_header header;
    wchar_t text[KILLFEED_TEXT_LENGTH];
};

static void killfeed_show(wchar_t const *text)
{
    struct data_iterator iterator;
    struct player_datum *player;
    data_iterator_new(&iterator, player_data);
    while ((player = data_iterator_next(&iterator)) != NULL)
        if (player->local_player_index != NONE)
            hud_print_message(player->local_player_index, text);
}

static void killfeed_send(wchar_t const *text)
{
    struct killfeed_message message;
    memset(&message, 0, sizeof(message));
    ustrncpy(message.text, text, KILLFEED_TEXT_LENGTH);
    message.text[KILLFEED_TEXT_LENGTH - 1] = 0;
    killfeed_show(message.text);
    distributed_send(&message, _distributed_message_coop_death, 1,
        (word)(sizeof(message.header) + sizeof(message.text)), _distributed_to_clients_reliably);
}

long network_killfeed_entry_size(void)
{
    return sizeof(((struct killfeed_message *)0)->text);
}

void network_killfeed_handle_message(void const *entries, word size, byte count, boolean from_stream)
{
    wchar_t text[KILLFEED_TEXT_LENGTH];
    unsigned i;
    if (size != sizeof(text) || count != 1 || !from_stream || game_connection() != _game_connection_network_client ||
        !network_coop_active()) return;
    memcpy(text, entries, sizeof(text));
    text[KILLFEED_TEXT_LENGTH - 1] = 0;
    for (i = 0; text[i]; i++)
        if (text[i] < 32 || text[i] == L'|') text[i] = L'?';
    killfeed_show(text);
}

static void killfeed_player_name(struct player_datum const *player, wchar_t *name)
{
    unsigned i;
    for (i = 0; i < 12 && player->name[i]; i++)
        name[i] = player->name[i] < 32 || player->name[i] == L'|' ? L'?' : player->name[i];
    name[i] = 0;
    if (!i) ustrncpy(name, L"Unknown", 13);
}

/* Use the owning asset component, not a hand-picked list of damage effects.
   This covers every weapon/vehicle and custom assets in these tag families.
   Effects outside a named asset family deliberately fall back to "X died". */
static boolean killfeed_asset_name(char const *tag, wchar_t *name)
{
    static struct { char const *tag; wchar_t const *name; } const names[] =
    {
        {"floodcombat", L"Combat Flood"}, {"floodcarrier", L"Carrier Flood"},
        {"floodinfection", L"Infection Flood"}, {"cyborg", L"Spartan"},
        {"wraith", L"Wraith Tank"}, {"scorpion", L"Scorpion Tank"},
        {"plasma_cannon", L"Plasma Cannon"}, {"flamethrower", L"Flamethrower"}
    };
    char const *start, *end;
    unsigned i, length;
    boolean word_start = TRUE;
    name[0] = 0;
    if (strncmp(tag, "characters\\", 11) && strncmp(tag, "vehicles\\", 9) &&
        strncmp(tag, "weapons\\", 8)) return FALSE;
    start = strchr(tag, '\\') + 1;
    end = strchr(start, '\\');
    length = end ? (unsigned)(end - start) : (unsigned)strlen(start);
    if (!length || length >= 32) return FALSE;
    for (i = 0; i < NUMBEROF(names); i++)
        if (!strncmp(start, names[i].tag, strlen(names[i].tag)))
        {
            ustrncpy(name, names[i].name, 32);
            return TRUE;
        }
    for (i = 0; i < length; i++)
    {
        unsigned c = (unsigned char)start[i];
        if (c < 32 || c > 126 || c == '|') return FALSE;
        if (c == '_') c = ' ';
        name[i] = (wchar_t)(word_start && c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c);
        word_start = c == ' ';
    }
    name[length] = 0;
    return TRUE;
}

void network_killfeed_note_death(long unit_index, struct damage_data const *damage)
{
    struct unit_datum *unit;
    struct object_datum *owner;
    struct player_datum *dead, *killer;
    struct damage_definition const *definition;
    char const *tag, *damage_tag;
    wchar_t const *cause, *kind;
    wchar_t name[13], killer_name[13], line[KILLFEED_TEXT_LENGTH];
    wchar_t cause_name[32], kind_name[32];
    long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(unit_index);
    long dead_index, killer_index;
    if (game_connection() != _game_connection_network_server || !network_coop_active() ||
        slot < 0 || slot >= HALO_PORT_MAXIMUM_OBJECTS_PER_MAP ||
        !(unit = unit_try_and_get(unit_index)) || killfeed_dead_units[slot] == unit_index) return;
    dead_index = unit->unit.player_index;
    dead = player_try_and_get(dead_index);
    if (!killfeed_present(dead) || dead->unit_index != unit_index) return;
    killfeed_dead_units[slot] = unit_index;
    if (!damage)
    {
        if (TEST_FLAG(unit->object.damage_flags, _object_die_act_of_god_silent_bit) ||
            TEST_FLAG(unit->object.damage_flags, _object_die_act_of_god_no_statistics_bit)) return;
        killfeed_player_name(dead, name);
        usnprintf(line, NUMBEROF(line), L"%s %s", name,
            TEST_FLAG(unit->object.flags, _object_outside_of_map_bit) ? L"fell out of the world" : L"died");
        killfeed_send(line);
        return;
    }
    if (TEST_FLAG(damage->flags, _damage_no_statistics_bit)) return;
    killer_index = damage->owner_player_index;
    killer = player_try_and_get(killer_index);
    owner = object_try_and_get(damage->owner_object_index);
    definition = damage->definition_index != NONE ? &damage_effect_definition_get(damage->definition_index)->damage : NULL;
    tag = damage->definition_index != NONE ? tag_get_name(damage->definition_index) : "";
    if (!tag) tag = "";
    damage_tag = tag;
    cause = killfeed_asset_name(tag, cause_name) ? cause_name : NULL;
    tag = owner ? tag_get_name(owner->definition_index) : "";
    /* A driver/gunner owns the shot; its parent supplies the vehicle name. */
    if (owner && owner->object.parent_object_index != NONE)
    {
        struct object_datum *parent = object_try_and_get(owner->object.parent_object_index);
        char const *parent_tag = parent ? tag_get_name(parent->definition_index) : NULL;
        if (parent_tag && !strncmp(parent_tag, "vehicles\\", 9)) tag = parent_tag;
    }
    kind = killfeed_asset_name(tag ? tag : "", kind_name) ? kind_name : NULL;
    killfeed_player_name(dead, name);
    if (killfeed_present(killer) && killer_index != dead_index)
    {
        killfeed_player_name(killer, killer_name);
        usnprintf(line, NUMBEROF(line), L"%s was killed by %s", name, killer_name);
    }
    else if (killer_index == dead_index)
        usnprintf(line, NUMBEROF(line), L"%s killed themselves", name);
    else if (TEST_FLAG(unit->object.flags, _object_outside_of_map_bit) ||
        !strcmp(damage_tag, "globals\\maximum distance"))
        usnprintf(line, NUMBEROF(line), L"%s fell out of the world", name);
    else if (TEST_FLAG(unit->object.damage_flags, _object_die_act_of_god_bit))
        usnprintf(line, NUMBEROF(line), L"%s died", name);
    else if (definition && definition->category == KILLFEED_FALLING)
        usnprintf(line, NUMBEROF(line), L"%s died of fall", name);
    else if (definition && definition->category == KILLFEED_VEHICLE &&
        ((kind && tag && !strncmp(tag, "vehicles\\", 9)) ||
         (cause && !strncmp(damage_tag, "vehicles\\", 9))))
        usnprintf(line, NUMBEROF(line), L"%s was run over by %s", name,
            kind && tag && !strncmp(tag, "vehicles\\", 9) ? kind : cause);
    else if (cause && (strstr(damage_tag, "grenade") || !strncmp(damage_tag, "vehicles\\", 9)))
        usnprintf(line, NUMBEROF(line), L"%s was killed by %s", name, cause);
    else if (kind || cause)
        usnprintf(line, NUMBEROF(line), L"%s was killed by %s", name, kind ? kind : cause);
    else usnprintf(line, NUMBEROF(line), L"%s died", name);
    line[NUMBEROF(line) - 1] = 0;
    killfeed_send(line);
}
