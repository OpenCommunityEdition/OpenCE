/* The game side of MCC package sharing (mcc_share.c) against stub engine headers: two copies of
 * the real file, one playing the host and one the joiner, each in a folder of its own, exchange
 * the real distributed messages through a queue. The engine's identity probe is replaced by one
 * that fingerprints the package's three files through mcc_package.c's paths (so the staging
 * redirect is exercised). No game, no sockets, no game data. argv[1] is a case, argv[2] an
 * empty folder. */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wchar.h>
#include "mcc_package.h"
#include "mcc_sha256.h"
#include "mcc_share_session.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); return 1; } } while (0)
#define CASE(x) if (!strcmp(argv[1],x))
#define LEVEL "mcc_maps\\solar_flare\\m0"
#define MACHINE 5

/* the engine's types the stub headers declare */
typedef int boolean;
typedef unsigned short word;

/* both copies' entry points, renamed by the build: h_ for the host, j_ for the joiner */
#define COPY(prefix) \
    void prefix##mcc_share_game_update(void); \
    boolean prefix##mcc_share_game_downloading(char const *); \
    boolean prefix##mcc_share_game_holds_players(void); \
    boolean prefix##mcc_share_game_machine_active(long); \
    boolean prefix##mcc_share_game_handles_message(word const *, word); \
    void prefix##mcc_share_game_handle_message(long, word const *, word, boolean);
COPY(h_)
COPY(j_)

static char host_dir[300],join_dir[300];
static unsigned clock_ms=1000;
static int map_share_on,map_download_on,aborts,errors_shown,accepts,accept_result;
static char shown[512];
static int (*host_missing)(char const[128],unsigned char const[32]);
static int (*join_missing)(char const[128],unsigned char const[32]);
static unsigned char host_digest[32];
static int join_in_game=1;
static int installed_taken;                 /* a stub installed map has this package's folder */
static char installed_name[64];

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
void build_message_header(word *header, word size, int type, int flags) { *header=(word)(size<<4|type|flags); }
int game_in_progress(void) { return 0; }
long game_time_get(void) { return 0; }
long interface_get_tag_index(int tag) { (void)tag; return -1; }
void draw_string_set_draw_mode(long a, long b, short c, short d, void const *e) { (void)a;(void)b;(void)c;(void)d;(void)e; }
void rasterizer_draw_string(void *a, void *b, void *c, short d, char const *e) { (void)a;(void)b;(void)c;(void)d;(void)e; }
struct { struct { short window_bounds[4],viewport_bounds[4]; } camera; } render;

void mcc_maps_rescan(void) {}
short mcc_maps_count(boolean multiplayer_only) { (void)multiplayer_only; return installed_taken ? 1 : 0; }
char const *mcc_maps_level_name(short index) { (void)index; return installed_name; }
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
    char path[160];
    unsigned i;
    mcc_sha256_begin(&hash);
    for (i=0;i<3;i++) {
        FILE *f;
        unsigned char block[4096];
        size_t got;
        char *p;
        if (i==0) {
            if (!mcc_package_map_path(level,path)) return 0;
            memmove(path,path+3,strlen(path+3)+1);
            for (p=path;*p;p++) if (*p=='\\') *p='/';
        } else if (!mcc_package_resource_path(level,i,path)) return 0;
        f=fopen(path,"rb");
        if (!f) return 0;
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
void h_mcc_identity_set_missing(int (*handler)(char const[128],unsigned char const[32])) { host_missing=handler; }
void j_mcc_identity_set_missing(int (*handler)(char const[128],unsigned char const[32])) { join_missing=handler; }
int j_mcc_identity_accept_installed(char const name[128], unsigned char const digest[32])
{
    unsigned char actual[32];
    (void)name;
    accepts++;
    accept_result=package_digest(LEVEL,actual) && !memcmp(actual,digest,32);
    return accept_result;
}
int h_mcc_identity_accept_installed(char const name[128], unsigned char const digest[32]) { (void)name;(void)digest;return 1; }

void h_display_error_text_when_main_menu_loaded(wchar_t const *t) { (void)t; }
void j_display_error_text_when_main_menu_loaded(wchar_t const *text)
{
    unsigned i;
    errors_shown++;
    for (i=0;text[i] && i<sizeof(shown)-1;i++) shown[i]=(char)text[i];
    shown[i]=0;
}
void h_network_game_abort(void) {}
void j_network_game_abort(void) { aborts++; }
/* the game record each copy's client holds */
struct network_game { struct { char name[128]; } map; };
static struct network_game host_game,join_game;
struct network_game *h_network_game_client_get_game(void *client) { (void)client;return &host_game; }
struct network_game *j_network_game_client_get_game(void *client) { (void)client;return &join_game; }
void *h_global_network_game_server_get(void) { return (void *)1; }
void *j_global_network_game_server_get(void) { return NULL; }
void *h_global_network_game_client_get(void) { return (void *)1; }
void *j_global_network_game_client_get(void) { return join_in_game ? (void *)1 : NULL; }
int network_connection_last_read_was_unreliable(void) { return 0; }

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
static int joiner_replies;
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
    char path[400];
    unsigned char *data=make_bytes(size,seed);
    int ok;
    snprintf(path,sizeof(path),"%s/mcc_maps/solar_flare/%s",host_dir,leaf);
    ok=write_text(path,data,size);
    free(data);
    return ok;
}

