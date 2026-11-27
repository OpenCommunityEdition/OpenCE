/*
SERVER_HTTP.H

A dedicated server's web port (server_http.c; docs/dedicated_server.md,
"HTTP" and "The RCON"): the admin page, the JSON API, the remote console
over WebSocket, /healthz and /metrics, served on a thread of their own so
that they answer while the game loads a map. What needs the game (a
command, a change) is handed to the game's thread, which runs it between
frames (server_http_main_thread); what only reads it is answered from the
snapshots that thread leaves.
*/

#ifndef __HALO_LINUX_SERVER_HTTP_H
#define __HALO_LINUX_SERVER_HTTP_H

/* the port bound and its thread started (server.web_port; nothing for 0):
0 if the port is taken */
int server_http_start(void);
/* the game's thread, each frame: the requests waiting for it run, and the
snapshots refreshed */
void server_http_main_thread(void);

struct server_json;

/* the server's capabilities (the RCON's hello's, the status's) as an array
of the key */
void server_http_capabilities(struct server_json *json, const char *key);

#endif
