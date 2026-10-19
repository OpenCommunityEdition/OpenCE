"""create_network_game_message's size of the encoded message, run as a little-endian and as a big-endian machine runs it
(the Mac's PowerPC): the encoder takes a short that is a union member beside a long, which on a big-endian machine is the
long's high half. The negative control puts the old statements back, which the big-endian run must catch."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["encoder-gets-a-size", "created-with-the-encoded-size"]

PRELUDE = """
typedef unsigned short word;
#define ENCODED_SIZE 20
#define HALO_PORT_MAXIMUM_NETWORK_MESSAGE_SIZE 1400
#define HALO_PORT_NETWORK_PACKET_SIZE 1400
/* a short * to a long on a big-endian machine points at its high half */
#define BE_SHORT_OF_LONG(p) ((short *)((byte *)(p) + 2))
enum network_game_message_type { _message_test };
static int seen_size;
static int created_size;
static byte network_game_message_buffer[2048];
static boolean encode_network_game_message(const void *message_struct, void *encoded_message, short *encoded_message_size,
	enum network_game_message_type message_type, long message_version)
{
	seen_size = *encoded_message_size;
	if (seen_size <= 0)
		return 0;
	*encoded_message_size = ENCODED_SIZE;
	return 1;
}
static void *create_message(long a, void *data, long size, void *buffer, long buffer_size)
{
	created_size = (int)size;
	return buffer;
}
#define network_event(...) ((void)0)
"""


def snippets(big_endian, old=False):
    source = read("source/networking/network_messages.c")
    union = source[source.index("union network_game_message_size\n{"):]
    union = union[:union.index("};") + 2]
    body = function(source, "create_network_game_message")
    start = body.index("/* port: the encoder reads")
    setup = body[start:body.index("switch (", start)]
    use = body[body.index("if (encode_network_game_message("):body.index("else\n\t{\n\t\tnetwork_event(\"encode_network")]
    if old:
        setup = "encoded_message_size.value = sizeof(encoded_message);\n"
        use = use.replace("encoded_message_size.encoded + sizeof(word)", "encoded_message_size.value + sizeof(word)")
        use = use.replace("encode_network_game_message(message_struct, encoded_message, &encoded_message_size.encoded",
                          "encode_network_game_message(message_struct, encoded_message, &encoded_message_size.encoded")
        use = use.replace("encoded, network_game_message_buffer", "encoded, network_game_message_buffer")
        use = use.replace("encoded_message_size.encoded, network_game_message_buffer",
                          "encoded_message_size.value, network_game_message_buffer")
    code = setup + use
    if big_endian:
        code = code.replace("&encoded_message_size.encoded", "BE_SHORT_OF_LONG(&encoded_message_size.value)")
        code = code.replace("encoded_message_size.encoded", "(*BE_SHORT_OF_LONG(&encoded_message_size.value))")
    return union, code


def generated(big_endian, old=False):
    union, code = snippets(big_endian, old)
    text = PRELUDE + union + """
static void *create_message_for_test(const byte *message_struct)
{
	byte encoded_message[HALO_PORT_NETWORK_PACKET_SIZE];
	union network_game_message_size encoded_message_size;
	enum network_game_message_type message_type = _message_test;
	void *message = NULL;

	""" + code + """
	return message;
}
"""
    return (("snippets.inc", text),)


@pytest.mark.parametrize("big_endian", [False, True])
@pytest.mark.parametrize("case", CASES)
def test_case(case, big_endian):
    status, output = run(build("network_message_size", generated(big_endian)), case)
    assert status == 0, output


@pytest.mark.parametrize("case", CASES)
def test_negative_control_old_statements_fail_big_endian(case):
    status, output = run(build("network_message_size", generated(True, old=True)), case)
    assert status == CHECK_FAILED, f"the old statements passed '{case}' as a big-endian machine runs them: {output}"


def test_old_statements_pass_little_endian():
    """The fix changes nothing on a little-endian host."""
    status, output = run(build("network_message_size", generated(False, old=True)), "encoder-gets-a-size")
    assert status == 0, output
