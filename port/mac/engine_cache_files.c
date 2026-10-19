#include "cseries.h"
#include "cache/cache_files.h"
#include "engine_cache_files.h"
#include "native_resource_memory.h"
#include "xbox_swap.h"
#include "xbox_geometry.h"
#undef memcpy
#undef memset
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
static struct mac_cache_files *active;
typedef char mac_cache_native_long[sizeof(long)==4?1:-1];
static int map_directory_set(const char *path,char destination[MAC_CACHE_MAP_DIRECTORY_MAX])
{
    const char *slash;size_t length;
    if(!path || !path[0])return 0;
    slash=strrchr(path,'/');
    if(!slash) {
        destination[0]='.';destination[1]='/';destination[2]=0;return 1;
    }
    length=(size_t)(slash-path)+1;
    if(length>=MAC_CACHE_MAP_DIRECTORY_MAX)return 0;
    memcpy(destination,path,length);destination[length]=0;return 1;
}
/* port: the converted sound space (port/mac/mac_sound_convert.c sets these in
   the session program; without them the space is unsupported) */
int (*mac_cache_files_converted_read_hook)(uint32_t offset,uint32_t size,void *destination)=NULL;
void (*mac_cache_files_converted_reset_hook)(void)=NULL;
const char *cache_files_map_directory(void)
{return active?active->map_directory:"";}
/* port: whether a map file is there (cache_files_map_present, for a network game's map) */
int mac_cache_files_map_file_exists(const char *path)
{
    FILE *file=path?fopen(path,"rb"):NULL;
    if(!file)return 0;
    fclose(file);return 1;
}
int mac_cache_files_open(const char *map,const char *bitmaps,const char *sounds,
    struct mac_cache_files *out,const char **error)
{
    struct mac_cache_files result;
    const char *why=NULL;
    if(mac_cache_files_converted_reset_hook)mac_cache_files_converted_reset_hook();
    if(!out){if(error)*error="cache scope output";return 0;}
    memset(out,0,sizeof(*out));memset(&result,0,sizeof(result));
    if(!map_directory_set(map,result.map_directory)) {
        if(error)*error="native map path has no representable parent directory";
        return 0;
    }
    /* port: an Xbox map (version 5) is inflated whole and used as the console
       used it; it has no bitmaps.map or sounds.map */
    if(mac_xbox_map_path_version(map)) {
        /* a big-endian host reads the maps' tags only after they are converted (xbox_swap.h) */
        if(!mac_xbox_swap_supported(&why)) {
            if(error)*error=why;
            return 0;
        }
        if(!mac_xbox_map_open(map,&result.map,error))return 0;
        if(!mac_xbox_map_inspect(result.map.data,result.map.size,&result.xbox_info,&why)) {
            if(error)*error=why;
            goto fail;
        }
        result.xbox=1;
        result.retail.file=result.map.data;result.retail.file_size=result.xbox_info.file_length;
        result.retail.tag_count=result.xbox_info.tag_count;result.retail.bsp_count=result.xbox_info.bsp_count;
        memcpy(result.retail.name,result.xbox_info.name,32);memcpy(result.retail.build,result.xbox_info.build,32);
        *out=result;if(error)*error=NULL;return 1;
    }
    if(!mac_mapped_file_open(map,MAC_CACHE_DECODED_BASE,&result.map,error))return 0;
    if(!mac_retail_open(result.map.data,result.map.size,&result.retail,&why)) {
        if(error)*error=why;
        goto fail;
    }
    if(bitmaps&&!mac_mapped_file_open(bitmaps,MAC_CACHE_SOUND_BASE-MAC_CACHE_BITMAP_BASE,&result.bitmaps,error))goto fail;
    if(sounds&&!mac_mapped_file_open(sounds,MAC_CACHE_OFFSET_LIMIT-MAC_CACHE_SOUND_BASE,&result.sounds,error))goto fail;
    if(!mac_retail_resources_check(&result.retail,(uint32_t)result.bitmaps.size,
        (uint32_t)result.sounds.size,&result.resources,&why)) {
        if(error)*error=why;
        goto fail;
    }
    *out=result;if(error)*error=NULL;return 1;
fail:mac_cache_files_close(&result);return 0;
}
static int xbox_mode;
void mac_xbox_mode_set(int on){xbox_mode=on?1:0;}
int mac_xbox_mode(void){return xbox_mode;}
/* port: the same answer in a variable, which the tag functions read without a call (engine_tags.c) */
int mac_cache_files_xbox_flag;
int mac_cache_files_xbox_active(void){return active&&active->xbox;}
const unsigned char *mac_cache_files_xbox_image(size_t *size)
{
    if(!active||!active->xbox)return NULL;
    if(size)*size=active->map.size;
    return active->map.data;
}
/* port: the engine's cache_file_open (cache_files_windows.c) for an Xbox map:
   the scope's map is the file; its header is the scenario's. Names are the
   map's last part, as scenario_tags_load strips them. */
