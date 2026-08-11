/*
 * Synthetic XDK 3911 allocator fixture for data_packets::_code_0010a7c0.
 *
 * This file deliberately models only the live-range/call topology around the
 * data, raw, and array cases.  It is not production code.  Compile with
 * /DVARIANT=N, N=0..19.  Each variant changes one readable-C factor.
 */

#ifndef VARIANT
#define VARIANT 0
#endif

typedef unsigned char byte;

enum field_type
{
	field_pad = 0,
	field_bytes,
	field_shorts,
	field_longs,
	field_int64s,
	field_string,
	field_data,
	field_array,
	field_raw,
	field_end
};

struct field
{
	short type;
	short count;
	short minimum_version;
	short maximum_version;
	short size;
};

struct packet_definition
{
	void *name;
	long flags;
	short size;
	short version;
	struct field *fields;
	char initialized;
};

struct encoding_state
{
	byte storage[20];
};

void verify_fields(
	struct packet_definition *packet_definition,
	short *packet_size,
	struct field *fields,
	short *field_count);

void encode_integer(
	struct encoding_state *state,
	long value,
	long maximum);

void encode_memory(
	struct encoding_state *state,
	void const *source,
	long count,
	long byte_order);

void encode_string(
	struct encoding_state *state,
	char const *string,
	long maximum);

void report_invalid(
	long line);

