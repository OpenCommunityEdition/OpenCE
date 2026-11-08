/* The game side of MCC package sharing (mcc_share.c) against stub engine headers: two copies of
 * the real file, one playing the host and one the joiner, each in a folder of its own, exchange
 * the real distributed messages through a queue. The real source planner, session and stage
 * run against synthetic files. The engine identity probe hashes the map and available resources
 * through mcc_package.c's paths, exercising the staging redirect. Metadata schema/decoder stubs
 * isolate adapter behavior; their production implementation has its own suite.
 * No game, no sockets, no game data. argv[1] is a case, argv[2] an
 * empty folder. */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#define chdir _chdir
#define getcwd _getcwd
#define rmdir _rmdir
#define unlink _unlink
#define mkdir(p,m) _mkdir(p)
#define S_ISDIR(m) (((m)&_S_IFMT)==_S_IFDIR)
#else
#include <unistd.h>
#include <dirent.h>
#endif
#include <wchar.h>
#include "mcc_package.h"
#include "mcc_sha256.h"
#include "mcc_share_session.h"
#include "mcc_workshop.h"
#include "mcc_share_metadata.h"
#include "mcc_share_storage.h"
#include "cseries.h"
#include "networking/network_client_manager.h"
#include "network_distributed.h"

/* Match the engine allocator contract without reading a libc pointer's absent
 * debug header. A mismatched allocation fails deterministically under ASan too. */
#undef malloc
#undef free
static void *engine_allocations[32];
static int engine_allocation_count;
void *fixture_engine_malloc(size_t bytes)
{
    void *pointer=malloc(bytes);
    if (pointer) {
        if (engine_allocation_count==32) { fprintf(stderr,"engine allocation fixture full\n"); exit(85); }
        memset(pointer,0xca,bytes);
        engine_allocations[engine_allocation_count++]=pointer;
    }
    return pointer;
}
void fixture_engine_free(void *pointer)
{
    int i;
    for (i=0;i<engine_allocation_count;i++) if (engine_allocations[i]==pointer) break;
    if (i==engine_allocation_count) {
        fprintf(stderr,"engine free received a non-engine allocation\n");
        free(pointer); exit(86);
    }
    engine_allocations[i]=engine_allocations[--engine_allocation_count];
    free(pointer);
}

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); return 1; } } while (0)
#define CASE(x) if (!strcmp(argv[1],x))
#define LEVEL "mcc_maps\\solar_flare\\m0"
#define MACHINE 5

/* the engine's types the stub headers declare */


struct network_game_server_client_machine;

/* both copies' entry points, renamed by the build: h_ for the host, j_ for the joiner */
#define COPY(prefix) \
    void prefix##mcc_share_game_update(void); \
    void prefix##mcc_share_game_render(void); \
    boolean prefix##mcc_share_game_downloading(char const *); \
    boolean prefix##mcc_share_game_holds_players(void); \
    boolean prefix##mcc_share_game_machine_active(long); \
    boolean prefix##mcc_share_game_handles_message(word const *, word); \
    void prefix##mcc_share_game_handle_message(long, word const *, word, boolean); \
    void prefix##mcc_share_game_disconnect(void); \
    void prefix##mcc_share_game_prompt_action(int); \
    void prefix##mcc_share_game_prompt_closed(void); \
    void prefix##mcc_share_game_prompt_text(wchar_t text[512]); \
    int prefix##mcc_share_game_can_download(void); \
    int prefix##mcc_share_game_has_workshop(void); \
    void prefix##mcc_share_game_peer_left(struct network_game_server_client_machine *);
COPY(h_)
COPY(j_)

static char host_dir[1024],join_dir[1024];
static unsigned clock_ms=1000;
static int map_share_on,map_download_on,aborts,errors_shown,accepts,accept_result;
static char shown[512];
static int (*host_missing)(char const[128],unsigned char const[32],unsigned char const[32]);
static int (*join_missing)(char const[128],unsigned char const[32],unsigned char const[32]);
static unsigned char host_digest[32],host_metadata[32];
static char snapshot[64],opened_url[128];
static int role_host,peer_joined=1,peer_capable=1,prompts,lobby_restores;
static int overlay_draws,allow_prompt=1;
static unsigned resource_mask=3,host_revision=1;
static int workshop_fallback,resource_shared,resource_invalid;
FILE *fixture_fopen(char const *,char const *);
static int join_in_game=1;
FILE *mcc_share_open_native(char const *path) { return fopen(path,"rb"); }
void mcc_share_storage_native_io(struct mcc_share_io *io)
{ memset(io,0,sizeof(*io)); io->open_native_read=mcc_share_open_native; }
int mcc_share_storage_find_resource(char const *maps,struct mcc_share_io const *io,
    struct mcc_share_file const *expected,char path[1024])
{ (void)maps;(void)io;(void)expected;(void)path;return 0; }

/* Storage receipt/attribution security has its own real-filesystem suite. */
int mcc_share_storage_record(struct mcc_share_stage *stage,char const *source,char const *maker,
    struct mcc_workshop_item const *item)
{
    (void)item;
    return stage->verified && stage->state==MCC_SHARE_STAGE_STAGED && source[0] && maker[0];
}

struct message { unsigned bytes; unsigned char data[4200]; long machine; };
static struct message to_host[400],to_join[400];
static int to_host_count,to_join_count;

/* ---------- the stubs the game side calls */

