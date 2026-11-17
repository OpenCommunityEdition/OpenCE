#include "mcc_local_package.h"
#include "posix.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static int same(const char *a, const char *b)
{
    while (*a && *b && tolower((unsigned char)*a)==tolower((unsigned char)*b)) {a++;b++;}
    return !*a && !*b;
}

static int child(const char *root, const char *name, char path[1024])
{
    char found[256];
    return posix_find_entry_case_insensitive(root,name,found,sizeof(found)) &&
        snprintf(path,1024,"%s/%s",root,found)<1024;
}

static int component(const char *name)
{
    size_t i, length=strlen(name);
    if (!length || name[length-1]=='.' || name[length-1]==' ') return 0;
    for (i=0;i<length;i++)
        if ((unsigned char)name[i]<32 || strchr("/\\:*?\"<>|",name[i])) return 0;
    return 1;
}

int mcc_local_package_path(const char *request, const char *data_root, char *path, unsigned long size)
{
    char relative[128], root[1024], package[1024], folder[1024], candidate[1024];
    char *separator, *leaf, *extension;
    struct posix_file_information information;
    unsigned i;
    int resource;
    if (!request || strlen(request)>=sizeof(relative)) return 0;
    for (i=0;request[i];i++) relative[i]=request[i]=='\\' ? '/' : request[i];
    relative[i]=0;
    separator=strchr(relative,'/');
    if (!separator) return 0;
    *separator=0;
    if (!same(relative,"mcc_maps")) return 0;
    leaf=strchr(separator+1,'/');
    if (!leaf) return 0;
    *leaf++=0;
    if (!component(separator+1) || !component(leaf) || same(separator+1,"_shared")) return 0;
    extension=strrchr(leaf,'.');
    if (!extension || (!same(extension,".map") && !same(extension,".txt"))) return 0;
    if (!child(data_root,"mcc_maps",root) || !child(root,separator+1,package)) return 0;
    /* Even an invalid direct override stays authoritative. Never borrow a
     * nested replacement for it, or resources from a second installation. */
    if (child(package,leaf,candidate)) return 0;
    resource=same(leaf,"bitmaps.map") || same(leaf,"sounds.map");
    if (!child(package,"maps",folder) || !child(folder,leaf,candidate)) {
        if (!resource || !child(package,"halo1",root) || !child(root,"maps",folder) ||
            !child(folder,leaf,candidate)) return 0;
    }
    if (!posix_mcc_share_path_safe(candidate) || posix_stat(candidate,&information) ||
        (information.flags & _posix_file_is_directory) || strlen(candidate)>=size) return 0;
    strcpy(path,candidate);
    return 1;
}
