#ifndef HALO_SESSION_INTERACTIVE_H
#define HALO_SESSION_INTERACTIVE_H

struct mac_session_interactive_stats {
    unsigned frames;
    unsigned ticks;
    unsigned quit_requested;
    /* menu mode: what ended the run (engine_menu.h's MAC_MENU_REQUEST_*, or
       MAC_INTERACTIVE_SCRIPT_DONE when HALO_MAC_MENU_KEYS ran out) */
    unsigned request;
};
#define MAC_INTERACTIVE_SCRIPT_DONE 3

/* Run the initialized local game session through the existing native GPU
   device. visible must match the device selected before rasterizer startup.
   An unbounded run requires visible!=0; hidden runs must set max_frames>0.
   Escape requests quit; other device-loop termination is reported as failure. */
int mac_session_interactive_run(unsigned max_frames, int visible,
    struct mac_session_interactive_stats *stats);

#endif