int config_boolean(char const *name)
{
    if (!strcmp(name,"network.mcc_map_share")) return map_share_on;
    if (!strcmp(name,"network.map_download")) return map_download_on;
    return 0;
}
static long upload_kb=64;
long config_integer(char const *name) { return strcmp(name,"network.map_share_upload")==0 ? upload_kb : 0; }
unsigned long system_milliseconds(void) { return clock_ms; }
void error(short level, char const *format, ...) { (void)level;(void)format; }
int csstrcasecmp(char const *a, char const *b)
{
    for (;*a && *b;a++,b++) {
        int x=*a,y=*b;
        if (x>='A' && x<='Z') x+='a'-'A';
        if (y>='A' && y<='Z') y+='a'-'A';
        if (x!=y) return x-y;
    }
    return *a-*b;
}
void offset_rectangle2d(void *r, short x, short y) { (void)r;(void)x;(void)y; }
void build_message_header(word *header, word size, int type, int flags) { *header=(word)(size<<4|type<<2|flags); }
int game_in_progress(void) { return 0; }
long game_time_get(void) { return 0; }
long interface_get_tag_index(int tag) { (void)tag; return 1; }
void draw_string_set_draw_mode(long a, long b, short c, short d, void const *e) { (void)a;(void)b;(void)c;(void)d;(void)e; }
void rasterizer_draw_string(void *a, void *b, void *c, short d, char const *e) { (void)a;(void)b;(void)c;(void)d;if (!e || !*e) exit(2);overlay_draws++; }
struct { struct { rectangle2d window_bounds,viewport_bounds; } camera; } render;

void mcc_maps_rescan(void) {}
int mcc_maps_workshop_title(char const *item, char *title, size_t size) { (void)item;(void)title;(void)size; return 0; }

/* the engine's file calls, on the folder the copy runs in */
static int exists_any(char const *path) { struct stat s; return stat(path,&s)==0; }
unsigned long GetFileAttributesA(char const *path)
{
    struct stat s;
    if (stat(path,&s)) return (unsigned long)-1;
    return S_ISDIR(s.st_mode) ? 0x10 : 0x80;
}
int CreateDirectoryA(char const *path, void *security) { (void)security; return mkdir(path,0755)==0; }
int RemoveDirectoryA(char const *path) { return rmdir(path)==0; }
static unsigned long long disk_free=1ull<<40;
typedef union { struct { unsigned int LowPart, HighPart; }; unsigned long long QuadPart; } ULARGE_INTEGER;
int GetDiskFreeSpaceExA(char const *path, ULARGE_INTEGER *available, ULARGE_INTEGER *total, ULARGE_INTEGER *free_total)
{
    (void)path;(void)total;(void)free_total;
    available->QuadPart=disk_free;
    return 1;
}
int MoveFileA(char const *from, char const *to) { return !exists_any(to) && rename(from,to)==0; }
int DeleteFileA(char const *path) { return remove(path)==0; }

/* the identity of the package at its paths: the map and its two resource maps, in order */
static int package_digest(char const *level, unsigned char digest[32])
{
    struct mcc_sha256 hash;
    char path[1024];
    unsigned i;
    mcc_sha256_begin(&hash);
    for (i=0;i<3;i++) {
        FILE *f;
        unsigned char block[4096];
        size_t got;
        char *p;
        if (i && !(resource_mask&(1u<<(i-1)))) continue;
        if (i==0) {
            if (!mcc_package_map_path(level,path)) return 0;
            memmove(path,path+3,strlen(path+3)+1);
            for (p=path;*p;p++) if (*p=='\\') *p='/';
        } else if (!mcc_package_resource_path(level,i,path)) return 0;
        f=fixture_fopen(path,"rb");
        if (!f) { if (i) continue; return 0; }
        while ((got=fread(block,1,sizeof(block),f))>0) mcc_sha256_add(&hash,block,(int)got);
        fclose(f);
    }
    mcc_sha256_end(&hash,digest);
    return 1;
}
int mcc_content_probe(char const *level, unsigned char digest[32]) { return package_digest(level,digest); }
int j_mcc_identity_host_package(char const *level, unsigned char digest[32]) { (void)level;(void)digest;return 0; }
int h_mcc_identity_host_package(char const *level, unsigned char digest[32])
{
    if (csstrcasecmp(level,LEVEL)) return 0;
    memcpy(digest,host_digest,32);
    return 1;
}