static void as_host(void) { if (chdir(host_dir)) abort(); }
static void as_join(void) { if (chdir(join_dir)) abort(); }

static int setup(char const *base)
{
    char path[400];
    unsigned char list[200];
    char hex[65];
    unsigned i;
    snprintf(host_dir,sizeof(host_dir),"%s/host",base);
    snprintf(join_dir,sizeof(join_dir),"%s/join",base);
    if (mkdir(host_dir,0755) || mkdir(join_dir,0755)) return 0;
    snprintf(path,sizeof(path),"%s/mcc_maps",host_dir);if (mkdir(path,0755)) return 0;
    snprintf(path,sizeof(path),"%s/mcc_maps/solar_flare",host_dir);if (mkdir(path,0755)) return 0;
    if (!host_make("m0.map",15483,1) || !host_make("bitmaps.map",7000,2) || !host_make("sounds.map",6144,3)) return 0;
    if (!host_make("ModInfo.json",120,4) || !host_make("readme.txt",300,5)) return 0;
    as_host();
    if (!package_digest(LEVEL,host_digest)) return 0;
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

/* until the joiner has been told the outcome */
static int pump_until_done(int limit)
{
    int i,before=accepts+aborts;
    for (i=0;i<limit && accepts+aborts==before;i++) pump_once();
    return accepts+aborts>before;
}

static void wire_name(char name[128], char const *level)
{
    memset(name,0,128);
    snprintf(name,64,"%s",level);
}

static int joiner_asks(char const *level, unsigned char const digest[32])
{
    char name[128];
    wire_name(name,level);
    as_join();
    return join_missing(name,digest);
}

int main(int argc,char **argv)
{
    char a[500],b[500];
    unsigned i;
    int steps;
    if (argc!=3) return 2;
    if (!setup(argv[2])) {fprintf(stderr,"setup failed\n");return 2;}
    snprintf(host_game.map.name,sizeof(host_game.map.name),"%s",LEVEL);
    snprintf(join_game.map.name,sizeof(join_game.map.name),"%s",LEVEL);
    as_join();j_mcc_share_game_update();                                    /* each copy starts and registers its handler */
    as_host();h_mcc_share_game_update();
    CHECK(join_missing && host_missing);
    snprintf(installed_name,sizeof(installed_name),"%s","mcc_maps\\other_pack\\x");

    CASE("transfer") {
        char staged[400];
        CHECK(joiner_asks(LEVEL,host_digest));
        CHECK(j_mcc_share_game_downloading("MCC_MAPS/Solar_Flare/M0") && j_mcc_share_game_holds_players());
        CHECK(!j_mcc_share_game_downloading("mcc_maps\\solar_flare\\m1"));
        CHECK(!h_mcc_share_game_downloading(LEVEL));
        for (steps=0;steps<6;steps++) pump_once();
        CHECK(h_mcc_share_game_machine_active(MACHINE));                     /* the host holds the joiner's slot */
        snprintf(staged,sizeof(staged),"%s/mcc_staging/solar_flare",join_dir);
        CHECK(pump_until_done(2000));
        CHECK(accepts==1 && accept_result && aborts==0 && errors_shown==0);
        CHECK(!j_mcc_share_game_downloading(LEVEL) && !j_mcc_share_game_holds_players());
        for (i=0;i<5;i++) pump_once();
        CHECK(!h_mcc_share_game_machine_active(MACHINE));
        {
            static char const *const files[]={"m0.map","bitmaps.map","sounds.map","ModInfo.json","readme.txt"};
            for (i=0;i<5;i++) {
                snprintf(a,sizeof(a),"%s/mcc_maps/solar_flare/%s",host_dir,files[i]);
                snprintf(b,sizeof(b),"%s/mcc_maps/solar_flare/%s",join_dir,files[i]);
                CHECK(same_file(a,b));
            }
        }
        CHECK(!exists(staged));
        return 0;
    }

    CASE("only_listed_files") {
        /* files of a package folder that are not the map, its resource maps or credit files are never offered */
        static char const *const extra[]={"evil.dll","loc.map","notes.txt","Thumbs.db","bitmaps.map.part","..hidden"};
        for (i=0;i<sizeof(extra)/sizeof(extra[0]);i++) CHECK(host_make(extra[i],50,9));
        CHECK(joiner_asks(LEVEL,host_digest));
        CHECK(pump_until_done(2000) && accept_result);
        for (i=0;i<sizeof(extra)/sizeof(extra[0]);i++) {
            snprintf(b,sizeof(b),"%s/mcc_maps/solar_flare/%s",join_dir,extra[i]);
            CHECK(!exists(b));
        }
        return 0;
    }

    CASE("off") {
        map_share_on=0;
        CHECK(joiner_asks(LEVEL,host_digest));
        CHECK(pump_until_done(200));
        CHECK(aborts==1 && accepts==0 && strstr(shown,"does not send"));
        snprintf(b,sizeof(b),"%s/mcc_maps",join_dir);
        CHECK(!exists(b));
        CHECK(!j_mcc_share_game_holds_players());
        return 0;
    }

    CASE("unlisted") {
        as_host();
        CHECK(unlink("mcc_share.txt")==0);                                  /* no list: unknown content */
        CHECK(joiner_asks(LEVEL,host_digest));
        CHECK(pump_until_done(200));
        CHECK(aborts==1 && strstr(shown,"cannot send"));
        snprintf(b,sizeof(b),"%s/mcc_maps",join_dir);CHECK(!exists(b));
        return 0;
    }

    CASE("listed_without_maker") {
        char hex[65];
        unsigned char text[100];
        for (i=0;i<32;i++) snprintf(hex+i*2,3,"%02x",host_digest[i]);
        snprintf((char *)text,sizeof(text),"%s\n",hex);
        as_host();
        CHECK(write_text("mcc_share.txt",text,(unsigned)strlen((char *)text)));
        CHECK(joiner_asks(LEVEL,host_digest));
        CHECK(pump_until_done(200));
        CHECK(aborts==1 && strstr(shown,"maker credit") && strstr(shown,"its maker"));
        return 0;
    }

    CASE("another_package_listed") {
        /* a list line for other content does not classify this package */
        char hex[65];
        unsigned char text[200];
        unsigned char other[32];
        memcpy(other,host_digest,32);other[31]^=1;
        for (i=0;i<32;i++) snprintf(hex+i*2,3,"%02x",other[i]);
        snprintf((char *)text,sizeof(text),"%s Somebody\n",hex);
        as_host();
        CHECK(write_text("mcc_share.txt",text,(unsigned)strlen((char *)text)));
        CHECK(joiner_asks(LEVEL,host_digest));
        CHECK(pump_until_done(200));
        CHECK(aborts==1 && strstr(shown,"cannot send"));
        return 0;
    }

    CASE("workshop_sidecar") {
        unsigned char text[]="2915693904\nSome Map\n";
        snprintf(a,sizeof(a),"%s/mcc_maps/solar_flare/workshop_id.txt",host_dir);
        CHECK(write_text(a,text,sizeof(text)-1));
        CHECK(joiner_asks(LEVEL,host_digest));
        CHECK(pump_until_done(200));
        CHECK(aborts==1 && strstr(shown,"Steam Workshop"));
        snprintf(b,sizeof(b),"%s/mcc_maps",join_dir);CHECK(!exists(b));
        return 0;
    }

    CASE("unreadable_sidecar") {
        snprintf(a,sizeof(a),"%s/mcc_maps/solar_flare/workshop_id.txt",host_dir);
        CHECK(write_text(a,(unsigned char const *)"not an id",9));
        CHECK(joiner_asks(LEVEL,host_digest));
        CHECK(pump_until_done(200));
        CHECK(aborts==1 && strstr(shown,"cannot send"));
        return 0;
    }

    CASE("not_the_games_package") {
        /* a joiner asking for another package than the host's game gets nothing */
        struct { word header; unsigned char type,count; int time; struct mcc_share_request r; } m;
        memset(&m,0,sizeof(m));
        m.type=102;m.count=1;
        m.r.op=MCC_SHARE_OP_MANIFEST;
        snprintf(m.r.level,sizeof(m.r.level),"mcc_maps\\solar_flare\\m1");
        as_host();
        h_mcc_share_game_handle_message(MACHINE,(word const *)&m,(word)sizeof(m),1);
        h_mcc_share_game_update();
        CHECK(to_join_count==1);
        {
            struct mcc_share_response head;
            memcpy(&head,to_join[0].data+8,sizeof(head));
            CHECK(head.op==MCC_SHARE_OP_MANIFEST && head.result==MCC_SHARE_RESULT_NOT_SHAREABLE && head.count==0);
        }
        return 0;
    }

    CASE("gate") {
        /* the joiner does not ask: the setting is off, the host is a Workshop item, the name is not a
         * shareable package, a folder of that name is installed */
        char name[128];
        map_download_on=0;
        CHECK(!joiner_asks(LEVEL,host_digest));
        map_download_on=1;
        wire_name(name,LEVEL);
        memcpy(name+64,"WS\1" "2915693904\0Title\0",18);
        as_join();
        CHECK(!join_missing(name,host_digest));
        CHECK(!joiner_asks("mcc_maps\\2915693904\\a10",host_digest));
        CHECK(!joiner_asks("mcc_maps\\loose",host_digest));
        CHECK(!joiner_asks("mcc_maps\\_shared\\multiplayer",host_digest));
        CHECK(!joiner_asks("mcc_maps\\a\\CON",host_digest));
        CHECK(!joiner_asks("maps\\bloodgulch",host_digest));
        installed_taken=1;snprintf(installed_name,sizeof(installed_name),"%s","mcc_maps\\Solar_Flare\\other");
        CHECK(!joiner_asks(LEVEL,host_digest));
        installed_taken=0;
        CHECK(to_host_count==0 && !j_mcc_share_game_holds_players());
        /* the host itself never fetches */
        as_host();
        wire_name(name,LEVEL);
        CHECK(!host_missing(name,host_digest));
        return 0;
    }

    CASE("identity_mismatch") {
        unsigned char wrong[32];
        memcpy(wrong,host_digest,32);wrong[3]^=0x40;
        CHECK(joiner_asks(LEVEL,wrong));
        CHECK(pump_until_done(2000));
        CHECK(aborts==1 && accepts==0 && strstr(shown,"identity") && strstr(shown,"Nothing was installed"));
        snprintf(b,sizeof(b),"%s/mcc_maps/solar_flare",join_dir);CHECK(!exists(b));
        snprintf(b,sizeof(b),"%s/mcc_staging/solar_flare",join_dir);CHECK(!exists(b));
        return 0;
    }

    CASE("handlers") {
        struct { word header; unsigned char type,count; int time; struct mcc_share_request r; } m;
        struct { word header; unsigned char type,count; int time; struct mcc_share_response r; } n;
        memset(&m,0,sizeof(m));m.type=102;m.count=1;m.r.op=MCC_SHARE_OP_MANIFEST;
        snprintf(m.r.level,sizeof(m.r.level),"%s",LEVEL);
        CHECK(h_mcc_share_game_handles_message((word const *)&m,(word)sizeof(m)));
        CHECK(!h_mcc_share_game_handles_message((word const *)&m,4));
        m.type=103;CHECK(h_mcc_share_game_handles_message((word const *)&m,(word)sizeof(m)));
        m.type=100;CHECK(!h_mcc_share_game_handles_message((word const *)&m,(word)sizeof(m)));          /* the join envelopes' kinds */
        m.type=101;CHECK(!h_mcc_share_game_handles_message((word const *)&m,(word)sizeof(m)));
        m.type=99;CHECK(!h_mcc_share_game_handles_message((word const *)&m,(word)sizeof(m)));
        m.type=104;CHECK(!h_mcc_share_game_handles_message((word const *)&m,(word)sizeof(m)));
        m.type=83;CHECK(!h_mcc_share_game_handles_message((word const *)&m,(word)sizeof(m)));      /* Custom Edition's are not ours */
        m.type=102;
        as_host();
        /* a datagram (not from the stream), from no machine, or short: ignored */
        h_mcc_share_game_handle_message(MACHINE,(word const *)&m,(word)sizeof(m),0);
        h_mcc_share_game_handle_message(-1,(word const *)&m,(word)sizeof(m),1);
        h_mcc_share_game_handle_message(MACHINE,(word const *)&m,(word)(sizeof(m)-1),1);
        for (i=0;i<20;i++) {h_mcc_share_game_update();clock_ms+=20;}
        CHECK(to_join_count==0 && !h_mcc_share_game_machine_active(MACHINE));
        /* a request to a joiner (it is not a host) is ignored */
        as_join();
        j_mcc_share_game_handle_message(MACHINE,(word const *)&m,(word)sizeof(m),1);
        CHECK(to_join_count==0 && to_host_count==0 && joiner_replies==0);
        /* a response to a machine that asked for nothing */
        memset(&n,0,sizeof(n));n.type=103;n.count=1;n.r.op=MCC_SHARE_OP_MANIFEST;
        snprintf(n.r.level,sizeof(n.r.level),"%s",LEVEL);
        j_mcc_share_game_handle_message(-1,(word const *)&n,(word)sizeof(n),1);
        CHECK(accepts==0 && aborts==0 && !j_mcc_share_game_holds_players());
        return 0;
    }

    CASE("map_changes") {
        /* the host changes the map during the download: the old one is dropped, nothing is installed */
        char staged[400];
        CHECK(joiner_asks(LEVEL,host_digest));
        for (steps=0;steps<4;steps++) pump_once();
        snprintf(staged,sizeof(staged),"%s/mcc_staging/solar_flare",join_dir);
        CHECK(exists(staged) && j_mcc_share_game_holds_players());
        snprintf(join_game.map.name,sizeof(join_game.map.name),"%s","mcc_maps\\solar_flare\\m1");
        as_join();j_mcc_share_game_update();
        CHECK(!exists(staged) && !j_mcc_share_game_holds_players());
        snprintf(join_game.map.name,sizeof(join_game.map.name),"%s","bloodgulch");
        as_join();j_mcc_share_game_update();
        for (steps=0;steps<30;steps++) pump_once();
        snprintf(b,sizeof(b),"%s/mcc_maps/solar_flare",join_dir);CHECK(!exists(b));
        CHECK(accepts==0 && aborts==0 && errors_shown==0);
        return 0;
    }

    CASE("disk_space") {
        disk_free=20ull<<20;                                                /* less than the files and the room kept */
        CHECK(joiner_asks(LEVEL,host_digest));
        CHECK(pump_until_done(200));
        CHECK(aborts==1 && strstr(shown,"not enough free disk space"));
        snprintf(b,sizeof(b),"%s/mcc_staging",join_dir);CHECK(!exists(b));
        snprintf(b,sizeof(b),"%s/mcc_maps",join_dir);CHECK(!exists(b));
        return 0;
    }

    CASE("leave") {
        /* the game is left during the download: nothing of the download stays */
        char staged[400];
        CHECK(joiner_asks(LEVEL,host_digest));
        for (steps=0;steps<4;steps++) pump_once();
        snprintf(staged,sizeof(staged),"%s/mcc_staging/solar_flare",join_dir);
        CHECK(exists(staged));
        CHECK(j_mcc_share_game_holds_players());
        join_in_game=0;
        as_join();j_mcc_share_game_update();
        CHECK(!exists(staged) && !j_mcc_share_game_holds_players() && !j_mcc_share_game_downloading(LEVEL));
        snprintf(b,sizeof(b),"%s/mcc_maps/solar_flare",join_dir);CHECK(!exists(b));
        CHECK(aborts==0 && errors_shown==0);                           /* leaving is not an error to show */
        /* the host drops its side when the joiner has said nothing for a while */
        clock_ms+=31000;
        as_host();h_mcc_share_game_update();
        CHECK(!h_mcc_share_game_machine_active(MACHINE));
        return 0;
    }

    return 2;
}
