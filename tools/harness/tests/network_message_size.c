/*
NETWORK_MESSAGE_SIZE.C (test)

create_network_game_message (source/networking/network_messages.c) gives the encoder the size of its buffer through a
short that is a member of a union with a long. On the Mac's PowerPC (big-endian) that short is the long's HIGH half, so
setting the long left the short zero: the encoder's first assert (a size above zero) halted the game the first time it
sent a message, searching for a system link game. The statements are taken from the source and run as this host (little
endian) and as a big-endian machine would run them: the union's short rewritten to the long's high half.
*/

#include "harness.h"
#include "snippets.inc"

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";

	CASE("encoder-gets-a-size")
	{
		byte message[12] = { 0 };

		CHECK(create_message_for_test(message) != NULL, "the message was not made (encoder size %d)", seen_size);
		CHECK(seen_size > 0, "the encoder was given a size of %d", seen_size);
	}

	CASE("created-with-the-encoded-size")
	{
		byte message[12] = { 0 };

		CHECK(create_message_for_test(message) != NULL, "the message was not made (encoder size %d)", seen_size);
		CHECK(created_size == ENCODED_SIZE, "the message was made %d long, not %d", created_size, ENCODED_SIZE);
	}

	return 0;
}