static void find_snapshot(void)
{
#ifdef _WIN32
    struct _finddata_t info;
    intptr_t search=_findfirst("mcc_maps/dl_*",&info);
    if (search!=-1) {
        do { if (info.attrib&_A_SUBDIR) snprintf(snapshot,sizeof(snapshot),"%s",info.name); }
        while (!_findnext(search,&info));
        _findclose(search);
    }
#else
    DIR *dir=opendir("mcc_maps");
    struct dirent *entry;
    if (!dir) return;
    while ((entry=readdir(dir))) if (!strncmp(entry->d_name,"dl_",3))
        snprintf(snapshot,sizeof(snapshot),"%s",entry->d_name);
    closedir(dir);
#endif
}
void h_mcc_identity_set_missing(int (*handler)(char const[128],unsigned char const[32],unsigned char const[32])) { host_missing=handler; }
void j_mcc_identity_set_missing(int (*handler)(char const[128],unsigned char const[32],unsigned char const[32])) { join_missing=handler; }
int j_mcc_identity_accept_installed(char const name[128], unsigned char const digest[32],unsigned char const metadata[32],char const *installed)
{
    unsigned char actual[32];
    char level[128];
    (void)name;(void)metadata;
    accepts++;
    find_snapshot();
    snprintf(level,sizeof(level),"%s",installed);
    accept_result=package_digest(level,actual) && !memcmp(actual,digest,32);
    return accept_result;
}
int h_mcc_identity_accept_installed(char const name[128],unsigned char const digest[32],unsigned char const metadata[32],char const *installed)
{ (void)name;(void)digest;(void)metadata;return 1; }
unsigned long h_mcc_identity_host_revision(void) { return host_revision; }
unsigned long j_mcc_identity_host_revision(void) { return 0; }
void h_display_error_text_when_main_menu_loaded(wchar_t const *t) { (void)t; }
void j_display_error_text_when_main_menu_loaded(wchar_t const *text)
{
    unsigned i;
    errors_shown++;
    for (i=0;text[i] && i<sizeof(shown)-1;i++) shown[i]=(char)text[i];
    shown[i]=0;
}
struct widget_instance { int unused; };
struct widget_instance *ui_widget_load_by_name_or_tag(char const *name,long tag,void *parent,int a,int b,int c,int d)
{
    static struct widget_instance widget;
    wchar_t text[512];
    unsigned i;
    if (!allow_prompt) return NULL;
    if (!role_host) {
        prompts++;
        j_mcc_share_game_prompt_text(text);
        for (i=0;text[i] && i<sizeof(shown)-1;i++) shown[i]=(char)text[i];
        shown[i]=0;
    }
    return &widget;
}
void ui_widgets_pop_stack(int controller)
{ (void)controller; /* Removing history alone does not restore a screen. */ }
void ui_widget_delete(struct widget_instance *widget)
{ if (role_host) h_mcc_share_game_prompt_closed(); else j_mcc_share_game_prompt_closed(); }
void ui_widget_port_go_back(struct widget_instance *widget)
{ ui_widget_delete(widget);lobby_restores++; }
long tag_loaded(unsigned long group,char const *name) { return -1; }
void platform_open_url(char const *url) { snprintf(opened_url,sizeof(opened_url),"%s",url); }
void h_network_game_abort(void) {}
void j_network_game_abort(void) { aborts++; }
struct network_game_server { int unused; };
struct network_game_client { int unused; };
struct network_game_server_client_machine { int unused; };
static struct network_game_server server;
static struct network_game_client client;
static struct network_game_server_client_machine peer;
static struct network_game host_game,join_game;
struct network_game *h_network_game_client_get_game(struct network_game_client *c) { return &host_game; }
struct network_game *j_network_game_client_get_game(struct network_game_client *c) { return &join_game; }
struct network_game *h_network_game_get_game(void) { return &host_game; }
struct network_game *j_network_game_get_game(void) { return join_in_game ? &join_game : NULL; }
struct network_game_server *h_global_network_game_server_get(void) { return &server; }
struct network_game_server *j_global_network_game_server_get(void) { return NULL; }
struct network_game_client *h_global_network_game_client_get(void) { return &client; }
struct network_game_client *j_global_network_game_client_get(void) { return join_in_game ? &client : NULL; }
struct network_game_server_client_machine *network_game_server_get_client_machine_at_index(struct network_game_server *s,long index)
{ return index==MACHINE ? &peer : NULL; }
int network_game_server_client_machine_is_joined_to_game(struct network_game_server *s,struct network_game_server_client_machine *p)
{ return p==&peer && peer_joined; }
int mcc_protocol_allows(struct network_game const *g,struct network_game_server_client_machine const *p)
{ return g && p==&peer && peer_capable; }
int mcc_level_name(char const *name) { return !strncmp(name,"mcc_maps",8); }
int network_connection_last_read_was_unreliable(void) { return 0; }
static int joiner_replies;
void platform_translate_path(char const *input,char *output,unsigned long size)
{
    char path[2048],cwd[1024],*p;
    if (!strncmp(input,"d:",2)) {
        input+=2;
        while (*input=='/' || *input=='\\') input++;
    }
    snprintf(path,sizeof(path),"%s",input);
    for (p=path;*p;p++) if (*p=='\\') *p='/';
    if (role_host && workshop_fallback && (!strcmp(path,"mcc_maps/solar_flare/bitmaps.map") ||
        !strcmp(path,"mcc_maps/solar_flare/sounds.map"))) {
        snprintf(output,size,"%s/halo1/maps/%s",host_dir,strrchr(path,'/')+1);return;
    }
    if (path[0]=='/' || (path[0] && path[1]==':')) snprintf(output,size,"%s",path);
    else { getcwd(cwd,sizeof(cwd)); snprintf(output,size,"%s/%s",cwd,path); }
}
FILE *fixture_fopen(char const *path,char const *mode)
{ char native[2048]; platform_translate_path(path,native,sizeof(native)); return fopen(native,mode); }
int posix_find_entry_case_insensitive(char const *directory,char const *name,char *result,unsigned long size)
{
    char path[2048];
    snprintf(path,sizeof(path),"%s/%s",directory,name);
    if (!exists_any(path)) return 0;
    snprintf(result,size,"%s",name); return 1;
}
int mcc_validation_resources(char const *name,char paths[2][128])
{
    unsigned i;
    memset(paths,0,256);
    if (resource_invalid) return 0;
    for (i=1;i<=2;i++) if (resource_mask&(1u<<(i-1)))
        if (!mcc_package_resource_path(resource_shared ? "mcc_maps/root" : name,i,paths[i-1])) return 0;
    return 1;
}
int mcc_manifest_package_root(char const *level,char root[1024])
{
    char path[128],*p;
    if (!mcc_package_map_path(level,path)) return 0;
    platform_translate_path(path,root,1024);
    p=strrchr(root,'/'); if (!p) return 0; *p=0; return 1;
}
/* Metadata parsing/PNG decoding have independent production-source tests. These deterministic
 * fixture planners isolate adapter consent, file selection, hashing and publication. */
