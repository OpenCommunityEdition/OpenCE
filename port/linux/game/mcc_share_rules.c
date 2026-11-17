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

/* Sharing names must be portable even when the sender is not on Windows.
 * This is stricter than local discovery and does not change its accepted names. */
static int mcc_share_component(char const *name)
{
    char device[64];
    size_t i,length=0;
    if (!*name || *name=='.' || *name==' ') return 0;
    for (i=0;name[i];i++) {
        unsigned char c=(unsigned char)name[i];
        if (c<32 || c>126) return 0;
    }
    /* A device name remains a device before an extension or trailing spaces. */
    while (name[length] && name[length]!='.') length++;
    while (length && name[length-1]==' ') length--;
    memcpy(device,name,length);
    device[length]=0;
    if (mcc_share_same(device,"con") || mcc_share_same(device,"prn") ||
        mcc_share_same(device,"aux") || mcc_share_same(device,"nul") ||
        mcc_share_same(device,"conin$") || mcc_share_same(device,"conout$")) return 0;
    if (length==4 && device[3]>='0' && device[3]<='9') {
        device[3]=0;
        if (mcc_share_same(device,"com") || mcc_share_same(device,"lpt")) return 0;
    }
    return 1;
}

enum mcc_share_reason mcc_share_decide(struct mcc_share_package const *package)
{
    char input[64],canonical[64];
    char *folder,*level;
    if (!package || !package->name || !package->name_length ||
        package->name_length>=sizeof(input) ||
        memchr(package->name,0,package->name_length)) return MCC_SHARE_BAD_NAME;
    memcpy(input,package->name,package->name_length);
    input[package->name_length]=0;
    if (!mcc_package_name(input,canonical)) return MCC_SHARE_BAD_NAME;
    folder=canonical+9;
    level=strchr(folder,'\\');
    if (!level) return MCC_SHARE_NOT_A_PACKAGE;
    *level++=0;
    if (!mcc_share_component(folder) || !mcc_share_component(level) ||
        mcc_share_same(level,"bitmaps") || mcc_share_same(level,"sounds") ||
        mcc_share_same(level,"loc")) return MCC_SHARE_BAD_NAME;
    if (mcc_share_same(folder,"_shared")) return MCC_SHARE_SHARED_RESOURCES;
    if (package->content==MCC_SHARE_CONTENT_STOCK) return MCC_SHARE_STOCK;
    if (package->sidecar!=MCC_SHARE_SIDECAR_ABSENT &&
        package->sidecar!=MCC_SHARE_SIDECAR_VALID) return MCC_SHARE_UNKNOWN_ORIGIN;
    if (package->content!=MCC_SHARE_CONTENT_CUSTOM || !package->content_known) return MCC_SHARE_UNKNOWN_CONTENT;
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
    case MCC_SHARE_UNKNOWN_ORIGIN: return "unknown-origin";
    case MCC_SHARE_UNKNOWN_CONTENT: return "unknown-content";
    }
    return "unknown";
}

int mcc_share_file_allowed(char const *leaf, size_t length)
{
    char name[12];
    if (!leaf || (length!=10 && length!=11) || memchr(leaf,0,length)) return 0;
    memcpy(name,leaf,length);
    name[length]=0;
    return mcc_share_same(name,"bitmaps.map") || mcc_share_same(name,"sounds.map");
}
