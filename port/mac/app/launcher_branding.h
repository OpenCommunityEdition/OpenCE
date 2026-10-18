/* What a build adds to the About and Send Feedback windows without the repository holding it: a publisher's
   paragraph, link buttons, a logo, and the feedback endpoint. They are in Contents/Resources/branding.plist
   (and the files it names beside it), which tools/mac_app_bundle.sh --branding copies in. Without the file
   the build is neutral: the OpenCE project's link only, and no Send Feedback. Core Foundation, 10.4 and later. */
#ifndef LAUNCHER_BRANDING_H
#define LAUNCHER_BRANDING_H

#include <limits.h>

#define LB_LINKS 3

struct lb_link
{
	char title[64], url[256];
};

struct lb_branding
{
	char publisher[1200];		/* a paragraph about the build's maker, or empty */
	struct lb_link links[LB_LINKS];
	int link_count;
	char feedback_url[256];		/* empty: no feedback */
	char feedback_app[64];		/* the report's "app" field */
	char feedback_token[64];	/* sent as X-Feedback-Client */
	char logo_path[PATH_MAX];	/* a PNG, or empty */
	char ca_path[PATH_MAX];		/* the CA list that the feedback's HTTPS is checked with */
	char sender_path[PATH_MAX];	/* Contents/MacOS/PowerSpark-send; set by the caller */
	char update_feed[256];		/* the appcast's https address, or empty: no updates (UpdateFeedURL) */
	char update_key[64];		/* the release key's public half, base64 (UpdatePublicKey) */
	char updater_path[PATH_MAX];	/* Contents/MacOS/PowerSpark-update; set by the caller */
};

/* the neutral build */
void lb_defaults(struct lb_branding *b);
/* reads resources/branding.plist if it is there (HALO_MAC_BRANDING names another plist, whose folder is then
   the resources); returns 1 if a file was read. Only http and https addresses are kept, and a file named
   by the plist must be a plain name in the same folder. */
int lb_load(struct lb_branding *b, const char *resources);
/* feedback can be sent: an https address, a CA list and the sender are all there */
int lb_feedback_ready(const struct lb_branding *b);
/* updates can be looked for: an https feed, a 32-byte release key, a CA list and the downloader are all there */
int lb_update_ready(const struct lb_branding *b);

#endif