int mcc_share_metadata_plan(char const *root,int campaign,char const *level,
    struct mcc_share_metadata_plan *plan,char reason[128])
{
    static char const *paths[]={"ModInfo.json","assets/menu.png","workshop_id.txt"};
    unsigned i;
    memset(plan,0,sizeof(*plan));
    for (i=0;i<3;i++) {
        char path[2048]; struct stat st;
        snprintf(path,sizeof(path),"%s/%s",root,paths[i]);
        if (!stat(path,&st)) {
            struct mcc_share_metadata_entry *e=&plan->files[plan->count++];
            if (i==2) {
                char text[128]; struct mcc_workshop_item item; FILE *f=fopen(path,"rb"); size_t n;
                if (!f) return 0; n=fread(text,1,sizeof(text)-1,f); fclose(f); text[n]=0;
                if (!mcc_workshop_parse_sidecar(text,&item)) return 0;
            }
            snprintf(e->path,sizeof(e->path),"%s",paths[i]); e->kind=i+1; e->size=(unsigned)st.st_size;
        }
    }
    return 1;
}
int mcc_share_metadata_digest(char const *root,int campaign,char const *level,unsigned char digest[32],char reason[128])
{
    char path[2048]; FILE *file; struct mcc_sha256 hash; unsigned char bytes[256]; size_t n;
    memset(digest,0,32); snprintf(path,sizeof(path),"%s/ModInfo.json",root); file=fopen(path,"rb");
    if (!file) return 1;
    mcc_sha256_begin(&hash);
    while ((n=fread(bytes,1,sizeof(bytes),file))) mcc_sha256_add(&hash,bytes,(int)n);
    fclose(file); mcc_sha256_end(&hash,digest); return 1;
}
int mcc_share_metadata_validate(char const *root,int campaign,char const *level,
    struct mcc_share_metadata_entry const *received,int count,unsigned char keep[160],char reason[128])
{
    struct mcc_share_metadata_plan plan; int i,j;
    if (!mcc_share_metadata_plan(root,campaign,level,&plan,reason) || plan.count!=count) return 0;
    for (i=0;i<count;i++) {
        for (j=0;j<plan.count;j++) if (!strcmp(received[i].path,plan.files[j].path)) break;
        if (j==plan.count || received[i].size!=plan.files[j].size || received[i].kind!=plan.files[j].kind) return 0;
        keep[i]=1;
    }
    return 1;
}

int h_network_distributed_server_send_to_machine_reliably(long machine, void *message, word size)
{
    if (to_join_count<400 && size<=sizeof(to_join[0].data)) {
        memcpy(to_join[to_join_count].data,message,size);
        to_join[to_join_count].bytes=size;
        to_join[to_join_count].machine=machine;
        to_join_count++;
    }
    return 1;
}
int h_network_distributed_client_send_reliably(void *message, word size) { (void)message;(void)size;return 1; }
int j_network_distributed_server_send_to_machine_reliably(long machine, void *message, word size) { (void)machine;(void)message;(void)size;joiner_replies++;return 1; }
int j_network_distributed_client_send_reliably(void *message, word size)
{
    if (to_host_count<400 && size<=sizeof(to_host[0].data)) {
        memcpy(to_host[to_host_count].data,message,size);
        to_host[to_host_count].bytes=size;
        to_host_count++;
    }
    return 1;
}

/* ---------- files and the two machines */

static unsigned char *make_bytes(unsigned size, unsigned seed)
{
    unsigned char *data=malloc(size?size:1);
    unsigned i,x=seed*2654435761u+1;
    for (i=0;i<size;i++) {
        x=x*1664525u+1013904223u;
        data[i]=(unsigned char)(x>>24);
    }
    return data;
}

static int write_text(char const *path, unsigned char const *data, unsigned size)
{
    FILE *f=fopen(path,"wb");
    if (!f) return 0;
    if (fwrite(data,1,size,f)!=size) {fclose(f);return 0;}
    return fclose(f)==0;
}

static int read_file(char const *path, unsigned char **data, unsigned *size)
{
    FILE *f=fopen(path,"rb");
    long n;
    if (!f) return 0;
    fseek(f,0,SEEK_END);n=ftell(f);fseek(f,0,SEEK_SET);
    *data=malloc((size_t)n+1);
    if (fread(*data,1,(size_t)n,f)!=(size_t)n) {fclose(f);return 0;}
    fclose(f);
    *size=(unsigned)n;
    return 1;
}

static int same_file(char const *a, char const *b)
{
    unsigned char *da=NULL,*db=NULL;
    unsigned sa=0,sb=0;
    int same=read_file(a,&da,&sa) && read_file(b,&db,&sb) && sa==sb && !memcmp(da,db,sa);
    free(da);free(db);
    return same;
}

static int exists(char const *path) { struct stat s; return stat(path,&s)==0; }

static int host_make(char const *leaf, unsigned size, unsigned seed)
{
    char path[2048];
    unsigned char *data=make_bytes(size,seed);
    int ok;
    snprintf(path,sizeof(path),"%s/mcc_maps/solar_flare/%s",host_dir,leaf);
    ok=write_text(path,data,size);
    free(data);
    return ok;
}

static void as_host(void) { role_host=1; if (chdir(host_dir)) abort(); }
static void as_join(void) { role_host=0; if (chdir(join_dir)) abort(); }

