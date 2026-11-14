/* ---------- headers */

#include "bungie_net/common/key_agreement.h"

#include "bungie_net/common/message_header.h"
#include "bungie_net/common/public_key_crypt.h"
#include "bungie_net/common/random_numbers.h"
#include "bungie_net/network/transport_endpoint_winsock.h"
#include "memory/data_packet_groups.h"

/* ---------- constants */

enum
{
	KEY_AGREEMENT_ENCODED_PACKET_SIZE = 0x80,
	KEY_AGREEMENT_MESSAGE_BUFFER_SIZE = 0x200,
	KEY_AGREEMENT_PACKET_VERSION = 1,
};

enum key_agreement_packet_type
{
	_key_agreement_packet_type_initiate = 0,
	_key_agreement_packet_type_finalize,
	NUMBER_OF_KEY_AGREEMENT_PACKET_TYPES,
};

/* ---------- macros */

#define KEY_AGREEMENT_FILE "c:\\halo\\SOURCE\\bungie_net\\common\\key_agreement.c"
#define DATA_PACKET_FIELD(type, count) { type, count, 0, 0, 0 }
#define DATA_PACKET_FIELD_END DATA_PACKET_FIELD(_data_packet_field_end, 0)

/* ---------- structures */

struct message_initiate_key_agreement
{
	struct public_key prime;
	struct public_key generator;
	struct public_key key;
};

struct message_finalize_key_agreement
{
	struct public_key key;
};

union key_agreement_packet_value
{
	long value;
	short encoded;
};

/* ---------- prototypes */

static char key_agreement_get_packet_type(
	word const *msgptr);
static boolean key_agreement_decode_packet(
	void *decoded_packet,
	void const *encoded_packet,
	short *encoded_packet_size,
	short *packet_type,
	short *packet_version,
	short expected_packet_class);
static boolean key_agreement_encode_packet(
	void const *decoded_packet,
	void *encoded_packet,
	short *encoded_packet_size,
	short packet_type,
	long packet_version);
static word *key_agreement_build_message(
	short packet_type,
	void const *packet,
	void *buffer,
	word buffer_size);
static word *build_initiate_key_agreement_message(
	struct public_key const *prime,
	struct public_key const *g,
	struct public_key const *key,
	void *buffer,
	word buffer_size);
static word *build_finalize_key_agreement_message(
	struct public_key const *key,
	void *buffer,
	word buffer_size);

/* ---------- globals */

/* Names and types of these six file statics are attested by a later first-party build (the 2003 PC demo PDB's
key_agreement.obj; its packet-array tag is data_packet_group_packet, our data_packet_entry); January's own PDB has
no static names. Their offsets and January's single 4-byte-aligned .data contribution agree with that build. */

static struct data_packet_field message_initiate_key_agreement_packet_fields[4] =
{
	DATA_PACKET_FIELD(_data_packet_field_longs, 2),
	DATA_PACKET_FIELD(_data_packet_field_longs, 2),
	DATA_PACKET_FIELD(_data_packet_field_longs, 2),
	DATA_PACKET_FIELD_END,
};

static struct data_packet_definition message_initiate_key_agreement_packet =
{
	"message_initiate_key_agreement_packet",
	0,
	sizeof(struct message_initiate_key_agreement),
	KEY_AGREEMENT_PACKET_VERSION,
	message_initiate_key_agreement_packet_fields,
	FALSE,
};

static struct data_packet_field message_finalize_key_agreement_packet_fields[2] =
{
	DATA_PACKET_FIELD(_data_packet_field_longs, 2),
	DATA_PACKET_FIELD_END,
};

static struct data_packet_definition message_finalize_key_agreement_packet =
{
	"message_finalize_key_agreement_packet",
	0,
	sizeof(struct message_finalize_key_agreement),
	KEY_AGREEMENT_PACKET_VERSION,
	message_finalize_key_agreement_packet_fields,
	FALSE,
};

