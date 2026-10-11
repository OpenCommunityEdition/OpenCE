/* One optional package directory, with no resource search or fallback. */
#include "mcc_package.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>

int mcc_package_name(char const *name, char canonical[64])
{
    size_t i, start=9, separators=0, length;
    static char const prefix[]="mcc_maps";
    if (!name) return 0;
    for (length=0;length<64 && name[length];length++) {}
    if (length<10 || length>63) return 0;
    for (i=0;i<8;i++) {
        unsigned char c=(unsigned char)name[i];
        if (c>='A' && c<='Z') c+='a'-'A';
        if (c!=prefix[i]) return 0;
    }
    if (name[8]!='/' && name[8]!='\\') return 0;
    memcpy(canonical,"mcc_maps\\",9);
    for (i=9;i<=length;i++) {
        unsigned char c=(unsigned char)name[i];
        if (!c || c=='/' || c=='\\') {
            if (i==start || name[i-1]=='.' || name[i-1]==' ') return 0;
            if (c && ++separators>1) return 0;
            start=i+1;
            canonical[i]=c ? '\\' : 0;
        } else {
            if (c<32 || strchr(":*?\"<>|",c)) return 0;
            canonical[i]=(char)c;
        }
    }
    return 1;
}

int mcc_package_map_path(char const *name, char path[128])
{
    char canonical[64];
    if (!mcc_package_name(name,canonical)) return 0;
    sprintf(path,"d:\\%s.map",canonical);
    return 1;
}

int mcc_package_resource_path(char const *name, unsigned type, char path[128])
{
    char canonical[64], *separator;
    size_t i;
    if (type<1 || type>2 || !mcc_package_name(name,canonical)) return 0;
    separator=strrchr(canonical,'\\');
    *separator=0;
    for (i=0;canonical[i];i++) if (canonical[i]=='\\') canonical[i]='/';
    sprintf(path,"%s/%s.map",canonical,type==1 ? "bitmaps" : "sounds");
    return 1;
}

int mcc_package_checksum(char const *name, uint32_t map_checksum, uint32_t *checksum)
{
    char canonical[64],path[128];
    unsigned char buffer[32768];
    uint32_t table[256],crc=~map_checksum,i,j;
    unsigned type;
    size_t bytes,k;
    FILE *file;
    if (!checksum || !mcc_package_name(name,canonical)) return 0;
    /* Preserve existing loose-map checkpoint and network identities. */
    if (!strchr(canonical+9,'\\')) { *checksum=map_checksum;return 1; }
    for (i=0;i<256;i++) {
        uint32_t value=i;
        for (j=0;j<8;j++) value=(value>>1)^(0xEDB88320u & (0u-(value&1u)));
        table[i]=value;
    }
    for (type=1;type<=2;type++) {
        if (!mcc_package_resource_path(canonical,type,path)) return 0;
        errno=0;
        file=fopen(path,"rb");
        if (!file && errno!=ENOENT) return 0;
        /* Separate absent, empty and populated resources and their types. */
        crc=(crc>>8)^table[(crc^(type*2+(file!=NULL)))&255];
        if (!file) continue;
        while ((bytes=fread(buffer,1,sizeof(buffer),file))!=0)
            for (k=0;k<bytes;k++) crc=(crc>>8)^table[(crc^buffer[k])&255];
        if (ferror(file)) { fclose(file);return 0; }
        fclose(file);
        crc=(crc>>8)^table[(crc^0xff)&255];
    }
    *checksum=~crc;
    if (!*checksum) *checksum=1; /* Zero means no comparison in native admission. */
    return 1;
}