static int setup(char const *base)
{
    char path[2048];
    unsigned char list[200];
    char hex[65];
    unsigned i;
    snprintf(host_dir,sizeof(host_dir),"%s/host",base);
    snprintf(join_dir,sizeof(join_dir),"%s/join",base);
    if (mkdir(host_dir,0755) || mkdir(join_dir,0755)) return 0;
    snprintf(path,sizeof(path),"%s/mcc_maps",host_dir);if (mkdir(path,0755)) return 0;
    snprintf(path,sizeof(path),"%s/mcc_maps/solar_flare",host_dir);if (mkdir(path,0755)) return 0;
    if (!host_make("m0.map",15483,1) || !host_make("bitmaps.map",7000,2) || !host_make("sounds.map",6144,3)) return 0;
    snprintf(path,sizeof(path),"%s/mcc_maps/solar_flare/assets",host_dir); if (mkdir(path,0755)) return 0;
    if (!host_make("ModInfo.json",120,4) || !host_make("assets/menu.png",80,6) || !host_make("readme.txt",300,5)) return 0;
    as_host();
    if (!package_digest(LEVEL,host_digest)) return 0;
    { char reason[128]; if (!mcc_share_metadata_digest("mcc_maps/solar_flare",-1,"m0",host_metadata,reason)) return 0; }
    for (i=0;i<32;i++) snprintf(hex+i*2,3,"%02x",host_digest[i]);
    snprintf((char *)list,sizeof(list),"# the host's own list\n%s Example Maker\n",hex);
    if (!write_text("mcc_share.txt",list,(unsigned)strlen((char *)list))) return 0;
    map_share_on=1;map_download_on=1;
    return 1;
}

static void pump_once(void)
{
    static struct message batch[400];
    int i,count;
    count=to_host_count;memcpy(batch,to_host,(size_t)count*sizeof(batch[0]));to_host_count=0;
    as_host();
    for (i=0;i<count;i++) h_mcc_share_game_handle_message(MACHINE,(word const *)batch[i].data,(word)batch[i].bytes,1);
    count=to_join_count;memcpy(batch,to_join,(size_t)count*sizeof(batch[0]));to_join_count=0;
    as_join();
    for (i=0;i<count;i++) j_mcc_share_game_handle_message(-1,(word const *)batch[i].data,(word)batch[i].bytes,1);
    as_host();h_mcc_share_game_update();
    as_join();j_mcc_share_game_update();
    clock_ms+=20;
}


static int pump_prompt(void)
{
    int i,before=prompts;
    for (i=0;i<300 && prompts==before;i++) pump_once();
    return prompts>before;
}
static int pump_until_done(int limit)
{
    int i;
    for (i=0;i<limit && !accepts;i++) {
        pump_once();
        if (j_mcc_share_game_can_download()) { as_join(); j_mcc_share_game_prompt_action(0); }
        else if (prompts>1) return 0;
    }
    return accepts>0;
}
static void wire_name(char name[128],char const *level)
{ memset(name,0,128); snprintf(name,64,"%s",level); }
static int joiner_asks(char const *level,unsigned char const digest[32])
{
    char name[128]; wire_name(name,level); memcpy(join_game.mcc_content_sha256,digest,32);
    memcpy(join_game.mcc_metadata_sha256,host_metadata,32);as_join(); return join_missing(name,digest,host_metadata);
}
static int workshop_asks(void)
{
    char name[128]; struct mcc_workshop_item item;
    wire_name(name,LEVEL); memset(&item,0,sizeof(item)); strcpy(item.id,"2915693904"); strcpy(item.title,"Fixture package");
    mcc_workshop_wire_pack(name,&item); memcpy(join_game.map.name,name,128);
    memcpy(join_game.mcc_content_sha256,host_digest,32);memcpy(join_game.mcc_metadata_sha256,host_metadata,32);
    as_join(); return join_missing(name,host_digest,host_metadata);
}
static int write_list(unsigned char const digest[32],char const *maker)
{
    char text[200],hex[65]; unsigned i;
    for (i=0;i<32;i++) snprintf(hex+i*2,3,"%02x",digest[i]);
    snprintf(text,sizeof(text),"%s %s\n",hex,maker); as_host();
    return write_text("mcc_share.txt",(unsigned char *)text,(unsigned)strlen(text));
}
static int transfer_started(void)
{
    int i;
    if (!joiner_asks(LEVEL,host_digest) || !pump_prompt() || !j_mcc_share_game_can_download()) return 0;
    as_join(); j_mcc_share_game_prompt_action(0);
    for (i=0;i<3;i++) pump_once();
    return h_mcc_share_game_machine_active(MACHINE) && j_mcc_share_game_holds_players();
}