static struct data_packet_entry key_agreement_packets_group_packets[NUMBER_OF_KEY_AGREEMENT_PACKET_TYPES] =
{
	{ 0, 0, &message_initiate_key_agreement_packet },
	{ 0, 0, &message_finalize_key_agreement_packet },
};

static struct data_packet_group_definition key_agreement_packets_group =
{
	"key_agreement_packets_group",
	NUMBER_OF_KEY_AGREEMENT_PACKET_TYPES,
	1,
	0x60,
	KEY_AGREEMENT_ENCODED_PACKET_SIZE,
	key_agreement_packets_group_packets,
};

static byte key_agreement_message_buffer[KEY_AGREEMENT_MESSAGE_BUFFER_SIZE];

/* ---------- public code */

long is_message_encryption_key_message(
	word const *msgptr,
	word message_size,
	byte *packet_type)
{
	word message_flags;
	byte message_type;

	match_assert(KEY_AGREEMENT_FILE, 0xC4, msgptr && packet_type);

	message_flags = GET_MESSAGE_FLAGS(*msgptr);
	*packet_type = ((byte const *)msgptr)[message_size - 1];
	message_type = GET_MESSAGE_TYPE(*msgptr);
	if (TEST_FLAG(message_flags, 1) &&
		message_type == _message_type_packet &&
		(*packet_type == _key_agreement_packet_type_initiate ||
		*packet_type == _key_agreement_packet_type_finalize))
	{
		return TRUE;
	}

	return FALSE;
}

boolean initiate_key_exchange(
	struct transport_endpoint *endpoint,
	struct public_key *key,
	struct public_key *prime,
	struct public_key *secret)
{
	boolean success = TRUE;
	struct public_key generator;
	word *message;
	short message_size;

	generate_key_parameters(prime, secret, &generator);
	generate_public_key(prime, secret, &generator, key);
	message = build_initiate_key_agreement_message(
		prime,
		&generator,
		key,
		key_agreement_message_buffer,
		sizeof(key_agreement_message_buffer));
	if (!message)
	{
		success = FALSE;
	}
	else
	{
		message_size = GET_MESSAGE_SIZE(*message);
		byte_swap_message_header(message, _byte_order_network);
		if (write_endpoint(endpoint, message, message_size) != message_size)
			success = FALSE;
	}

	return success;
}

boolean complete_key_exchange(
	struct transport_endpoint *endpoint,
	word const *msgptr,
	struct public_key const *prime,
	struct public_key *secret,
	struct public_key *private_key)
{
	struct message_initiate_key_agreement initiate_packet;
	struct message_finalize_key_agreement finalize_packet;
	struct public_key key;
	short encoded_packet_size;
	short packet_type;
	short packet_version = KEY_AGREEMENT_PACKET_VERSION;
	word *message;
	short message_size;
	byte message_type;

	match_assert(KEY_AGREEMENT_FILE, 0x105, msgptr && prime && secret && private_key);

	message_size = GET_MESSAGE_SIZE(*msgptr);
	encoded_packet_size = message_size - sizeof(word);
	message_type = GET_MESSAGE_TYPE(*msgptr);
	if (message_type == _message_type_packet)
	{
		packet_type = key_agreement_get_packet_type(msgptr);

		switch (packet_type)
		{
		case _key_agreement_packet_type_initiate:
			{
				if (!key_agreement_decode_packet(
					&initiate_packet,
					msgptr + 1,
					&encoded_packet_size,
					&packet_type,
					&packet_version,
					0))
				{
					return FALSE;
				}

				secret->dwords[0] = randomrange(0xFF, initiate_packet.prime.dwords[0] - 2);
				secret->dwords[1] = randomrange(0xFF, initiate_packet.prime.dwords[1] - 2);
				generate_public_key(&initiate_packet.prime, secret, &initiate_packet.generator, &key);
				message = build_finalize_key_agreement_message(
					&key,
					key_agreement_message_buffer,
					sizeof(key_agreement_message_buffer));
				if (!message)
					return FALSE;

				message_size = GET_MESSAGE_SIZE(*message);
				byte_swap_message_header(message, _byte_order_network);
				if (write_endpoint(endpoint, message, message_size) != message_size)
					return FALSE;

				generate_private_key(&initiate_packet.key, &initiate_packet.prime, secret, private_key);
				return TRUE;
			}

		case _key_agreement_packet_type_finalize:
			{
				if (!key_agreement_decode_packet(
					&finalize_packet,
					msgptr + 1,
					&encoded_packet_size,
					&packet_type,
					&packet_version,
					0))
				{
					return FALSE;
				}

				generate_private_key(&finalize_packet.key, prime, secret, private_key);
				return TRUE;
			}
		}
	}

