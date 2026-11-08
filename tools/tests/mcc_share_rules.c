/* Exercises the actual MCC-owned rules; no map assets or network activity. */
#include "mcc_share_rules.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); return 1; } } while (0)
#define CASE(x) if (!strcmp(argv[1],x))

static struct mcc_share_package package(char const *name)
{
    struct mcc_share_package p;
    memset(&p,0,sizeof(p));
    p.name=name;
    p.name_length=name ? strlen(name) : 0;
    p.sidecar=MCC_SHARE_SIDECAR_ABSENT;
    p.content=MCC_SHARE_CONTENT_CUSTOM;
    p.content_known=1;
    p.host_opt_in=1;
    return p;
}

int main(int argc,char **argv)
{
    struct mcc_share_package p=package("mcc_maps/solar_flare/m0");
    unsigned i;
    if (argc!=2) return 2;
    CASE("policy") {
        CHECK(mcc_share_eligible(&p));
        p=package("MCC_MAPS/Solar_Flare/m0");
        CHECK(mcc_share_eligible(&p));
        p=package("mcc_maps\\solar_flare\\m0");
        CHECK(mcc_share_eligible(&p));
        p.host_opt_in=0;
        CHECK(mcc_share_decide(&p)==MCC_SHARE_NOT_OPTED_IN);
        p.host_opt_in=1;p.content_known=0;
        CHECK(mcc_share_decide(&p)==MCC_SHARE_UNKNOWN_CONTENT);
        CHECK(!mcc_share_eligible(NULL));
        memset(&p,0,sizeof(p));
        CHECK(!mcc_share_eligible(&p));
        p=package("mcc_maps/loose");
        CHECK(mcc_share_decide(&p)==MCC_SHARE_NOT_A_PACKAGE);
        p=package("mcc_maps/_SHARED/multiplayer");
        CHECK(mcc_share_decide(&p)==MCC_SHARE_SHARED_RESOURCES);
        return 0;
    }
    CASE("stock") {
        static char const *const names[]={"mcc_maps/bloodgulch/bloodgulch",
            "mcc_maps/retail_copy/bloodgulch", "mcc_maps/custom/a10", "mcc_maps/a10/m0"};
        for (i=0;i<sizeof(names)/sizeof(names[0]);i++) {
            p=package(names[i]);
            CHECK(mcc_share_eligible(&p)); /* Names do not establish stock content. */
            p.content=MCC_SHARE_CONTENT_STOCK;
            CHECK(mcc_share_decide(&p)==MCC_SHARE_STOCK);
            p.content=MCC_SHARE_CONTENT_UNKNOWN;
            CHECK(mcc_share_decide(&p)==MCC_SHARE_UNKNOWN_CONTENT);
        }
        return 0;
    }
    CASE("workshop") {
        static char const *const names[]={"mcc_maps/2915693904/a10","mcc_maps/copied_ruby/a10"};
        for (i=0;i<sizeof(names)/sizeof(names[0]);i++) {
            p=package(names[i]);
            CHECK(mcc_share_eligible(&p)); /* Names and absent sidecars are not permission. */
            p.sidecar=MCC_SHARE_SIDECAR_VALID;
            CHECK(mcc_share_eligible(&p));
            p.host_opt_in=0;
            CHECK(mcc_share_decide(&p)==MCC_SHARE_NOT_OPTED_IN);
            p.host_opt_in=1;p.content_known=0;
            CHECK(mcc_share_decide(&p)==MCC_SHARE_UNKNOWN_CONTENT);
            p.content_known=1;p.content=MCC_SHARE_CONTENT_UNKNOWN;
            CHECK(mcc_share_decide(&p)==MCC_SHARE_UNKNOWN_CONTENT);
            p.content=MCC_SHARE_CONTENT_STOCK;
            CHECK(mcc_share_decide(&p)==MCC_SHARE_STOCK);
            p.content=MCC_SHARE_CONTENT_CUSTOM;p.sidecar=MCC_SHARE_SIDECAR_UNREADABLE;
            CHECK(mcc_share_decide(&p)==MCC_SHARE_UNKNOWN_ORIGIN);
        }
        return 0;
    }
    CASE("states") {
        int state;
        for (state=-128;state<=255;state++) {
            p=package("mcc_maps/custom/level");
            p.sidecar=(enum mcc_share_sidecar)state;
            CHECK(mcc_share_eligible(&p)==(state==MCC_SHARE_SIDECAR_ABSENT || state==MCC_SHARE_SIDECAR_VALID));
            p=package("mcc_maps/custom/level");
            p.content=(enum mcc_share_content)state;
            CHECK(mcc_share_eligible(&p)==(state==MCC_SHARE_CONTENT_CUSTOM));
        }
        return 0;
    }
    CASE("names") {
        static char const *const unsafe[]={"", "maps/x/y", "custom_maps/x/y",
            "mcc_maps/a/../b", "mcc_maps/../b", "mcc_maps/a/..", "mcc_maps/a/b/c",
            "mcc_maps//b", "mcc_maps/a/", "mcc_maps/a:b/c", "mcc_maps/a/b.",
            "mcc_maps/a/b ", "mcc_maps/a/ b", "mcc_maps/.hidden/b", "mcc_maps/a/b\n",
            "mcc_maps/a/b\177", "mcc_maps/a/b\377", "mcc_maps/a/b:stream",
            "mcc_maps/CON/b", "mcc_maps/a/CON", "mcc_maps/a/con.txt", "mcc_maps/Nul/b",
            "mcc_maps/aux .map/b", "mcc_maps/a/PRN", "mcc_maps/com0/b", "mcc_maps/a/LPT9",
            "mcc_maps/a/COM1.foo", "mcc_maps/a/CONIN$", "mcc_maps/conout$/b",
            "mcc_maps/a/bitmaps", "mcc_maps/a/SOUNDS", "mcc_maps/a/loc"};
        static char const *const safe[]={"mcc_maps/console/auxiliary", "mcc_maps/com10/a",
            "mcc_maps/Elite Alpha/Level (v2).final", "mcc_maps/a/con_1", "mcc_maps/a/a.con"};
        for (i=0;i<sizeof(unsafe)/sizeof(unsafe[0]);i++) {
            p=package(unsafe[i]);
            CHECK(mcc_share_decide(&p)==MCC_SHARE_BAD_NAME);
        }
        for (i=0;i<sizeof(safe)/sizeof(safe[0]);i++) {
            p=package(safe[i]);
            CHECK(mcc_share_eligible(&p));
        }
        p=package(NULL);CHECK(!mcc_share_eligible(&p));
        return 0;
    }
    CASE("lengths") {
        char full[64],embedded[]="mcc_maps/a/b\0suffix";
        char const exact[]={'m','c','c','_','m','a','p','s','/','a','/','b'};
        char *allocation;
        memset(full,'x',sizeof(full));memcpy(full,"mcc_maps/a/",11);
        p.name=full;p.name_length=63;
        CHECK(mcc_share_eligible(&p));
        p.name_length=64;CHECK(mcc_share_decide(&p)==MCC_SHARE_BAD_NAME);
        p.name_length=(size_t)-1;CHECK(mcc_share_decide(&p)==MCC_SHARE_BAD_NAME);
        p.name_length=0;CHECK(mcc_share_decide(&p)==MCC_SHARE_BAD_NAME);
        p.name=embedded;p.name_length=sizeof(embedded)-1;
        CHECK(mcc_share_decide(&p)==MCC_SHARE_BAD_NAME);
        allocation=(char *)malloc(sizeof(exact));CHECK(allocation!=NULL);
        memcpy(allocation,exact,sizeof(exact));p.name=allocation;p.name_length=sizeof(exact);
        CHECK(mcc_share_eligible(&p));
        free(allocation);
        return 0;
    }
    CASE("resources") {
        static char const *const unsafe[]={"", "loc.map", "level.map", "workshop_id.txt", "ModInfo.json",
            "a.exe", "a.dll", "a.bat", "a.ps1", "a.sh", "..\\bitmaps.map", "x/sounds.map",
            "C:\\bitmaps.map", "bitmaps.map:stream", "sounds.map.", "sounds.map ", "sounds.map.exe"};
        char embedded[]="sounds.map\0hidden";
        char *exact=(char *)malloc(11);CHECK(exact!=NULL);
        memcpy(exact,"BiTmApS.MaP",11);
        CHECK(mcc_share_file_allowed(exact,11));
        CHECK(!mcc_share_file_allowed(exact,(size_t)-1));
        free(exact);
        CHECK(mcc_share_file_allowed("SOUNDS.MAP",10));
        CHECK(!mcc_share_file_allowed(NULL,10));
        CHECK(!mcc_share_file_allowed(embedded,sizeof(embedded)-1));
        CHECK(!mcc_share_file_allowed("sounds.map",11));
        for (i=0;i<sizeof(unsafe)/sizeof(unsafe[0]);i++)
            CHECK(!mcc_share_file_allowed(unsafe[i],strlen(unsafe[i])));
        return 0;
    }
    CASE("reasons") {
        unsigned j;
        for (i=MCC_SHARE_ELIGIBLE;i<=MCC_SHARE_UNKNOWN_CONTENT;i++) {
            CHECK(strcmp(mcc_share_reason_word((enum mcc_share_reason)i),"unknown"));
            for (j=i+1;j<=MCC_SHARE_UNKNOWN_CONTENT;j++)
                CHECK(strcmp(mcc_share_reason_word((enum mcc_share_reason)i),
                             mcc_share_reason_word((enum mcc_share_reason)j)));
        }
        CHECK(!strcmp(mcc_share_reason_word((enum mcc_share_reason)-1),"unknown"));
        return 0;
    }
    return 2;
}
