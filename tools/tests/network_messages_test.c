/* network_messages_test.c: encodes every network game message through the
game's packet table (source/networking/network_messages.c and
source/memory/data_*.c, linked as built) and prints the bytes each gives the
wire, then decodes the pregame server messages into buffers of exactly their
structure's size. tools/test_network_messages.py builds it with
AddressSanitizer and compares the bytes with a reference captured from the
packet table as upstream ships it, so a change that moves a byte on the wire
fails. Every `long` of the game is 32 bits: int here. */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char byte;

/* ---------- the game's (network_messages.h) */

enum
{
	MESSAGE_TYPE_COUNT = 35,
	/* in network_messages.h's order */
	SERVER_PREGAME_COUNTDOWN = 7,
	SERVER_BEGIN_GAME,
	SERVER_GRACEFUL_GAME_EXIT_PREGAME,
	SERVER_PREGAME_KEEP_ALIVE,
	SERVER_POSTGAME_KEEP_ALIVE,
	PACKET_CLASS_PREGAME = 2,
};

void initialize_network_game_packets(void);
void *create_network_game_message(int message_type, const void *message_struct, short message_struct_size);
byte decode_network_game_message(void *message_struct, const void *encoded_message, short *encoded_message_size,
	short *packet_type, short *packet_version, int expected_packet_class);

/* each message's structure size, by type (network_messages.c's
DEFINE_NETWORK_GAME_MESSAGE): compiled with network_messages.c, the sizes of
the tree under test (tools/test_network_messages.py) */
extern const int network_messages_test_sizes[MESSAGE_TYPE_COUNT];
#define message_sizes network_messages_test_sizes

/* ---------- what the five files need of the rest of the game */

char temporary[256];
static byte captured[0x4000];
static unsigned int captured_size;

void *create_message(int type, const void *data, unsigned int data_size, void *buffer, unsigned short buffer_size)
{
	(void)type;
	if (data_size > sizeof(captured) || data_size > buffer_size)
		return NULL;
	memcpy(captured, data, data_size);
	captured_size = data_size;
	return buffer;
}

void display_assert(char *information, char *file, int line, byte fatal)
{
	fprintf(stderr, "assert %s:%d %s%s\n", file, line, information ? information : "", fatal ? " (fatal)" : "");
}

/* (a release build's assertion: a failure here too) */
void release_assert_failed(const char *information, const char *file, int line, byte fatal)
{
	display_assert((char *)information, (char *)file, line, fatal);
	exit(1);
}

void system_exit(int code)
{
	exit(code ? code : 1);
}

void error(int priority, const char *format, ...)
{
	va_list arguments;

	(void)priority;
	va_start(arguments, format);
	vfprintf(stderr, format, arguments);
	va_end(arguments);
	fputc('\n', stderr);
}

unsigned int system_milliseconds(void) { return 1; }
int halo_linux_vsnprintf(char *buffer, size_t count, const char *format, va_list arguments) { return vsnprintf(buffer, count, format, arguments); }
char *csprintf(char *buffer, char *format, ...)
{
	va_list arguments;
	va_start(arguments, format);
	vsnprintf(buffer, sizeof(temporary), format, arguments);
	va_end(arguments);
	return buffer;
}
void *csmemcpy(void *destination, const void *source, unsigned int size) { return memmove(destination, source, size); }
void *csmemset(void *buffer, int c, unsigned int size) { return memset(buffer, c, size); }
char *csstrcpy(char *destination, const char *source) { return strcpy(destination, source); }
char *csstrncpy(char *s1, const char *s2, unsigned int size) { return strncpy(s1, s2, size); }

/* ---------- the test */

/* a message's structure, alone in its own allocation (AddressSanitizer
reports a read or write a byte past it), filled with a pattern: the pregame
server messages but the keep-alive (sent zero) with values that tell their
bytes apart, the rest zero (the array and data fields carry counts a pattern
would make invalid). NETWORK_MESSAGES_TEST_SLACK=<byte> puts 16 bytes of
that value after each structure instead (to see what an encoder reads past
one, and a decoder writes past one, without AddressSanitizer). */
static byte *new_message(int type)
{
	int size = message_sizes[type];
	const char *slack = getenv("NETWORK_MESSAGES_TEST_SLACK");
	int slack_size = slack ? 16 : 0;
	byte *message = malloc(size + slack_size ? size + slack_size : 1);
	int i;

	memset(message, 0, size);
	if (slack)
		memset(message + size, (int)strtol(slack, NULL, 0), slack_size);
	if (type >= SERVER_PREGAME_COUNTDOWN && type <= SERVER_POSTGAME_KEEP_ALIVE && type != SERVER_PREGAME_KEEP_ALIVE)
	{
		for (i = 0; i < size; i++)
			message[i] = (byte)(0x11 * (type - 6) + i);
	}
	return message;
}

int main(void)
{
	int type;

	initialize_network_game_packets();

	for (type = 0; type < MESSAGE_TYPE_COUNT; type++)
	{
		byte *message = new_message(type);
		unsigned int i;

		captured_size = 0;
		if (!create_network_game_message(type, message, (short)message_sizes[type]))
		{
			printf("%d failed\n", type);
			free(message);
			continue;
		}
		printf("%d", type);
		for (i = 0; i < captured_size; i++)
			printf("%s%02x", i ? "" : " ", captured[i]);
		printf("\n");

		/* the client's side: the pregame server messages decode into exactly
		their structure */
		if (type >= SERVER_PREGAME_COUNTDOWN && type <= SERVER_PREGAME_KEEP_ALIVE)
		{
			byte *decoded = malloc(message_sizes[type] + (getenv("NETWORK_MESSAGES_TEST_SLACK") ? 16 : 0));
			short encoded_size = (short)captured_size;
			short packet_type = (short)type;
			short packet_version = 1;

			memset(decoded, 0, message_sizes[type]);
			if (!decode_network_game_message(decoded, captured, &encoded_size, &packet_type, &packet_version,
				PACKET_CLASS_PREGAME) || packet_type != type)
			{
				printf("%d decode failed\n", type);
			}
			else
			{
				printf("%d decoded", type);
				for (i = 0; i < (unsigned int)message_sizes[type]; i++)
					printf("%s%02x", i ? "" : " ", decoded[i]);
				printf("\n");
			}
			free(decoded);
		}
		free(message);
	}

	return 0;
}
