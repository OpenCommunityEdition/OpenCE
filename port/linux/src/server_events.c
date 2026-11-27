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
	/* the text with the players' addresses (NULL: the same as text) */
	char *addressed_text;
};

static struct
{
	pthread_mutex_t lock;
	struct server_event events[SERVER_EVENTS_RING_SIZE];
	unsigned long latest;
	void (*listener)(unsigned long sequence, const char *group, const char *text, const char *addressed_text);
} server_events = { PTHREAD_MUTEX_INITIALIZER };

/* an event's object with its type, number and time put first: NULL if it
is not an object, or out of memory */
static char *event_text(const char *object, unsigned long sequence, long long time)
{
	char head[96];
	size_t length, head_length;
	char *text;

	if (!object || object[0] != '{')
		return NULL;
	length = strlen(object);
	snprintf(head, sizeof(head), "{\"type\":\"event\",\"seq\":%lu,\"time\":%lld%s", sequence, time,
		object[1] == '}' ? "" : ",");
	head_length = strlen(head);
	text = malloc(head_length + length);
	if (!text)
		return NULL;
	memcpy(text, head, head_length);
	memcpy(text + head_length, object + 1, length);
	return text;
}

unsigned long server_event_add_addressed(const char *group, const char *object, const char *addressed_object)
{
	struct server_event *event;
	char *text, *addressed_text = NULL;
	unsigned long sequence;
	long long time = posix_server_unix_milliseconds();
	void (*listener)(unsigned long sequence, const char *group, const char *text, const char *addressed_text);

	if (!object || object[0] != '{')
		return 0;
	pthread_mutex_lock(&server_events.lock);
	sequence = server_events.latest + 1;
	text = event_text(object, sequence, time);
	if (addressed_object)
		addressed_text = event_text(addressed_object, sequence, time);
	if (!text || (addressed_object && !addressed_text))
	{
		pthread_mutex_unlock(&server_events.lock);
		free(text);
		free(addressed_text);
		return 0;
	}
	server_events.latest = sequence;
	event = &server_events.events[sequence % SERVER_EVENTS_RING_SIZE];
	free(event->text);
	free(event->addressed_text);
	event->text = text;
	event->addressed_text = addressed_text;
	event->sequence = sequence;
	snprintf(event->group, sizeof(event->group), "%s", group ? group : "server");
	listener = server_events.listener;
	/* (the listener given the texts while the ring still holds them: under
	the lock, as it copies what it keeps) */
	if (listener)
		listener(sequence, event->group, text, addressed_text);
	pthread_mutex_unlock(&server_events.lock);
	return sequence;
}

unsigned long server_event_add(const char *group, const char *object)
{
	return server_event_add_addressed(group, object, NULL);
}

int server_events_read(unsigned long after, int maximum, int addresses,
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
			each(context, sequence, event->group, addresses && event->addressed_text ? event->addressed_text :
				event->text);
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

void server_events_set_listener(void (*listener)(unsigned long sequence, const char *group, const char *text,
	const char *addressed_text))
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

unsigned long server_event_add_addressed(const char *group, const char *object, const char *addressed_object)
{
	(void)group;
	(void)object;
	(void)addressed_object;
	return 0;
}

int server_events_read(unsigned long after, int maximum, int addresses,
	void (*each)(void *context, unsigned long sequence, const char *group, const char *text), void *context,
	unsigned long *next, unsigned long *missed)
{
	(void)after;
	(void)addresses;
	(void)maximum;
	(void)each;
	(void)context;
	*next = 0;
	*missed = 0;
	return 0;
}

void server_events_set_listener(void (*listener)(unsigned long sequence, const char *group, const char *text,
	const char *addressed_text))
{
	(void)listener;
}

unsigned long server_events_latest(void)
{
	return 0;
}

#endif
