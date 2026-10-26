/*
SERVER_EVENTS.H

A dedicated server's events (server_events.c; docs/dedicated_server.md,
"Events"): what happens to the game, its players and the server, each a
JSON object numbered in order, kept in a ring of the last
SERVER_EVENTS_RING_SIZE for the remote console's subscribers and
/api/events. Any thread may add one.
*/

#ifndef __HALO_LINUX_SERVER_EVENTS_H
#define __HALO_LINUX_SERVER_EVENTS_H

enum
{
	SERVER_EVENTS_RING_SIZE = 512,
	SERVER_EVENT_GROUP_SIZE = 16,
};

/* adds an event of a group ("player", "chat", "game", "moderation",
"status", "log", "master", "server") from its object's text ({"event":
"player_joined",...}): its sequence number and time are put in it. The
event's sequence number (0 if it was not kept) */
unsigned long server_event_add(const char *group, const char *object);
/* the same, with a second text of it, addressed_object, for the readers who
see players' addresses (the owner, or every role with
server.log_addresses): the same event, under the same number, with its
address and hardware id */
unsigned long server_event_add_addressed(const char *group, const char *object, const char *addressed_object);
/* each event after the sequence number after (0: every one kept), in order:
the callback gets its number, group and text (its addressed text, if it
has one, for a reader with addresses); how many. next is the number to ask
after next time, missed how many had left the ring */
int server_events_read(unsigned long after, int maximum, int addresses,
	void (*each)(void *context, unsigned long sequence, const char *group, const char *text), void *context,
	unsigned long *next, unsigned long *missed);
/* each event as it is added (the remote console's), on the adder's thread:
its text and its addressed text (NULL for none) */
void server_events_set_listener(void (*listener)(unsigned long sequence, const char *group, const char *text,
	const char *addressed_text));
/* the latest event's number */
unsigned long server_events_latest(void);

#endif
