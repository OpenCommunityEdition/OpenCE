/* ---------- headers */

#include "cseries.h"

#include "sound_import.h"

#include "memory/byte_swapping.h"
#include "tag_files/files.h"

/* ---------- constants */

enum
{
	RIFF_CONTAINER_HEADER_SIZE= 12,
	RIFF_CHUNK_HEADER_SIZE= 8,
	RIFF_FORMAT_CHUNK_SIZE= 18,
	WAVE_FORMAT_PCM= 1
};

/* ---------- macros */

/* ---------- structures */

struct riff_container_chunk
{
	tag signature;
	long data_size;
	tag form_type;
};

struct riff_chunk
{
	long data_size;
	tag type;
};

struct riff_format_chunk
{
	short format_tag;
	short channel_count;
	long samples_per_second;
	long average_bytes_per_second;
	short block_alignment;
	short significant_bits_per_sample;
	short extra_data_size;
};

/* ---------- prototypes */

/* ---------- globals */

/* Names and types of these eight file statics are attested by a later first-party build (the 2003 PC demo PDB's
sound_wave.obj); January's own PDB has no static names. Their offsets and January's single 4-byte-aligned .data
contribution agree with that build. */

static byte_swap_code riff_container_chunk_bs_codes[6]=
{
	_begin_bs_array, 1,
	_4byte, _4byte, _4byte,
	_end_bs_array,
};

static struct byte_swap_definition riff_container_chunk_bs_definition=
{
	"riff container chunk",
	sizeof(struct riff_container_chunk),
	riff_container_chunk_bs_codes,
	BYTE_SWAP_DEFINITION_SIGNATURE,
	FALSE,
};

static byte_swap_code riff_chunk_type_bs_codes[4]=
{
	_begin_bs_array, 1,
	_4byte,
	_end_bs_array,
};

static struct byte_swap_definition riff_chunk_type_bs_definition=
{
	"riff chunk type",
	sizeof(tag),
	riff_chunk_type_bs_codes,
	BYTE_SWAP_DEFINITION_SIGNATURE,
	FALSE,
};

static byte_swap_code riff_chunk_length_bs_codes[4]=
{
	_begin_bs_array, 1,
	_4byte,
	_end_bs_array,
};

static struct byte_swap_definition riff_chunk_length_bs_definition=
{
	"riff chunk length",
	sizeof(long),
	riff_chunk_length_bs_codes,
	BYTE_SWAP_DEFINITION_SIGNATURE,
	FALSE,
};

static byte_swap_code riff_format_info_bs_codes[10]=
{
	_begin_bs_array, 1,
	_2byte, _2byte, _4byte, _4byte, _2byte, _2byte, _2byte,
	_end_bs_array,
};

static struct byte_swap_definition riff_format_info_bs_definition=
{
	"riff chunk",
	RIFF_FORMAT_CHUNK_SIZE,
	riff_format_info_bs_codes,
	BYTE_SWAP_DEFINITION_SIGNATURE,
	FALSE,
};

/* ---------- public code */

boolean sound_file_is_wave(
	struct file_reference const *file)
{
	struct riff_container_chunk container;
	boolean result= FALSE;

	if (file_open((struct file_reference *)file, FLAG(_permission_read_bit)))
	{
		if (file_read_from_position(file, 0, sizeof(container), &container))
		{
			byte_swap_data(&riff_container_chunk_bs_definition, &container, 1);
			if (container.signature=='RIFF' && container.form_type=='WAVE')
				result= TRUE;
		}
		file_close((struct file_reference *)file);
	}

	return result;
}

boolean sound_file_wave_info_get(
	struct file_reference const *file,
	struct sound_file_info *info)
{
	struct riff_chunk chunk;
	struct riff_format_chunk format;
	boolean result= FALSE;
	long position= RIFF_CONTAINER_HEADER_SIZE;

	if (file_open((struct file_reference *)file, FLAG(_permission_read_bit)))
	{
		while (file_read_from_position(file, position, sizeof(chunk.type), &chunk.type))
		{
			position+= sizeof(chunk.type);
			if (file_read_from_position(file, position, sizeof(chunk.data_size), &chunk.data_size))
			{
				byte_swap_data(&riff_chunk_type_bs_definition, &chunk.type, 1);
				if (chunk.type=='fmt ')
				{
					position+= sizeof(chunk.data_size);
					if (file_read_from_position(file, position, RIFF_FORMAT_CHUNK_SIZE, &format))
					{
						if (format.samples_per_second==11025 ||
							format.samples_per_second==22050 ||
							format.samples_per_second==44100)
						{
							info->samples_per_second= format.samples_per_second;
							info->channel_count= format.channel_count;
							info->significant_bits_per_sample= format.significant_bits_per_sample;
							if (format.format_tag==WAVE_FORMAT_PCM)
								result= TRUE;
						}
						else
							info->samples_per_second= NONE;
					}
					break;
				}

				position+= sizeof(chunk.data_size)+(chunk.data_size&1 ? chunk.data_size+1 : chunk.data_size);
			}
		}
		file_close((struct file_reference *)file);
	}

	return result;
}

boolean sound_file_wave_raw_data_get(
	struct file_reference const *file,
	long *size,
	void *data)
{
	struct riff_chunk chunk;
	boolean result= FALSE;
	long position= RIFF_CONTAINER_HEADER_SIZE;

	if (file_open((struct file_reference *)file, FLAG(_permission_read_bit)))
	{
		while (file_read_from_position(file, position, sizeof(chunk.type), &chunk.type))
		{
			position+= sizeof(chunk.type);
			if (file_read_from_position(file, position, sizeof(chunk.data_size), &chunk.data_size))
			{
				byte_swap_data(&riff_chunk_type_bs_definition, &chunk.type, 1);
				if (chunk.type=='data')
				{
					*size= chunk.data_size;
					position+= sizeof(chunk.data_size);
					if (file_read_from_position(file, position, *size, data))
						result= TRUE;
					break;
				}

				position+= sizeof(chunk.data_size)+(chunk.data_size&1 ? chunk.data_size+1 : chunk.data_size);
			}
		}
		file_close((struct file_reference *)file);
	}

	return result;
}

void sound_file_wave_format(
	struct sound_file_info const *info,
	long *size,
	void *data)
{
	if (info->significant_bits_per_sample==8)
	{
		long sample_index;
		byte *source;
		short *destination;

		sample_index= *size-1;
		source= (byte *)data+sample_index;
		destination= (short *)data+sample_index;
		for (; sample_index>=0; sample_index--)
		{
			*destination--= (short)(*source*0x101+0x7F80);
			source--;
		}
		*size<<= 1;
	}
	else
		match_assert("c:\\halo\\SOURCE\\sound\\sound_import\\sound_wave.c", 280, info->significant_bits_per_sample==16);

	return;
}

/* ---------- private code */
