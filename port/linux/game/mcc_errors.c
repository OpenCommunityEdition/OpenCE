#include "mcc_errors.h"
#include <stdio.h>
#include <string.h>

char const *mcc_status_explanation(enum mcc_cache_status status)
{
    switch (status) {
    case MCC_CACHE_OK: return "";
    case MCC_CACHE_WRONG_VERSION:
        return "It is not a supported Halo 1 MCC map (version 13). Install the MCC version of this map.";
    case MCC_CACHE_NO_MEMORY:
        return "There is not enough memory for its map data. Close other applications and try again.";
    case MCC_CACHE_READ_FAILED:
        return "Its data could not be read. Check that the complete map is accessible.";
    case MCC_CACHE_BAD_FILE_SIZE:
        return "Its file is incomplete or has an invalid length. Reinstall the complete map.";
    case MCC_CACHE_BAD_SCRIPT:
        return "Its script data is invalid or unsupported. Check for an updated map; debug.txt has the failing check.";
    case MCC_CACHE_BAD_BSP:
    case MCC_CACHE_BAD_MODEL_RANGE:
        return "Its level geometry is invalid or unsupported. Check for an updated map; debug.txt has the failing check.";
    case MCC_CACHE_BAD_BITMAP:
        return "Its texture data is invalid or unsupported. Check for an updated map; debug.txt has the failing check.";
    case MCC_CACHE_BAD_SOUND:
        return "Its sound data is invalid or unsupported. Check for an updated map; debug.txt has the failing check.";
    default:
        return "Its map data is invalid or unsupported. Check for an updated map; debug.txt has the failing check.";
    }
}

void mcc_error_message(char const *name, char const *reason, char *out, size_t size)
{
    size_t i;
    size_t length;
    char label[25];
    char summary[81];
    if (!out || !size) return;
    /* The stock dismissible dialog has five short lines. Keep both ends of
     * long package/level names, and leave detailed checks in the load log. */
    if (name && !strncmp(name, "mcc_maps", 8) && (name[8] == '/' || name[8] == '\\')) name += 9;
    if (!name) name="(unknown)";
    if (!reason) reason="See debug.txt for details.";
    length=strlen(name);
    if (length>24) snprintf(label,sizeof(label),"%.10s...%s",name,name+length-11);
    else snprintf(label,sizeof(label),"%s",name);
    if (strlen(reason)>80) {
        length=sizeof(summary)-sizeof("... See debug.txt.");
        while (length && reason[length]!=' ') --length;
        if (!length) length=sizeof(summary)-sizeof("... See debug.txt.");
        snprintf(summary,sizeof(summary),"%.*s... See debug.txt.",(int)length,reason);
    } else snprintf(summary,sizeof(summary),"%s",reason);
    snprintf(out, size, "MCC %s: %s",label,summary);
    for (i = 0; out[i]; i++)
        if ((unsigned char)out[i] < 32 || (unsigned char)out[i] >= 127) out[i] = '?';
}

void mcc_error_text(char const *name, char const *reason, wchar_t *out, size_t size)
{
    char message[512];
    size_t i;
    if (!out || !size) return;
    mcc_error_message(name, reason, message, sizeof(message));
    for (i = 0; message[i] && i < size - 1; i++) out[i] = (unsigned char)message[i];
    out[i] = 0;
}
