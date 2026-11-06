/* One-time reset of saved optional lighting, before environment overrides.
 * TOML source positions let us preserve comments, quoted/dotted keys and
 * inline tables without rewriting unrelated preferences. */
static const char *const lighting_reset_keys[] = {
    "display.plasma_lighting", "display.muzzle_lighting", "display.plasma_occlusion",
    "display.weapon_glow", "display.fuel_rod_lighting", "display.ammo_counter_lighting",
    "display.needler_lighting", "display.plasma_prong_lighting", "display.vehicle_lighting",
    "display.world_lighting", "display.jackal_shield_lighting", "display.covenant_shield_lighting",
    "display.shield_glow", "display.shield_glow_intensity"
};
#define LIGHTING_RESET_MARKER "display.dynamic_lighting_reset"
#define LIGHTING_RESET_COUNT (sizeof(lighting_reset_keys) / sizeof(lighting_reset_keys[0]))

static int lighting_settings_legacy(toml_datum_t table)
{
    toml_datum_t marker = toml_seek(table, LIGHTING_RESET_MARKER);
    return marker.type != TOML_BOOLEAN || !marker.u.boolean;
}

static int lighting_settings_reset_key(const char *name)
{
    size_t i;
    for (i = 0; i < LIGHTING_RESET_COUNT; i++)
        if (!strcmp(name, lighting_reset_keys[i])) return 1;
    return 0;
}

static char *lighting_settings_migrate(const char *text, toml_datum_t table)
{
    struct lighting_edit { size_t offset, length; const char *value; } edits[LIGHTING_RESET_COUNT + 1], swap;
    size_t i, j, count = 0, length = strlen(text), capacity = length + 256, cursor = 0, used = 0;
    char *out;
    if (!lighting_settings_legacy(table)) return NULL;
    for (i = 0; i <= LIGHTING_RESET_COUNT; i++)
    {
        const char *key = i < LIGHTING_RESET_COUNT ? lighting_reset_keys[i] : LIGHTING_RESET_MARKER;
        int intensity = !strcmp(key, "display.shield_glow_intensity");
        toml_datum_t datum = toml_seek(table, key);
        const char *start = text, *end;
        int line = 1;
        if (datum.type == TOML_UNKNOWN) continue;
        if (datum.lineno < 1 || datum.colno < 1) return NULL;
        while (*start && line < datum.lineno) if (*start++ == '\n') line++;
        if (line != datum.lineno || (size_t)(datum.colno - 1) > strcspn(start, "\n")) return NULL;
        start += datum.colno - 1;
        if (datum.type == TOML_BOOLEAN)
        {
            const char *old = datum.u.boolean ? "true" : "false";
            if (strncmp(start, old, strlen(old))) return NULL;
            end = start + strlen(old);
        }
        else if (intensity && datum.type == TOML_STRING)
        {
            char quote;
            int triple;
            /* tomlc17 locates string content after its opening delimiter
             * and the optional first newline of a multiline string. */
            if (start > text && start[-1] == '\n')
            { start--; if (start > text && start[-1] == '\r') start--; }
            if (start == text || (start[-1] != '"' && start[-1] != '\'')) return NULL;
            quote = start[-1];
            triple = start - text >= 3 && start[-2] == quote && start[-3] == quote;
            start -= triple ? 3 : 1;
            end = start + (triple ? 3 : 1);
            while (*end)
            {
                if (quote == '"' && *end == '\\') { if (!end[1]) return NULL; end += 2; continue; }
                if (*end == quote && (!triple || (end[1] == quote && end[2] == quote))) break;
                end++;
            }
            if (!*end) return NULL;
            end += triple ? 3 : 1;
        }
        else return NULL; /* Invalid types remain diagnosable; never mark a partial reset complete. */
        edits[count].offset = (size_t)(start - text);
        edits[count].length = (size_t)(end - start);
        edits[count++].value = i == LIGHTING_RESET_COUNT ? "true" : intensity ? "\"default\"" : "false";
    }
    for (i = 1; i < count; i++)
        for (j = i; j && edits[j].offset < edits[j-1].offset; j--)
        { swap = edits[j]; edits[j] = edits[j-1]; edits[j-1] = swap; }
    out = malloc(capacity);
    if (!out) return NULL;
    for (i = 0; i < count; i++)
    {
        size_t prefix = edits[i].offset - cursor, value_length = strlen(edits[i].value);
        if (edits[i].offset < cursor || edits[i].offset + edits[i].length > length)
        { free(out); return NULL; }
        memcpy(out + used, text + cursor, prefix); used += prefix;
        memcpy(out + used, edits[i].value, value_length); used += value_length;
        cursor = edits[i].offset + edits[i].length;
    }
    memcpy(out + used, text + cursor, length - cursor + 1);
    return out;
}