static int leaf_equal(const char *a,const char *b)
{
    unsigned char x,y;
    if(!a||!b)return 0;
    while(*a&&*b) {
        x=(unsigned char)*a++;y=(unsigned char)*b++;
        if(x>='A'&&x<='Z')x=(unsigned char)(x-'A'+'a');
        if(y>='A'&&y<='Z')y=(unsigned char)(y-'A'+'a');
        if(x!=y)return 0;
    }
    return *a==0&&*b==0;
}
boolean cache_file_open(char const *scenario_name,struct cache_file_header *header)
{
    if(!active||!active->xbox||!scenario_name||!header)return FALSE;
    if(!leaf_equal(scenario_name,active->xbox_info.name))return FALSE;
    memcpy(header,active->map.data,0x800);
    /* the file's words are little endian: the engine reads them as host values */
    mac_xbox_cache_header_to_native(header);
    return TRUE;
}
void cache_file_close(void)
{
    /* the scope owns the map until it is closed */
}
int mac_cache_files_bind(struct mac_cache_files *scope)
{
    if(active||!scope||!scope->map.data)return 0;
    active=scope;mac_cache_files_xbox_flag=active->xbox?1:0;return 1;
}
int mac_cache_files_unbind(struct mac_cache_files *scope)
{
    if(!scope||active!=scope)return 0;active=NULL;mac_cache_files_xbox_flag=0;return 1;
}
void mac_cache_files_close(struct mac_cache_files *scope)
{
    if(!scope)return;if(active==scope){active=NULL;mac_cache_files_xbox_flag=0;}
    if(mac_cache_files_converted_reset_hook)mac_cache_files_converted_reset_hook();
    mac_mapped_file_close(&scope->sounds);mac_mapped_file_close(&scope->bitmaps);mac_mapped_file_close(&scope->map);
    memset(scope,0,sizeof(*scope));
}
int mac_cache_files_read(struct mac_cache_files *scope,uint32_t offset,uint32_t size,
    void *destination,size_t capacity,const char **error)
{
    const struct mac_mapped_file *file;const void *source;
    if(error)*error="cache scope or destination";
    if(!scope||!scope->map.data||!destination||size>capacity)return 0;
    if(offset>=MAC_CACHE_OFFSET_LIMIT){if(error)*error="cache offset outside native spaces";return 0;}
    if(offset>=MAC_CACHE_SOUND_BASE){file=&scope->sounds;offset-=MAC_CACHE_SOUND_BASE;}
    else if(offset>=MAC_CACHE_BITMAP_BASE){file=&scope->bitmaps;offset-=MAC_CACHE_BITMAP_BASE;}
    else if(offset>=MAC_CACHE_DECODED_BASE) {
        if(!mac_cache_files_converted_read_hook || !mac_cache_files_converted_read_hook(offset-MAC_CACHE_DECODED_BASE,size,destination)){
            if(error)*error="decoded sound space unsupported";return 0;
        }
        if(scope->completed_reads==ULONG_MAX){if(error)*error="cache completion counter exhausted";return 0;}
        ++scope->completed_reads;if(error)*error=NULL;return 1;
    }
    else file=&scope->map;
    source=mac_mapped_file_span(file,offset,size);
    if(!source){if(error)*error="cache file not bound or range invalid";return 0;}
    if(scope->completed_reads==ULONG_MAX){if(error)*error="cache completion counter exhausted";return 0;}
    memcpy(destination,source,size);++scope->completed_reads;
    /* port: what an Xbox map reads into the console's memory (tags, bsps,
       textures) is a write the GPU path's caches must see */
    memory_watch_prepare_write(destination,size);
    /* port: the byte-order hook (xbox_swap.h): the tag data or a bsp just read */
    if(scope->xbox&&offset==scope->xbox_info.tag_data_offset&&size==scope->xbox_info.tag_data_size) {
        if(!mac_xbox_tag_data_to_native((unsigned char *)destination,size,&scope->xbox_info,error))return 0;
        /* (then the models' vertices and indices, which the tag tables do not reach: xbox_geometry.h) */
        if(!mac_xbox_geometry_tags_to_native((unsigned char *)destination,size,&scope->xbox_info,error))return 0;
    } else if(scope->xbox) {
        uint32_t b;
        for(b=0;b<scope->xbox_info.bsp_count;++b)
            if(offset==scope->xbox_info.bsps[b].file_offset&&size==scope->xbox_info.bsps[b].size) {
                if(!mac_xbox_bsp_to_native((unsigned char *)destination,size,&scope->xbox_info.bsps[b],&scope->xbox_info,error))return 0;
                if(!mac_xbox_geometry_bsp_to_native((unsigned char *)destination,size,&scope->xbox_info.bsps[b],error))return 0;
                break;
            }
    }
    if(error)*error=NULL;return 1;
}
static void read_failure(long tag,long offset,long size,const char *why)
{
    fprintf(stderr,"native cache read refused: tag=%08lx offset=%08lx size=%ld: %s\n",
        (unsigned long)tag,(unsigned long)offset,size,why);abort();
}
short cache_file_read(long tag,long offset,long size,void *buffer,boolean *completion,boolean blocking)
{
    const char *error;
    (void)blocking;
    if(!completion)read_failure(tag,offset,size,"completion pointer missing");
    if(offset<0||size<0)read_failure(tag,offset,size,"negative range");
    if(!mac_cache_files_read(active,(uint32_t)offset,(uint32_t)size,buffer,(size_t)size,&error))
        read_failure(tag,offset,size,error);
    /* Only successful real copies reach TRUE. A failed flag would be mistaken
       for success by current sound/texture cache consumers. */
    *completion=TRUE;
    return 0;
}
void cache_file_promote_read(short request)
{
    /* The sole completed slot has no asynchronous work left to promote. */
    if(request!=0||!active||!active->completed_reads)
        read_failure(NONE,0,0,"unknown completed request");
}
void cache_file_block_until_not_busy(void)
{
    /* All reads finish before returning, including those requested nonblocking.
       There is no worker queue or outstanding request in this implementation. */
}