void fixture(
	struct packet_definition *packet_definition,
	struct encoding_state *state,
	short packet_version,
	void const *decoded_packet,
	short *encoded_packet_size,
	struct field *fields,
	short *field_count)
{
	struct field *field;
	byte const *decoded_data;
	byte const *decoded_start;
#if VARIANT == 19
	struct packet_definition *packet = packet_definition;
#endif

	field = fields;
	decoded_data = decoded_packet;
	decoded_start = decoded_data;
	while (field->type != field_end)
	{
		if (packet_version >= field->minimum_version &&
			(packet_version <= field->maximum_version || field->maximum_version == 0))
		{
			switch (field->type)
			{
			case field_pad:
				break;
			case field_bytes:
				encode_memory(state, decoded_data, field->count, 1);
				break;
			case field_shorts:
				encode_memory(state, decoded_data, field->count, -2);
				break;
			case field_longs:
				encode_memory(state, decoded_data, field->count, -4);
				break;
			case field_int64s:
				encode_memory(state, decoded_data, field->count, -8);
				break;
			case field_string:
				encode_string(state, (char const *)decoded_data, field->count);
				break;
			case field_data:
			{
				short data_size = *(short const *)decoded_data;
#if VARIANT == 8 || VARIANT == 9 || VARIANT == 10
				byte const *data_pointer;
#endif

#if VARIANT == 8
				data_pointer = decoded_data + sizeof(short);
#endif
				if (data_size < 0 || data_size > field->count)
					report_invalid(253);
				if (data_size < 0 || data_size > field->count)
					data_size = 0;
#if VARIANT == 10
				encode_integer((data_pointer = decoded_data + sizeof(short), state), data_size, field->count);
#else
				encode_integer(state, data_size, field->count);
#endif
#if VARIANT == 9
				data_pointer = decoded_data + sizeof(short);
#endif
#if VARIANT == 8 || VARIANT == 9 || VARIANT == 10
				encode_memory(state, data_pointer, data_size, 1);
#elif VARIANT == 12
				encode_memory(state, (short const *)decoded_data + 1, data_size, 1);
#else
				encode_memory(state, decoded_data + sizeof(short), data_size, 1);
#endif
				break;
			}
			case field_raw:
#if VARIANT == 15
			{
				short raw_count = field->count;
				encode_memory(state, decoded_data, raw_count, 1);
				break;
			}
#else
				encode_memory(state, decoded_data, field->count, 1);
				break;
#endif
			case field_array:
			{
				short element_field_count;
				byte const *element;
				short element_count;
#if VARIANT == 6 || VARIANT == 7
				struct packet_definition *packet;
#endif
#if VARIANT == 13
				struct element_cursor
				{
					byte const *pointer;
				} cursor;
#endif
#if VARIANT == 14
				byte const *element_base;
#endif

#if VARIANT == 0 || VARIANT == 6 || VARIANT == 7 || VARIANT == 16
				element = decoded_data + sizeof(short);
				element_count = *(short const *)decoded_data;
#elif VARIANT == 4
				element_count = *(short const *)decoded_data;
				element = decoded_data + sizeof(short);
#elif VARIANT == 11
				element = decoded_data;
				element += sizeof(short);
				element_count = *(short const *)decoded_data;
#elif VARIANT == 12
				element = (byte const *)((short const *)decoded_data + 1);
				element_count = *(short const *)decoded_data;
#elif VARIANT == 13
				cursor.pointer = decoded_data + sizeof(short);
				element = cursor.pointer;
				element_count = *(short const *)decoded_data;
#elif VARIANT == 14
				element_base = decoded_data;
				element_count = *(short const *)decoded_data;
#elif VARIANT == 17
				element = decoded_data + sizeof(short);
				element_count = *(short const *)(element - sizeof(short));
#elif VARIANT == 18
				element = decoded_data;
				element_count = *(short const *)element;
				element += sizeof(short);
#elif VARIANT == 1 || VARIANT == 2 || VARIANT == 3 || VARIANT == 5 || VARIANT == 14
				element_count = *(short const *)decoded_data;
#else
				element = decoded_data + sizeof(short);
				element_count = *(short const *)decoded_data;
#endif

#if VARIANT == 6
				packet = packet_definition;
				verify_fields(packet, 0, field + 1, &element_field_count);
#elif VARIANT == 7
				packet = (element, packet_definition);
				verify_fields(packet, 0, field + 1, &element_field_count);
#elif VARIANT == 1
				verify_fields(
					packet_definition,
					(element = decoded_data + sizeof(short), 0),
					field + 1,
					&element_field_count);
#elif VARIANT == 2
				verify_fields(
					(element = decoded_data + sizeof(short), packet_definition),
					0,
					field + 1,
					&element_field_count);
#elif VARIANT == 3
				verify_fields(
					packet_definition,
					0,
					(element = decoded_data + sizeof(short), field + 1),
					&element_field_count);
#elif VARIANT == 14
				verify_fields(packet_definition, 0, field + 1, &element_field_count);
				element = element_base + sizeof(short);
#elif VARIANT == 19
				verify_fields(packet, 0, field + 1, &element_field_count);
#else
				verify_fields(packet_definition, 0, field + 1, &element_field_count);
#endif
#if VARIANT == 5
				element = decoded_data + sizeof(short);
#endif
				if (element_count < 0 || element_count > field->count)
					report_invalid(281);
				if (element_count < 0 || element_count > field->count)
					element_count = 0;
				encode_integer(state, element_count, field->count);
				while (element_count-- > 0)
				{
					short element_size;

					fixture(
#if VARIANT == 6 || VARIANT == 7
						packet,
#elif VARIANT == 19
						packet,
#else
						packet_definition,
#endif
						state,
						packet_version,
						element,
						&element_size,
						field + 1,
						0);
					element += element_size;
				}
				field += element_field_count;
				break;
			}
			default:
				report_invalid(297);
				break;
			}
		}
		else
		{
			switch (field->type)
			{
			case field_pad:
				break;
			case field_bytes:
			case field_shorts:
			case field_longs:
			case field_int64s:
			case field_raw:
				encode_memory(state, 0, field->count, 1);
				break;
			case field_string:
			{
				byte zero = 0;
				encode_memory(state, &zero, 1, 1);
				break;
			}
			case field_data:
			case field_array:
				encode_integer(state, 0, field->count);
				break;
			default:
				report_invalid(324);
				break;
			}
		}
		decoded_data += field->size;
		field++;
	}
	if (field_count)
		*field_count = (short)(field - fields + 1);
	if (encoded_packet_size)
		*encoded_packet_size = (short)(decoded_data - decoded_start);

	return;
}
