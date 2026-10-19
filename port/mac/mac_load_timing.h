#ifndef HALO_MAC_LOAD_TIMING_H
#define HALO_MAC_LOAD_TIMING_H

/* port: where a level's load time goes (the Mac build). The clock starts when the first map is opened and the line
   "load timing: ..." is printed when the first frame of the game has been drawn, with the time the cache file took to
   open (inflated or mapped), the tag load (game_load, less the open), the textures' decoding and uploading, and the
   programs' compiling; the rest is "other". The counters are in port/mac/xbox_map.c; they are weak here, as the
   tools that link a user of them do not all link that file, and then nothing is counted. */
/* (no system header here: some of the units that use this are built with the Xbox SDK's names, which clash with <sys/time.h>) */

#define MAC_LOAD_SLOT_CACHE 0   /* mac_xbox_map_open */
#define MAC_LOAD_SLOT_TAGS 1    /* game_load, which includes the open */
#define MAC_LOAD_SLOT_TEXTURES 2
#define MAC_LOAD_SLOT_PROGRAMS 3
#define MAC_LOAD_SLOTS 4
#define MAC_LOAD_TAGS_BEGIN 4   /* (not a time: game_load starts, so that the opens inside it are known) */

#ifdef MAC_LOAD_TIMING_OWNER
double mac_load_clock(void);
#else
extern double mac_load_clock(void) __attribute__((weak));
extern void mac_load_timing_add(int slot, double milliseconds) __attribute__((weak));
extern void mac_load_timing_report(void) __attribute__((weak));

/* the start of a span: 0 when nothing is counting */
static __inline__ double mac_load_begin(void)
{
    return mac_load_timing_add ? mac_load_clock() : 0.0;
}
static __inline__ double mac_load_begin_tags(void)
{
    if (mac_load_timing_add) mac_load_timing_add(MAC_LOAD_TAGS_BEGIN, 0.0);
    return mac_load_begin();
}
static __inline__ void mac_load_end(int slot, double begin)
{
    if (mac_load_timing_add && begin != 0.0) mac_load_timing_add(slot, mac_load_clock() - begin);
}
#endif

#endif
