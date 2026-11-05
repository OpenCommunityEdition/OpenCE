#include "mcc_share_rules.h"
#include "mcc_package.h"
#include <string.h>

static int mcc_share_same(char const *a, char const *b)
{
    for (;*a && *b;a++,b++) {
        unsigned char x=(unsigned char)*a,y=(unsigned char)*b;
        if (x>='A' && x<='Z') x=(unsigned char)(x+'a'-'A');
        if (y>='A' && y<='Z') y=(unsigned char)(y+'a'-'A');
        if (x!=y) return 0;
    }
    return !*a && !*b;
}

static int mcc_share_stock_name(char const *folder)
{
    static char const *const names[]={
        "a10","a30","a50","b30","b40","c10","c20","c40","d20","d40",
        "beavercreek","bloodgulch","boardingaction","carousel","chillout","damnation",
        "dangercanyon","deathisland","gephyrophobia","hangemhigh","icefields","infinity",
        "longest","prisoner","putput","ratrace","sidewinder","timberland","wizard"};
    unsigned i;
    for (i=0;i<sizeof(names)/sizeof(names[0]);i++)
        if (mcc_share_same(folder,names[i])) return 1;
    return 0;
}

static int mcc_share_digits(char const *text)
{
    unsigned i;
    for (i=0;text[i];i++) if (text[i]<'0' || text[i]>'9') return 0;
    return i>0;
}

enum mcc_share_reason mcc_share_decide(struct mcc_share_package const *package)
{
    char canonical[64],folder[64];
    char const *start,*end;
    size_t length;
    if (!package || !package->name || !mcc_package_name(package->name,canonical)) return MCC_SHARE_BAD_NAME;
    start=canonical+9;
    end=strchr(start,'\\');
    if (!end) return MCC_SHARE_NOT_A_PACKAGE;
    length=(size_t)(end-start);
    memcpy(folder,start,length);
    folder[length]=0;
    if (mcc_share_same(folder,"_shared")) return MCC_SHARE_SHARED_RESOURCES;
    if (mcc_share_stock_name(folder)) return MCC_SHARE_STOCK;
    if (mcc_share_digits(folder) || package->sidecar==MCC_SHARE_SIDECAR_VALID) return MCC_SHARE_WORKSHOP;
    if (package->sidecar==MCC_SHARE_SIDECAR_UNREADABLE) return MCC_SHARE_UNKNOWN_ORIGIN;
    if (!package->content_known) return MCC_SHARE_UNKNOWN_CONTENT;
    if (!package->host_opt_in) return MCC_SHARE_NOT_OPTED_IN;
    return MCC_SHARE_ELIGIBLE;
}

int mcc_share_eligible(struct mcc_share_package const *package)
{
    return mcc_share_decide(package)==MCC_SHARE_ELIGIBLE;
}

char const *mcc_share_reason_word(enum mcc_share_reason reason)
{
    switch (reason) {
    case MCC_SHARE_ELIGIBLE: return "eligible";
    case MCC_SHARE_NOT_OPTED_IN: return "not-opted-in";
    case MCC_SHARE_BAD_NAME: return "bad-name";
    case MCC_SHARE_NOT_A_PACKAGE: return "not-a-package";
    case MCC_SHARE_SHARED_RESOURCES: return "shared-resources";
    case MCC_SHARE_STOCK: return "stock";
    case MCC_SHARE_WORKSHOP: return "workshop";
    case MCC_SHARE_UNKNOWN_ORIGIN: return "unknown-origin";
    case MCC_SHARE_UNKNOWN_CONTENT: return "unknown-content";
    }
    return "unknown";
}

int mcc_share_file_allowed(char const *leaf)
{
    if (!leaf) return 0;
    return mcc_share_same(leaf,"bitmaps.map") || mcc_share_same(leaf,"sounds.map");
}