int main(int argc,char **argv)
{
    char a[2048],b[2048]; unsigned i;
    if (argc!=3) return 2;
    if (!setup(argv[2])) { fprintf(stderr,"setup failed\n"); return 2; }
    snprintf(host_game.map.name,sizeof(host_game.map.name),"%s",LEVEL);
    snprintf(join_game.map.name,sizeof(join_game.map.name),"%s",LEVEL);
    as_join();j_mcc_share_game_update(); as_host();h_mcc_share_game_update();
    CHECK(host_missing && join_missing);

    CASE("prompt_overlay") {
        as_join();j_mcc_share_game_render();CHECK(!overlay_draws);
        CHECK(joiner_asks(LEVEL,host_digest));j_mcc_share_game_render();CHECK(overlay_draws==1);
        allow_prompt=0;
        for (i=0;i<300 && !j_mcc_share_game_can_download();i++) pump_once();
        CHECK(j_mcc_share_game_can_download() && !prompts);
        j_mcc_share_game_render();CHECK(overlay_draws==1);
        allow_prompt=1;CHECK(pump_prompt());as_join();j_mcc_share_game_render();CHECK(overlay_draws==1);
        j_mcc_share_game_prompt_action(0);j_mcc_share_game_render();CHECK(overlay_draws==2);
        j_mcc_share_game_disconnect();j_mcc_share_game_render();CHECK(overlay_draws==2);return 0;
    }
    CASE("host_revision") {
        CHECK(transfer_started());as_host();host_revision++;h_mcc_share_game_update();
        CHECK(!h_mcc_share_game_machine_active(MACHINE));as_join();j_mcc_share_game_disconnect();return 0;
    }
    if (!strcmp(argv[1],"shared_resource") || !strcmp(argv[1],"invalid_resource")) {
        resource_shared=!strcmp(argv[1],"shared_resource");resource_invalid=!strcmp(argv[1],"invalid_resource");
        if (resource_shared) {
            unsigned char *data=NULL;unsigned bytes=0;
            as_host();CHECK(read_file("mcc_maps/solar_flare/bitmaps.map",&data,&bytes));
            CHECK(write_text("mcc_maps/bitmaps.map",data,bytes));free(data);
            CHECK(read_file("mcc_maps/solar_flare/sounds.map",&data,&bytes));
            CHECK(write_text("mcc_maps/sounds.map",data,bytes));free(data);
        }
        CHECK(joiner_asks(LEVEL,host_digest));CHECK(pump_prompt() && !j_mcc_share_game_can_download());
        CHECK(accepts==0);as_join();j_mcc_share_game_prompt_action(2);return 0;
    }
    if (!strcmp(argv[1],"unused_resources") || !strcmp(argv[1],"bitmap_only")) {
        resource_mask=!strcmp(argv[1],"unused_resources") ? 0 : 1;as_host();
        CHECK(write_text("mcc_maps/solar_flare/sounds.map",(unsigned char *)"unrelated invalid",17));
        if (!resource_mask) CHECK(write_text("mcc_maps/solar_flare/bitmaps.map",(unsigned char *)"also unrelated",14));
        CHECK(package_digest(LEVEL,host_digest));CHECK(write_list(host_digest,"Example Maker"));
    }
    CASE("workshop_resource_fallback") {
        as_host();CHECK(!mkdir("halo1",0755));CHECK(!mkdir("halo1/maps",0755));
        CHECK(!rename("mcc_maps/solar_flare/bitmaps.map","halo1/maps/bitmaps.map"));
        CHECK(!rename("mcc_maps/solar_flare/sounds.map","halo1/maps/sounds.map"));workshop_fallback=1;
    }
    CASE("gate") {
        char name[128]; map_download_on=0;
        CHECK(joiner_asks(LEVEL,host_digest)); CHECK(to_host_count==0);
        CHECK(pump_prompt() && !j_mcc_share_game_can_download());
        CHECK(strstr(shown,"OFF"));
        as_join();j_mcc_share_game_disconnect(); map_download_on=1;
        CHECK(!joiner_asks("maps\\bloodgulch",host_digest));
        CHECK(!joiner_asks("custom_maps\\bloodgulch",host_digest));
        CHECK(!joiner_asks("mcc_maps\\loose",host_digest));
        as_host();wire_name(name,LEVEL);CHECK(!host_missing(name,host_digest,host_metadata));
        CHECK(to_host_count==0); return 0;
    }
    CASE("consent") {
        CHECK(joiner_asks(LEVEL,host_digest));CHECK(pump_prompt());
        CHECK(j_mcc_share_game_can_download() && j_mcc_share_game_holds_players());
        for (i=0;i<50;i++) pump_once();
        CHECK(accepts==0); snprintf(b,sizeof(b),"%s/mcc_staging",join_dir);CHECK(!exists(b));
        snprintf(b,sizeof(b),"%s/mcc_maps",join_dir);CHECK(!exists(b));
        CHECK(strstr(shown,"Example Maker") && strstr(shown,"kept"));
        CHECK(pump_until_done(2000) && accept_result);return 0;
    }
    if (!strcmp(argv[1],"prompt_map_change") || !strcmp(argv[1],"prompt_disconnect") ||
        !strcmp(argv[1],"prompt_content_change") || !strcmp(argv[1],"prompt_metadata_change") ||
        !strcmp(argv[1],"stale_accept")) {
        CHECK(joiner_asks(LEVEL,host_digest));CHECK(pump_prompt());CHECK(j_mcc_share_game_can_download());
        if (!strcmp(argv[1],"prompt_disconnect")) join_in_game=0;
        else if (!strcmp(argv[1],"prompt_content_change")) join_game.mcc_content_sha256[0]^=1;
        else if (!strcmp(argv[1],"prompt_metadata_change")) join_game.mcc_metadata_sha256[0]^=1;
        else strcpy(join_game.map.name,"maps\\bloodgulch");
        as_join();
        if (!strcmp(argv[1],"stale_accept")) j_mcc_share_game_prompt_action(0);
        else j_mcc_share_game_update();
        CHECK(!j_mcc_share_game_holds_players() && !j_mcc_share_game_can_download());
        CHECK(accepts==0);snprintf(b,sizeof(b),"%s/mcc_staging",join_dir);CHECK(!exists(b));return 0;
    }
    CASE("cancel") {
        CHECK(joiner_asks(LEVEL,host_digest));CHECK(pump_prompt());
        as_join();j_mcc_share_game_prompt_action(2); CHECK(aborts==1 && !j_mcc_share_game_holds_players());
        snprintf(b,sizeof(b),"%s/mcc_staging",join_dir);CHECK(!exists(b)); return 0;
    }
    CASE("workshop_off") { map_share_on=0; }
    CASE("workshop_client_off") { map_download_on=0; }
    if (!strcmp(argv[1],"workshop_off") || !strcmp(argv[1],"workshop_client_off")) {
        CHECK(workshop_asks()); if (!map_download_on) CHECK(to_host_count==0);
        CHECK(pump_prompt() && j_mcc_share_game_has_workshop() && !j_mcc_share_game_can_download());
        as_join();j_mcc_share_game_prompt_action(1);
        CHECK(!strcmp(opened_url,"https://steamcommunity.com/sharedfiles/filedetails/?id=2915693904"));
        CHECK(aborts==1 && accepts==0); snprintf(b,sizeof(b),"%s/mcc_maps",join_dir);CHECK(!exists(b));return 0;
    }
    CASE("off") { map_share_on=0; }
    CASE("unlisted") { as_host();CHECK(!unlink("mcc_share.txt")); }
    CASE("listed_without_maker") { CHECK(write_list(host_digest,"")); }
    CASE("another_package_listed") { unsigned char d[32];memcpy(d,host_digest,32);d[0]^=1;CHECK(write_list(d,"Other")); }
    CASE("unreadable_sidecar") { as_host();CHECK(write_text("mcc_maps/solar_flare/workshop_id.txt",(unsigned char *)"bad id",6)); }
    if (!strcmp(argv[1],"off") || !strcmp(argv[1],"unlisted") || !strcmp(argv[1],"listed_without_maker") ||
        !strcmp(argv[1],"another_package_listed") || !strcmp(argv[1],"unreadable_sidecar")) {
        CHECK(joiner_asks(LEVEL,host_digest));CHECK(pump_prompt()); CHECK(!j_mcc_share_game_can_download() && accepts==0);
        snprintf(b,sizeof(b),"%s/mcc_staging",join_dir);CHECK(!exists(b));
        as_join();j_mcc_share_game_prompt_action(2);CHECK(aborts==1);return 0;
    }
    CASE("disk_space") {
        disk_free=20ull<<20;CHECK(joiner_asks(LEVEL,host_digest));CHECK(pump_prompt());
        if (j_mcc_share_game_can_download()) {as_join();j_mcc_share_game_prompt_action(0);CHECK(pump_prompt());}
        CHECK(strstr(shown,"disk space") && accepts==0);snprintf(b,sizeof(b),"%s/mcc_staging",join_dir);CHECK(!exists(b));return 0;
    }
    CASE("identity_mismatch") { host_digest[3]^=0x40; }
    CASE("metadata_mismatch") { host_metadata[3]^=0x40; }
    if (!strcmp(argv[1],"identity_mismatch") || !strcmp(argv[1],"metadata_mismatch")) {
        /* Identity on the wire differs from the actual staged bytes. The host list still represents original bytes. */
        if (!strcmp(argv[1],"identity_mismatch")) CHECK(write_list(host_digest,"Example Maker"));
        CHECK(joiner_asks(LEVEL,host_digest));CHECK(pump_prompt()); CHECK(j_mcc_share_game_can_download());
        as_join();j_mcc_share_game_prompt_action(0);CHECK(pump_prompt());
        CHECK(!j_mcc_share_game_can_download() && accepts==0);as_join();find_snapshot();CHECK(!snapshot[0]);return 0;
    }
    if (!strcmp(argv[1],"leave") || !strcmp(argv[1],"map_changes") || !strcmp(argv[1],"client_toggle")) {
        CHECK(transfer_started());
        if (!strcmp(argv[1],"leave")) join_in_game=0;
        else if (!strcmp(argv[1],"client_toggle")) map_download_on=0;
        else strcpy(join_game.map.name,"custom_maps\\bloodgulch");
        as_join();j_mcc_share_game_update();
        if (!strcmp(argv[1],"client_toggle")) {
            CHECK(!j_mcc_share_game_can_download());
            CHECK(strstr(shown,"OFF"));
            j_mcc_share_game_prompt_action(2);CHECK(aborts==1);aborts=0;
        }
        CHECK(!j_mcc_share_game_holds_players());
        CHECK(!j_mcc_share_game_downloading(LEVEL));find_snapshot();CHECK(!snapshot[0]);CHECK(!aborts);return 0;
    }
    if (!strcmp(argv[1],"host_toggle") || !strcmp(argv[1],"peer_left")) {
        CHECK(transfer_started()); as_host();
        if (!strcmp(argv[1],"host_toggle")) {map_share_on=0;h_mcc_share_game_update();}
        else {peer_joined=0;h_mcc_share_game_peer_left(&peer);}
        CHECK(!h_mcc_share_game_machine_active(MACHINE));as_join();j_mcc_share_game_disconnect();CHECK(!j_mcc_share_game_holds_players());return 0;
    }
    if (!strcmp(argv[1],"handlers") || !strcmp(argv[1],"not_the_games_package")) {
        struct {struct distributed_message_header h;struct mcc_share_request r;} m;
        memset(&m,0,sizeof(m));m.h.type=103;m.h.count=1;build_message_header(&m.h.header,sizeof(m),2,0);
        m.r.op=MCC_SHARE_OP_MANIFEST;strcpy(m.r.level,LEVEL);
        CHECK(h_mcc_share_game_handles_message((word *)&m,sizeof(m)));
        m.h.type=104;CHECK(h_mcc_share_game_handles_message((word *)&m,sizeof(m)));
        m.h.type=101;CHECK(!h_mcc_share_game_handles_message((word *)&m,sizeof(m)));
        m.h.type=83;CHECK(!h_mcc_share_game_handles_message((word *)&m,sizeof(m)));m.h.type=103;
        CHECK(!h_mcc_share_game_handles_message((word *)&m,4));as_host();
        if (!strcmp(argv[1],"not_the_games_package")) {
            struct mcc_share_response response;
            strcpy(m.r.level,"mcc_maps\\solar_flare\\m1");
            h_mcc_share_game_handle_message(MACHINE,(word *)&m,sizeof(m),1);h_mcc_share_game_update();
            CHECK(to_join_count==1);memcpy(&response,to_join[0].data+sizeof(m.h),sizeof(response));
            CHECK(response.result==MCC_SHARE_RESULT_NOT_SHAREABLE);return 0;
        }
        h_mcc_share_game_handle_message(MACHINE,(word *)&m,sizeof(m),0);
        h_mcc_share_game_handle_message(-1,(word *)&m,sizeof(m),1);
        h_mcc_share_game_handle_message(MACHINE+1,(word *)&m,sizeof(m),1);
        h_mcc_share_game_handle_message(MACHINE,(word *)&m,sizeof(m)-1,1);
        peer_capable=0;h_mcc_share_game_handle_message(MACHINE,(word *)&m,sizeof(m),1);peer_capable=1;
        m.h.count=2;h_mcc_share_game_handle_message(MACHINE,(word *)&m,sizeof(m),1);m.h.count=1;
        m.h.header|=1;h_mcc_share_game_handle_message(MACHINE,(word *)&m,sizeof(m),1);m.h.header&=~1;
        h_mcc_share_game_update();CHECK(to_join_count==0 && !h_mcc_share_game_machine_active(MACHINE));
        as_join();j_mcc_share_game_handle_message(MACHINE,(word *)&m,sizeof(m),1);CHECK(joiner_replies==0 && to_host_count==0);
        /* An otherwise valid offer from a peer rather than our host must not open consent. */
        CHECK(joiner_asks(LEVEL,host_digest));CHECK(to_host_count==1);as_host();
        h_mcc_share_game_handle_message(MACHINE,(word *)to_host[0].data,(word)to_host[0].bytes,1);
        to_host_count=0;
        for (i=0;i<100 && !to_join_count;i++) {h_mcc_share_game_update();clock_ms+=20;}
        CHECK(to_join_count==1);as_join();
        j_mcc_share_game_handle_message(MACHINE,(word *)to_join[0].data,(word)to_join[0].bytes,1);
        j_mcc_share_game_update();CHECK(prompts==0);
        CHECK(pump_prompt() && j_mcc_share_game_can_download());
        as_join();j_mcc_share_game_prompt_action(2);CHECK(aborts==1);
        return 0;
    }
    CASE("existing_version") {
        as_join();CHECK(!mkdir("mcc_maps",0755));CHECK(!mkdir("mcc_maps/solar_flare",0755));
        CHECK(write_text("mcc_maps/solar_flare/m0.map",(unsigned char *)"original",8));
    }
    CASE("map_only") {
        resource_mask=0;as_host();CHECK(!unlink("mcc_maps/solar_flare/ModInfo.json"));CHECK(!unlink("mcc_maps/solar_flare/assets/menu.png"));
        CHECK(!unlink("mcc_maps/solar_flare/bitmaps.map"));CHECK(!unlink("mcc_maps/solar_flare/sounds.map"));
        memset(host_metadata,0,32);CHECK(package_digest(LEVEL,host_digest));CHECK(write_list(host_digest,"Example Maker"));
    }
    CASE("workshop_sidecar") {
        as_host();CHECK(write_text("mcc_maps/solar_flare/workshop_id.txt",(unsigned char *)"2915693904\nFixture\n",19));
    }
    CASE("only_listed_files") { CHECK(host_make("evil.dll",50,8));CHECK(host_make("notes.txt",50,9)); }
    CHECK(joiner_asks(LEVEL,host_digest));CHECK(pump_until_done(2000));CHECK(accept_result && accepts==1 && !aborts && !errors_shown);
    CHECK(lobby_restores==1);
    CHECK(!j_mcc_share_game_holds_players());
    snprintf(a,sizeof(a),"%s/mcc_maps/solar_flare/m0.map",host_dir);
    snprintf(b,sizeof(b),"%s/mcc_maps/%s/m0.map",join_dir,snapshot);CHECK(same_file(a,b));
    if (strcmp(argv[1],"map_only")) {
        static char const *files[]={"bitmaps.map","sounds.map","ModInfo.json","assets/menu.png","readme.txt"};
        for (i=0;i<5;i++) {
            if (i<2 && !(resource_mask&(1u<<i))) {
                snprintf(b,sizeof(b),"%s/mcc_maps/%s/%s",join_dir,snapshot,files[i]);CHECK(!exists(b));continue;
            }
            snprintf(a,sizeof(a),"%s/%s/%s",host_dir,workshop_fallback && i<2 ? "halo1/maps" : "mcc_maps/solar_flare",files[i]);
            snprintf(b,sizeof(b),"%s/mcc_maps/%s/%s",join_dir,snapshot,files[i]);CHECK(same_file(a,b));}
    }
    CASE("workshop_sidecar") {snprintf(b,sizeof(b),"%s/mcc_maps/%s/workshop_id.txt",join_dir,snapshot);CHECK(exists(b));}
    CASE("existing_version") {
        unsigned char *data=NULL;unsigned size=0;as_join();CHECK(read_file("mcc_maps/solar_flare/m0.map",&data,&size));
        CHECK(size==8 && !memcmp(data,"original",8));free(data);
    }
    CASE("only_listed_files") {snprintf(b,sizeof(b),"%s/mcc_maps/%s/evil.dll",join_dir,snapshot);CHECK(!exists(b));
        snprintf(b,sizeof(b),"%s/mcc_maps/%s/notes.txt",join_dir,snapshot);CHECK(!exists(b));}
    for (i=0;i<5;i++) pump_once();CHECK(!h_mcc_share_game_machine_active(MACHINE));
    CHECK(engine_allocation_count==0);
    return 0;
}
