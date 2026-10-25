/*
SERVER_EVENTS.C

A dedicated server's events (server_events.h).
*/

#include "server_events.h"
#include "server_mode.h"

#if HALO_SERVER_BUILD

#include "posix_server.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct server_event
{
	unsigned long sequence;
	char group[SERVER_EVENT_GROUP_SIZE];
	char *text;
};

static struct
{
	pthread_mutex_t lock;
	struct server_event events[SERVER_EVENTS_RING_SIZE];
	unsigned long latest;
	void (*listener)(unsigned long sequence, const char *group, const char *text);
} server_events = { PTHREAD_MUTEX_INITIALIZER };

unsigned long server_event_add(const char *group, const char *object)
{
	struct server_event *event;
	size_t length;
	char head[96];
	char *text;
	unsigned long sequence;
	void (*listener)(unsigned long sequence, const char *group, const char *text);

	if (!object || object[0] != '{')
		return 0;
	length = strlen(object);
	pthread_mutex_lock(&server_events.lock);
	sequence = server_events.latest + 1;
	snprintf(head, sizeof(head), "{\"type\":\"event\",\"seq\":%lu,\"time\":%lld%s", sequence,
		posix_server_unix_milliseconds(), object[1] == '}' ? "" : ",");
	text = malloc(strlen(head) + length);
	if (!text)
	{
		pthread_mutex_unlock(&server_events.lock);
		return 0;
	}
	memcpy(text, head, strlen(head));
	memcpy(text + strlen(head), object + 1, length);
	server_events.latest = sequence;
	event = &server_events.events[sequence % SERVER_EVENTS_RING_SIZE];
	free(event->text);
	event->text = text;
	event->sequence = sequence;
	snprintf(event->group, sizeof(event->group), "%s", group ? group : "server");
	listener = server_events.listener;
	/* (the listener given the text while the ring still holds it: under the
	lock, as it copies what it keeps) */
	if (listener)
		listener(sequence, event->group, text);
	pthread_mutex_unlock(&server_events.lock);
	return sequence;
}

int server_events_read(unsigned long after, int maximum,
	void (*each)(void *context, unsigned long sequence, const char *group, const char *text), void *context,
	unsigned long *next, unsigned long *missed)
{
	unsigned long oldest, sequence;
	int count = 0;

	pthread_mutex_lock(&server_events.lock);
	oldest = server_events.latest >= SERVER_EVENTS_RING_SIZE ? server_events.latest - SERVER_EVENTS_RING_SIZE + 1 : 1;
	*missed = after + 1 < oldest ? oldest - (after + 1) : 0;
	sequence = after + 1 < oldest ? oldest : after + 1;
	for (; sequence <= server_events.latest && count < maximum; sequence++)
	{
		struct server_event const *event = &server_events.events[sequence % SERVER_EVENTS_RING_SIZE];

		if (event->text && event->sequence == sequence)
		{
			each(context, sequence, event->group, event->text);
			count++;
		}
	}
	*next = sequence - 1;
	if (after > server_events.latest)
	{
		*missed = 0;
		*next = server_events.latest;
	}
	pthread_mutex_unlock(&server_events.lock);
	return count;
}

void server_events_set_listener(void (*listener)(unsigned long sequence, const char *group, const char *text))
{
	pthread_mutex_lock(&server_events.lock);
	server_events.listener = listener;
	pthread_mutex_unlock(&server_events.lock);
}

unsigned long server_events_latest(void)
{
	unsigned long latest;

	pthread_mutex_lock(&server_events.lock);
	latest = server_events.latest;
	pthread_mutex_unlock(&server_events.lock);
	return latest;
}

#else

unsigned long server_event_add(const char *group, const char *object)
{
	(void)group;
	(void)object;
	return 0;
}

int server_events_read(unsigned long after, int maximum,
	void (*each)(void *context, unsigned long sequence, const char *group, const char *text), void *context,
	unsigned long *next, unsigned long *missed)
{
	(void)after;
	(void)maximum;
	(void)each;
	(void)context;
	*next = 0;
	*missed = 0;
	return 0;
}

void server_events_set_listener(void (*listener)(unsigned long sequence, const char *group, const char *text))
{
	(void)listener;
}

unsigned long server_events_latest(void)
{
	return 0;
}

#endif