	return FALSE;
}

void initialize_key_agreement_packets(
	void)
{
	data_packet_group_initialize(&key_agreement_packets_group);

	return;
}

/* ---------- private code */

static char key_agreement_get_packet_type(
	word const *msgptr)
{
	byte message_type;
	word message_size = GET_MESSAGE_SIZE(*msgptr);

	match_assert(
		KEY_AGREEMENT_FILE,
		0x4D,
		(message_type= GET_MESSAGE_TYPE(*msgptr)) == _message_type_packet);

	return ((char const *)msgptr)[message_size - 1];
}

static boolean key_agreement_decode_packet(
	void *decoded_packet,
	void const *encoded_packet,
	short *encoded_packet_size,
	short *packet_type,
	short *packet_version,
	short expected_packet_class)
{
	return data_packet_group_decode_packet(
		&key_agreement_packets_group,
		decoded_packet,
		encoded_packet,
		encoded_packet_size,
		packet_type,
		packet_version,
		expected_packet_class);
}

static boolean key_agreement_encode_packet(
	void const *decoded_packet,
	void *encoded_packet,
	short *encoded_packet_size,
	short packet_type,
	long packet_version)
{
	return data_packet_group_encode_packet(
		&key_agreement_packets_group,
		decoded_packet,
		encoded_packet,
		encoded_packet_size,
		packet_type,
		packet_version);
}

static word *key_agreement_build_message(
	short packet_type,
	void const *packet,
	void *buffer,
	word buffer_size)
{
	byte encoded_packet[KEY_AGREEMENT_ENCODED_PACKET_SIZE] = { 0 };
	union key_agreement_packet_value encoded_packet_size;
	word *message = NULL;

	encoded_packet_size.value = sizeof(encoded_packet);
	if (key_agreement_encode_packet(
		packet,
		encoded_packet,
		&encoded_packet_size.encoded,
		packet_type,
		KEY_AGREEMENT_PACKET_VERSION))
	{
		message = create_message(
			_message_type_packet,
			encoded_packet,
			encoded_packet_size.value,
			buffer,
			buffer_size);
		if (message)
			SET_MESSAGE_FLAGS(*message, FLAG(1));
	}

	return message;
}

static word *build_initiate_key_agreement_message(
	struct public_key const *prime,
	struct public_key const *g,
	struct public_key const *key,
	void *buffer,
	word buffer_size)
{
	struct message_initiate_key_agreement packet;

	match_assert(KEY_AGREEMENT_FILE, 0xA2, prime && g && key);

	packet.prime = *prime;
	packet.generator = *g;
	packet.key = *key;

	return key_agreement_build_message(
		_key_agreement_packet_type_initiate,
		&packet,
		buffer,
		buffer_size);
}

static word *build_finalize_key_agreement_message(
	struct public_key const *key,
	void *buffer,
	word buffer_size)
{
	struct message_finalize_key_agreement packet;

	match_assert(KEY_AGREEMENT_FILE, 0xB3, key);

	packet.key = *key;

	return key_agreement_build_message(
		_key_agreement_packet_type_finalize,
		&packet,
		buffer,
		buffer_size);
}
