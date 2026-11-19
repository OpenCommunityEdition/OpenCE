/*
HUD_HIRES.C

The high-res HUD's textures (hud_hires.h): which one stands for a bitmap being
uploaded, and each one's GL texture.

Which bitmap is at an address the game knows (from the loaded map's tags:
port/linux/game/hud_hires_tags.c). Each texture is decoded from its PNG when
first drawn and kept: up to 71 of the HUD's, about 227 MB with their mip
levels, though a game draws only some (the scopes' only when zoomed), and
the titles of the menus shown, about 3 MB each (11 MB for the carnage
report's, a whole panel), and their button icons, about 0.3 MB each (5.3
MB for the message icons' sheet). On the desktop builds a map's HUD PNGs
are decoded as it loads, on a thread of their own, so that the HUD's first
frame only uploads them (decoding them there held that frame up); what is
decoded and not drawn is let go when the map is unloaded.
They are drawn with linear filtering and their mip levels (d3d8_gl.c,
configure_sampler), as they are larger than they appear.

The embedded PNGs are the ones tools/hud_assets.py, title_assets.py and
button_assets.py write. A menus folder's PNGs can be anyone's. Only 8-bit
RGBA, non-interlaced PNGs are read, with their data inflated by the port's
zlib (port/third_party/zlib).
*/

#include "hud_hires.h"
#include "platform.h"
#include "port_config.h"
#include "xgpu.h"

#include "zlib_prefixed.h"

#if !defined(HALO_ANDROID) && !defined(HALO_WEB)
#define HUD_HIRES_DECODE_AHEAD
#include <pthread.h>
#endif
#include <stdlib.h>
#include <string.h>

/* the game's (port/linux/game/hud_hires_tags.c) */
long hud_hires_asset_at(unsigned long address, long width, long height);
long hud_hires_asset_stock_custom_edition(unsigned long address, long width, long height, long asset);

/* Include every embedded HUD and button style, including stick frames. */
#define MAXIMUM_TEXTURES 256
/* a PNG's inflated rows and its texels, which are held at once: 128 MB for
a 4096 by 4096 sheet (the largest shipped, 2048 by 2048, takes 32 MB), and
no more for a small file that names a large size (a menus folder's) */
#define MAXIMUM_DECODED_SIZE (192UL << 20)

/* a texture's PNG decoded ahead of its first draw (decode_state) */
enum
{
	_decode_none,
	_decode_queued,
	_decode_decoding,
	_decode_done,
};

static struct
{
	unsigned int texture;
	unsigned long levels;
	int failed;
	int other_pixels_logged;
	/* (decoded ahead: these, the decoding thread's until it is done, under
	decode_lock) */
	int decode_state;
	unsigned char *pixels;
	unsigned long width, height;
} textures[MAXIMUM_TEXTURES];

#ifdef HUD_HIRES_DECODE_AHEAD
/* (the thread, detached, is waited for by its decode_running) */
static pthread_mutex_t decode_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t decode_finished = PTHREAD_COND_INITIALIZER;
static int decode_running, decode_stopping;
#endif

long hud_hires_asset_count(void)
{
	return hud_hires_embedded_count < MAXIMUM_TEXTURES ? (long)hud_hires_embedded_count : MAXIMUM_TEXTURES;
}

/* whether the texture may stand for a Custom Edition map's stock bitmap of
its name too (port/assets/hud/custom_edition.json) */
int hud_hires_asset_custom_edition(long asset)
{
	return asset >= 0 && asset < hud_hires_asset_count() && hud_hires_embedded[asset].custom_edition;
}

int hud_hires_asset_ce_only(long asset)
{
	return asset >= 0 && asset < hud_hires_asset_count() && hud_hires_embedded[asset].ce_only;
}

long hud_hires_ce_stock_find(char const *tag, long bitmap)
{
	unsigned int index;
	for (index = 0; index < hud_hires_ce_stock_count; index++)
		if (hud_hires_ce_stock[index].bitmap == bitmap && !strcmp(tag, hud_hires_ce_stock[index].tag))
			return (long)index;
	return -1;
}

long hud_hires_ce_stock_count_get(void) { return hud_hires_ce_stock_count; }
char const *hud_hires_ce_stock_tag(long index) { return hud_hires_ce_stock[index].tag; }
long hud_hires_ce_stock_bitmap(long index) { return hud_hires_ce_stock[index].bitmap; }

int hud_hires_ce_stock_matches(long index, unsigned long address, long width, long height,
	unsigned long size)
{
	const struct hud_hires_ce_stock *stock;
	if (index < 0 || (unsigned long)index >= hud_hires_ce_stock_count || !address)
		return 0;
	stock = &hud_hires_ce_stock[index];
	return width == stock->width && height == stock->height && size == stock->size &&
		crc32(0L, (const Bytef *)address, (uInt)size) == stock->crc;
}

char const *hud_hires_asset_tag(long asset)
{
	return hud_hires_embedded[asset].tag;
}

long hud_hires_asset_bitmap(long asset)
{
	return hud_hires_embedded[asset].bitmap;
}

/* whether the texture can stand for a bitmap of this size: a whole multiple
of it, the same both ways */
long hud_hires_asset_fits(long asset, long width, long height)
{
	const struct hud_hires_embedded *embedded = &hud_hires_embedded[asset];

	return width > 0 && height > 0 && embedded->width % width == 0 && embedded->height % height == 0 &&
		embedded->width / width == embedded->height / height && embedded->width / width > 1;
}

/* Changed settings since the previous rendered frame: HUD bit 0, titles bit 1.
 * Pending menu edits do not change config_changes(). */
unsigned long hud_hires_settings_changed(void)
{
	static unsigned long read_at = (unsigned long)-1;
	static unsigned long previous;
	unsigned long current, changed;

	if (read_at == config_changes()) return 0;
	current = (config_boolean("display.high_res_hud") ? 1UL : 0UL) |
		(config_boolean("display.high_res_text") ? 2UL : 0UL);
	changed = read_at == (unsigned long)-1 ? 0 : previous ^ current;
	read_at = config_changes();
	previous = current;
	return changed;
}

int hud_hires_bitmap_affected(unsigned long address, long width, long height, unsigned long changed)
{
	long asset = hud_hires_asset_at(address, width, height);

	return asset >= 0 && asset < hud_hires_asset_count() &&
		(changed & (hud_hires_embedded[asset].title ? 2UL : 1UL)) != 0;
}

long hud_hires_override_find(unsigned long address, unsigned long width, unsigned long height,
	unsigned long level0_size)
{
	static int hud_enabled, titles_enabled;
	static unsigned long read_at = (unsigned long)-1;
	long asset;

	if (read_at != config_changes())
	{
		read_at = config_changes();
		hud_enabled = config_boolean("display.high_res_hud");
		titles_enabled = config_boolean("display.high_res_text");
	}
	if (!hud_enabled && !titles_enabled)
		return -1;
	asset = hud_hires_asset_at(address, (long)width, (long)height);
	if (asset < 0 || asset >= hud_hires_asset_count())
		return -1;
	if (!(hud_hires_embedded[asset].title ? titles_enabled : hud_enabled))
		return -1;
	/* (a Custom Edition map's stock HUD bitmap, laid out as this texture's,
	has its own pixels) */
	if (hud_hires_asset_stock_custom_edition(address, (long)width, (long)height, asset))
	{
		const struct hud_hires_embedded *embedded = &hud_hires_embedded[asset];

		/* Resource maps can be modified too. Match the stock pixels after
		the CE loader has put them in hardware order, not just their name. */
		return width == embedded->ce_width && height == embedded->ce_height &&
			level0_size == embedded->ce_size &&
			crc32(0L, (const Bytef *)address, (uInt)level0_size) == embedded->ce_crc ? asset : -1;
	}
	if (crc32(0L, (const Bytef *)address, (uInt)level0_size) != hud_hires_embedded[asset].crc)
	{
		if (!textures[asset].other_pixels_logged)
		{
			platform_log("high-res hud: %s bitmap %d is not the one its texture was drawn for here "
				"(another language's or a modified map): drawn as it is",
				hud_hires_embedded[asset].tag, hud_hires_embedded[asset].bitmap);
			textures[asset].other_pixels_logged = 1;
		}
		return -1;
	}
	return asset;
}

int hud_hires_override_coverage(long asset)
{
	return asset >= 0 && asset < hud_hires_asset_count() && hud_hires_embedded[asset].coverage;
}

/* ---------- sprites */

/* the placeholders of the textures drawn for some of a bitmap's sprites
(port/linux/game/hud_hires_tags.c): their D3D textures' Data */
/* Room for the gameplay button styles and map-owned sprite placeholders. */
#define MAXIMUM_PLACEHOLDERS 32

static struct
{
	unsigned long data;
	long asset;
} placeholders[MAXIMUM_PLACEHOLDERS];
static long placeholder_count = 0;
/* a bit of 1024 for each placeholder's Data (placeholder_filter_bit): a
texture whose bit is clear is no placeholder, found without searching
them, as the renderer asks for each texture it binds */
static unsigned char placeholder_filter[128];

static unsigned long placeholder_filter_bit(unsigned long data)
{
	return (unsigned long)((data * 2654435761UL) & 0xFFFFFFFFUL) >> 22;
}

/* (after a placeholder's Data has changed) */
static void placeholder_filter_build(void)
{
	long index;

	memset(placeholder_filter, 0, sizeof(placeholder_filter));
	for (index = 0; index < placeholder_count; index++)
	{
		unsigned long bit = placeholder_filter_bit(placeholders[index].data);

		if (placeholders[index].data)
			placeholder_filter[bit >> 3] |= (unsigned char)(1 << (bit & 7));
	}
}

/* the sequences whose sprites the texture is drawn for (hud_hires.h), or 0 */
unsigned long hud_hires_asset_sprites(long asset)
{
	return hud_hires_embedded[asset].sprites;
}

/* whether the texture drawn for some of a bitmap's sprites can be drawn for
the bitmap whose pixels are at address (guest virtual; its first mip level
level0_size bytes): its setting on, the pixels those it was drawn for, and
the texture decoded */
int hud_hires_sprites_drawable(long asset, unsigned long address, unsigned long level0_size)
{
	static int hud_enabled, titles_enabled;
	static unsigned long read_at = (unsigned long)-1;
	unsigned long levels;

	if (read_at != config_changes())
	{
		read_at = config_changes();
		hud_enabled = config_boolean("display.high_res_hud");
		titles_enabled = config_boolean("display.high_res_text");
	}
	if (asset < 0 || asset >= hud_hires_asset_count() || !hud_hires_embedded[asset].sprites ||
		!(hud_hires_embedded[asset].title ? titles_enabled : hud_enabled))
	{
		return 0;
	}
	if (crc32(0L, (const Bytef *)address, (uInt)level0_size) != hud_hires_embedded[asset].crc)
	{
		if (!textures[asset].other_pixels_logged)
		{
			platform_log("high-res hud: %s bitmap %d is not the one its sprites' texture was drawn for "
				"here (another language's or a modified map): drawn as it is",
				hud_hires_embedded[asset].tag, hud_hires_embedded[asset].bitmap);
			textures[asset].other_pixels_logged = 1;
		}
		return 0;
	}
	return hud_hires_override_texture(asset, &levels) != 0;
}

/* the placeholder the game draws the texture's sprites from: its D3D
texture (NULL forgets it) */
void hud_hires_register_placeholder(long asset, const unsigned long *texture)
{
	long index;

	for (index = 0; index < placeholder_count && placeholders[index].asset != asset; index++)
		;
	if (texture && index == placeholder_count && placeholder_count < MAXIMUM_PLACEHOLDERS)
		placeholder_count++;
	if (index < placeholder_count)
	{
		placeholders[index].data = texture ? texture[1] : 0;
		placeholders[index].asset = asset;
	}
	placeholder_filter_build();
}

unsigned int hud_hires_placeholder_texture(unsigned long data, unsigned long *levels)
{
	unsigned long bit = placeholder_filter_bit(data);
	long index;

	if (!(placeholder_filter[bit >> 3] & 1 << (bit & 7)))
		return 0;
	for (index = 0; data && index < placeholder_count; index++)
	{
		if (placeholders[index].data == data)
			return hud_hires_override_texture(placeholders[index].asset, levels);
	}
	return 0;
}

int hud_hires_override_point_threshold(long asset)
{
	return asset >= 0 && asset < hud_hires_asset_count() && hud_hires_embedded[asset].point_threshold;
}

/* ---------- decoding */

/* Catmull-Rom interpolation of the map's original texels. No font, sprite
 * rectangle, registration point or animation frame is substituted. Keeping
 * the input centres at (x + .5) also keeps the original HUD alignment.
 *
 * At exactly 4x, output texel 4k + phase falls at source position
 * k + (phase - 1.5) / 4, so there are only four sets of weights (t = .625,
 * .875, .125 and .375), each a multiple of 1/1024: phases 0 and 1 read
 * source texels k - 2 .. k + 1, phases 2 and 3 k - 1 .. k + 2.
 *
 * The GPU draws it (upscale_draw): each output texel's 16 taps summed in
 * integers, the filter's exact value rounded, in one pass the same on every
 * build. Where that pass cannot be made, the CPU does it (upscale_pixels):
 * the filter is separable, so each source row is first spread across to the
 * output width (exact in float: 8-bit texels times 10-bit weights), then
 * four output rows are made from each five of those, within a step of the
 * exact value. */
#define UPSCALE_WEIGHT(n) ((float)(n) / 1024.0f)
#define UPSCALE_PHASE_0(a, b, c, d) (UPSCALE_WEIGHT(-45) * (a) + UPSCALE_WEIGHT(399) * (b) + \
	UPSCALE_WEIGHT(745) * (c) + UPSCALE_WEIGHT(-75) * (d))
#define UPSCALE_PHASE_1(a, b, c, d) (UPSCALE_WEIGHT(-7) * (a) + UPSCALE_WEIGHT(93) * (b) + \
	UPSCALE_WEIGHT(987) * (c) + UPSCALE_WEIGHT(-49) * (d))

static unsigned long upscale_clamp(long index, unsigned long count)
{
	return index < 0 ? 0 : index >= (long)count ? count - 1 : (unsigned long)index;
}

/* source row (row - 2, clamped) of the BGRA sheet across to the output
 * width, as RGBA floats (16 floats a source texel); padded holds width + 4
 * texels */
static void upscale_across(const unsigned char *bgra, unsigned long width, unsigned long height, unsigned long row,
	float *padded, float *out)
{
	const unsigned char *source = bgra + upscale_clamp((long)row - 2, height) * width * 4;
	unsigned long x;

	/* (two texels repeated at either end: the edges clamp) */
	for (x = 0; x < width + 4; x++)
	{
		const unsigned char *texel = source + upscale_clamp((long)x - 2, width) * 4;

		padded[x * 4 + 0] = texel[2];
		padded[x * 4 + 1] = texel[1];
		padded[x * 4 + 2] = texel[0];
		padded[x * 4 + 3] = texel[3];
	}
	for (x = 0; x < width; x++)
	{
		const float *p = padded + x * 4;
		float *texels = out + x * 16;
		int channel;

		for (channel = 0; channel < 4; channel++)
		{
			float a = p[channel], b = p[4 + channel], c = p[8 + channel], d = p[12 + channel];
			float e = p[16 + channel];

			texels[channel] = UPSCALE_PHASE_0(a, b, c, d);
			texels[4 + channel] = UPSCALE_PHASE_1(a, b, c, d);
			/* (phases 2 and 3 are 1 and 0 mirrored) */
			texels[8 + channel] = UPSCALE_PHASE_1(e, d, c, b);
			texels[12 + channel] = UPSCALE_PHASE_0(e, d, c, b);
		}
	}
}

static unsigned char upscale_byte(float value)
{
	value += 0.5f;
	return (unsigned char)(value < 0.0f ? 0 : value > 255.0f ? 255 : (int)value);
}

/* the CPU's upscale: RGBA, width * 4 by height * 4; NULL past maximum */
static unsigned char *upscale_pixels(const unsigned char *bgra, unsigned long width, unsigned long height,
	unsigned long maximum)
{
	unsigned long count = width * 16, x, k;
	unsigned char *out;
	float *rows, *padded;

	if (!bgra || !width || !height || width > maximum || height > maximum)
		return NULL;
	out = malloc(width * height * 64);
	/* (the five spread rows an output row reads, by row % 5) */
	rows = malloc(5 * count * sizeof(float));
	padded = malloc((width + 4) * 4 * sizeof(float));
	if (out && rows && padded)
	{
		for (k = 0; k < 4; k++)
			upscale_across(bgra, width, height, k, padded, rows + k * count);
		for (k = 0; k < height; k++)
		{
			const float *row_a = rows + k % 5 * count, *row_b = rows + (k + 1) % 5 * count;
			const float *row_c = rows + (k + 2) % 5 * count, *row_d = rows + (k + 3) % 5 * count;
			const float *row_e = rows + (k + 4) % 5 * count;
			unsigned char *out_0 = out + k * 4 * count, *out_1 = out_0 + count;
			unsigned char *out_2 = out_1 + count, *out_3 = out_2 + count;

			upscale_across(bgra, width, height, k + 4, padded, rows + (k + 4) % 5 * count);
			for (x = 0; x < count; x++)
			{
				float a = row_a[x], b = row_b[x], c = row_c[x], d = row_d[x], e = row_e[x];

				out_0[x] = upscale_byte(UPSCALE_PHASE_0(a, b, c, d));
				out_1[x] = upscale_byte(UPSCALE_PHASE_1(a, b, c, d));
				out_2[x] = upscale_byte(UPSCALE_PHASE_1(e, d, c, b));
				out_3[x] = upscale_byte(UPSCALE_PHASE_0(e, d, c, b));
			}
		}
	}
	else
	{
		free(out);
		out = NULL;
	}
	free(rows);
	free(padded);
	return out;
}

/* the GPU's: one triangle over the target, each texel's 16 taps of the
 * source (BGRA bytes as an RGBA8UI texture, read exactly) */
static const char upscale_vertex_source[] =
	"void main()\n"
	"{\n"
	"\tgl_Position = vec4(float((gl_VertexID & 1) << 2) - 1.0, float((gl_VertexID & 2) << 1) - 1.0, 0.0, 1.0);\n"
	"}\n";

static const char upscale_fragment_source[] =
	"uniform highp usampler2D source;\n"
	"out vec4 result;\n"
	"\n"
	"/* the four phases' weights, in 1/1024 */\n"
	"const int weights[16] = int[16](-45, 399, 745, -75, -7, 93, 987, -49, -49, 987, 93, -7, -75, 745, 399, -45);\n"
	"\n"
	"void main()\n"
	"{\n"
	"\tivec2 size = textureSize(source, 0);\n"
	"\tivec2 position = ivec2(gl_FragCoord.xy);\n"
	"\tivec2 phase = position & 3;\n"
	"\tivec2 first = (position >> 2) - 2 + (phase >> 1);\n"
	"\t/* (at most 1264 * 1264 * 255: 32 bits hold it) */\n"
	"\tivec4 sum = ivec4(0);\n"
	"\n"
	"\tfor (int row = 0; row < 4; row++)\n"
	"\t{\n"
	"\t\tint y = clamp(first.y + row, 0, size.y - 1);\n"
	"\t\tivec4 across = ivec4(0);\n"
	"\n"
	"\t\tfor (int column = 0; column < 4; column++)\n"
	"\t\t\tacross += weights[phase.x * 4 + column] *\n"
	"\t\t\t\tivec4(texelFetch(source, ivec2(clamp(first.x + column, 0, size.x - 1), y), 0));\n"
	"\t\tsum += weights[phase.y * 4 + row] * across;\n"
	"\t}\n"
	"\t/* (rounded half up, in 1/(1024 * 1024); BGRA to RGBA) */\n"
	"\tresult = vec4(clamp((max(sum, 0) + 524288) >> 20, 0, 255)).bgra / 255.0;\n"
	"}\n";

/* (the unit the source is read from: past the draws' (D3DTSS_MAXSTAGES) and
 * the post-processing passes', so no sampler object is ever bound to it, as
 * an integer texture needs) */
#define UPSCALE_UNIT 4

static struct
{
	GLuint program, vertex_array, framebuffer;
	int failed;
} upscale_gpu;

static void upscale_shader_header(struct xgpu_text *text)
{
#ifdef HALO_GLES
	xgpu_text_append(text,
		"#version %s\n"
		"precision highp float;\n"
		"precision highp int;\n",
		xgpu_capabilities.shading_language);
#else
	xgpu_text_append(text, "#version 450 core\n");
#endif
}

/* the GPU's program and objects, made once; FALSE if they cannot be (the
 * CPU upscales from then on). It changes GL state behind the state cache's
 * back (the program used), as the callers tell it */
static int upscale_gpu_prepare(void)
{
	struct xgpu_text text = { 0 };
	GLuint vertex_shader, fragment_shader;

	if (upscale_gpu.program || upscale_gpu.failed)
		return !upscale_gpu.failed;
	upscale_gpu.failed = 1;
	upscale_shader_header(&text);
	xgpu_text_append(&text, "%s", upscale_vertex_source);
	vertex_shader = xgpu_compile_shader(GL_VERTEX_SHADER, text.buffer, "HUD upscale");
	text.length = 0;
	upscale_shader_header(&text);
	xgpu_text_append(&text, "%s", upscale_fragment_source);
	fragment_shader = vertex_shader ? xgpu_compile_shader(GL_FRAGMENT_SHADER, text.buffer, "HUD upscale") : 0;
	free(text.buffer);
	if (fragment_shader)
		upscale_gpu.program = xgpu_link_program(vertex_shader, fragment_shader, "HUD upscale");
	if (vertex_shader)
		glDeleteShader(vertex_shader);
	if (fragment_shader)
		glDeleteShader(fragment_shader);
	if (!upscale_gpu.program)
	{
		platform_log("high-res HUD: the GPU's upscale cannot be made; the CPU's instead");
		return 0;
	}
	glUseProgram(upscale_gpu.program);
	glUniform1i(glGetUniformLocation(upscale_gpu.program, "source"), UPSCALE_UNIT);
	glGenVertexArrays(1, &upscale_gpu.vertex_array);
	glGenFramebuffers(1, &upscale_gpu.framebuffer);
	upscale_gpu.failed = 0;
	return 1;
}

void hud_hires_gl_initialize(void)
{
	/* (with the setting turned on later, at the first sheet) */
	if (config_boolean("display.high_res_hud"))
		upscale_gpu_prepare();
}

/* the upscale drawn into texture's first level (width * 4 by height * 4,
 * made); FALSE if it was not. It is asked for while a draw's textures are
 * found, before its targets and state are set (d3d8_gl.c, prepare_draw), so
 * the GL state it changes is only forgotten after, as a multisampled target's
 * resolve there is (hud_hires_upscale_texture) */
static int upscale_draw(const unsigned char *bgra, unsigned long width, unsigned long height, GLuint texture)
{
	GLenum draw_buffer = GL_COLOR_ATTACHMENT0;
	GLuint source;
	int drawn;

	if (!upscale_gpu_prepare())
		return 0;
	glActiveTexture(GL_TEXTURE0 + UPSCALE_UNIT);
	glGenTextures(1, &source);
	glBindTexture(GL_TEXTURE_2D, source);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8UI, (GLsizei)width, (GLsizei)height, 0, GL_RGBA_INTEGER,
		GL_UNSIGNED_BYTE, bgra);
	glBindFramebuffer(GL_FRAMEBUFFER, upscale_gpu.framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
	glDrawBuffers(1, &draw_buffer);
	drawn = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
	if (drawn)
	{
		glDisable(GL_SCISSOR_TEST);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_STENCIL_TEST);
		glDisable(GL_BLEND);
		glDisable(GL_CULL_FACE);
#ifndef HALO_GLES
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glViewport(0, 0, (GLsizei)width * 4, (GLsizei)height * 4);
		glUseProgram(upscale_gpu.program);
		glBindVertexArray(upscale_gpu.vertex_array);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
	else
		platform_log("high-res HUD: the GPU cannot draw a %lu by %lu upscale; the CPU's instead", width * 4,
			height * 4);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	glDeleteTextures(1, &source);
	glBindTexture(GL_TEXTURE_2D, texture);
	return drawn;
}

static unsigned char *png_decode(const unsigned char *data, unsigned long size,
	unsigned long *png_width, unsigned long *png_height);

/* The background's old icon otherwise draws over the existing high-res meter
 * contour. Only the five known Xbox sheets get this matching contour; their
 * text, and customized sheets, retain their original pixels and layout. It
 * is drawn into the upscale's first level (bound) over the rectangle. */
static void upscale_outline(const unsigned char *bgra, unsigned long width, unsigned long height, long kind)
{
	unsigned long crc, png_width, png_height;
	long asset;
	unsigned char *outline;
	char const *name;
	unsigned long left, top, right, bottom;

	if (width != 128 || height != 32 || (kind != 2 && kind != 3))
		return;
	crc = crc32(0L, bgra, (uInt)(width * height * 4));
	if (kind == 2)
	{
		if (crc != 0x23894da5UL && crc != 0x38fb467aUL && crc != 0xcd279468UL)
			return;
		name = "ui\\hud\\redraws\\vehicle_wrench_outline";
		left = 73; top = 11; right = 101; bottom = 25;
	}
	else
	{
		if (crc != 0x17446e88UL && crc != 0xb14f00a1UL)
			return;
		name = "ui\\hud\\redraws\\passenger_health_outline";
		left = 74; top = 10; right = 89; bottom = 24;
	}
	for (asset = 0; asset < hud_hires_asset_count(); asset++)
	{
		const struct hud_hires_embedded *embedded = &hud_hires_embedded[asset];

		if (strcmp(embedded->tag, name))
			continue;
		outline = png_decode((const unsigned char *)embedded->png, embedded->png_size, &png_width, &png_height);
		if (outline && png_width == width * 4 && png_height == height * 4)
		{
			glPixelStorei(GL_UNPACK_ROW_LENGTH, (GLint)png_width);
			glTexSubImage2D(GL_TEXTURE_2D, 0, (GLint)left * 4, (GLint)top * 4, (GLsizei)(right - left) * 4,
				(GLsizei)(bottom - top) * 4, GL_RGBA, GL_UNSIGNED_BYTE,
				outline + (top * 4 * png_width + left * 4) * 4);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		}
		free(outline);
		break;
	}
}

unsigned int hud_hires_upscale_texture(const unsigned char *bgra, unsigned long width,
	unsigned long height, long kind, unsigned long *levels)
{
	unsigned long maximum = kind == 4 ? 512 : 256, largest;
	GLuint texture;

	if (!xgpu_gl_ready() || !bgra || !width || !height || width > maximum || height > maximum)
		return 0;
	*levels = 1;
	for (largest = (width > height ? width : height) * 4; largest > 1; largest >>= 1)
		(*levels)++;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, (GLint)*levels - 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)width * 4, (GLsizei)height * 4,
		0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	if (!upscale_draw(bgra, width, height, texture))
	{
		unsigned char *pixels = upscale_pixels(bgra, width, height, maximum);

		if (!pixels)
		{
			glDeleteTextures(1, &texture);
			xgpu_gl_state_invalidate();
			return 0;
		}
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (GLsizei)width * 4, (GLsizei)height * 4, GL_RGBA,
			GL_UNSIGNED_BYTE, pixels);
		free(pixels);
	}
	upscale_outline(bgra, width, height, kind);
	glGenerateMipmap(GL_TEXTURE_2D);
	xgpu_gl_state_invalidate();
	return texture;
}

static unsigned long big_endian_long(const unsigned char *bytes)
{
	return ((unsigned long)bytes[0] << 24) | ((unsigned long)bytes[1] << 16) |
		((unsigned long)bytes[2] << 8) | bytes[3];
}

static unsigned char paeth(unsigned char left, unsigned char up, unsigned char up_left)
{
	int estimate = (int)left + up - up_left;
	int to_left = abs(estimate - left), to_up = abs(estimate - up), to_up_left = abs(estimate - up_left);

	if (to_left <= to_up && to_left <= to_up_left)
		return left;
	return to_up <= to_up_left ? up : up_left;
}

/* the PNG's texels, RGBA in rows top first, and its size; NULL if it is not
one that tools/hud_assets.py writes */
static unsigned char *png_decode(const unsigned char *data, unsigned long size, unsigned long *png_width,
	unsigned long *png_height)
{
	unsigned long position = 8;
	unsigned long width = size >= 33 ? big_endian_long(data + 16) : 0;
	unsigned long height = size >= 33 ? big_endian_long(data + 20) : 0;
	unsigned long stride = width * 4, filtered_size = height * (stride + 1);
	unsigned char *compressed = NULL, *filtered = NULL, *pixels = NULL;
	unsigned long compressed_size = 0, row, column;
	uLongf inflated_size = filtered_size;
	int result;

	if (size < 33 || memcmp(data, "\x89PNG\r\n\x1a\n", 8) || memcmp(data + 12, "IHDR", 4) ||
		!width || !height || width > 8192 || height > 8192 ||
		data[24] != 8 || data[25] != 6 || data[28] != 0)
		return NULL;
	if (filtered_size + stride * height > MAXIMUM_DECODED_SIZE)
	{
		platform_log("png: %lux%lu is too large to decode (more than %lu MB)", width, height,
			MAXIMUM_DECODED_SIZE >> 20);
		return NULL;
	}
	*png_width = width;
	*png_height = height;
	compressed = malloc(size);
	while (compressed && position + 12 <= size)
	{
		unsigned long length = big_endian_long(data + position);

		if (length > size - position - 12)
			break;
		if (!memcmp(data + position + 4, "IDAT", 4))
		{
			memcpy(compressed + compressed_size, data + position + 8, length);
			compressed_size += length;
		}
		else if (!memcmp(data + position + 4, "IEND", 4))
		{
			break;
		}
		position += 12 + length;
	}
	filtered = compressed ? malloc(filtered_size) : NULL;
	pixels = filtered ? malloc(stride * height) : NULL;
	if (!pixels)
		goto failed;
	result = uncompress(filtered, &inflated_size, compressed, compressed_size);
	/* (the game's zlib is 1.1, which can stop short of saying the stream has
	ended when the output is exactly full: all of it is enough) */
	if ((result != Z_OK && result != Z_BUF_ERROR) || inflated_size != filtered_size)
		goto failed;
	/* (each row by its filter, the first pixel's 4 bytes, which have none to
	their left, apart; the first row has none above: zeroes) */
	for (row = 0; row < height; row++)
	{
		const unsigned char *line = filtered + row * (stride + 1) + 1;
		unsigned char filter = line[-1];
		unsigned char *out = pixels + row * stride;
		const unsigned char *above = row ? out - stride : NULL;

		if (filter > 4)
			goto failed;
		if (!above && filter == 2)
			filter = 0; /* (up: zero) */
		else if (!above && filter == 4)
			filter = 1; /* (Paeth of left, zero and zero: left) */
		switch (filter)
		{
		case 0:
			memcpy(out, line, stride);
			break;
		case 1:
			memcpy(out, line, 4);
			for (column = 4; column < stride; column++)
				out[column] = (unsigned char)(line[column] + out[column - 4]);
			break;
		case 2:
			for (column = 0; column < stride; column++)
				out[column] = (unsigned char)(line[column] + above[column]);
			break;
		case 3:
			for (column = 0; column < 4; column++)
				out[column] = (unsigned char)(line[column] + (above ? above[column] : 0) / 2);
			for (column = 4; column < stride; column++)
				out[column] = (unsigned char)(line[column] +
					((unsigned)out[column - 4] + (above ? above[column] : 0)) / 2);
			break;
		default:
			/* (Paeth of zero, up and zero: up) */
			for (column = 0; column < 4; column++)
				out[column] = (unsigned char)(line[column] + above[column]);
			for (column = 4; column < stride; column++)
				out[column] = (unsigned char)(line[column] + paeth(out[column - 4], above[column], above[column - 4]));
			break;
		}
	}
	free(compressed);
	free(filtered);
	return pixels;

failed:
	free(compressed);
	free(filtered);
	free(pixels);
	return NULL;
}

/* a GL texture of a decoded PNG's texels (which it frees), as below */
static unsigned int png_pixels_texture(unsigned char *pixels, unsigned long width, unsigned long height,
	unsigned long *levels)
{
	unsigned long largest;
	GLuint texture;

	/* (no OpenGL: a null renderer's postgame scoreboard asks for its icons) */
	if (!pixels || !xgpu_gl_ready())
	{
		free(pixels);
		return 0;
	}
	*levels = 1;
	for (largest = width > height ? width : height; largest > 1; largest >>= 1)
		(*levels)++;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	xgpu_gl_state_invalidate();
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, (GLint)*levels - 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glGenerateMipmap(GL_TEXTURE_2D);
	xgpu_gl_state_invalidate();
	free(pixels);
	return texture;
}

unsigned int hud_hires_png_texture(const void *png, unsigned long size, unsigned long *levels)
{
	unsigned long width = 0, height = 0;
	unsigned char *pixels = png_decode(png, size, &width, &height);

	return png_pixels_texture(pixels, width, height, levels);
}

unsigned int hud_hires_override_texture(long asset, unsigned long *levels)
{
	const struct hud_hires_embedded *embedded;

	if (asset < 0 || asset >= hud_hires_asset_count() || textures[asset].failed || !xgpu_gl_ready())
		return 0;
	if (textures[asset].texture)
	{
		*levels = textures[asset].levels;
		return textures[asset].texture;
	}
	embedded = &hud_hires_embedded[asset];
	{
		unsigned char *pixels = NULL;
		unsigned long width = 0, height = 0;
		int decoded = 0;

#ifdef HUD_HIRES_DECODE_AHEAD
		/* (its PNG decoded as the map loaded, or being decoded: waited for) */
		pthread_mutex_lock(&decode_lock);
		while (textures[asset].decode_state == _decode_decoding)
			pthread_cond_wait(&decode_finished, &decode_lock);
		if (textures[asset].decode_state == _decode_done)
		{
			pixels = textures[asset].pixels;
			width = textures[asset].width;
			height = textures[asset].height;
			textures[asset].pixels = NULL;
			decoded = 1;
		}
		textures[asset].decode_state = _decode_none;
		pthread_mutex_unlock(&decode_lock);
#endif
		if (!decoded)
			pixels = png_decode((const unsigned char *)embedded->png, embedded->png_size, &width, &height);
		textures[asset].texture = png_pixels_texture(pixels, width, height, &textures[asset].levels);
	}
	if (!textures[asset].texture)
	{
		platform_log("high-res hud: could not decode the texture for %s bitmap %d", embedded->tag, embedded->bitmap);
		textures[asset].failed = 1;
		return 0;
	}
	*levels = textures[asset].levels;
	return textures[asset].texture;
}

/* ---------- decoding ahead */

#ifdef HUD_HIRES_DECODE_AHEAD
static void *decode_thread_main(void *unused)
{
	long asset, count = hud_hires_asset_count();

	(void)unused;
	for (asset = 0; asset < count; asset++)
	{
		const struct hud_hires_embedded *embedded = &hud_hires_embedded[asset];
		unsigned long width = 0, height = 0;
		unsigned char *pixels;

		pthread_mutex_lock(&decode_lock);
		if (decode_stopping)
		{
			pthread_mutex_unlock(&decode_lock);
			break;
		}
		if (textures[asset].decode_state != _decode_queued)
		{
			pthread_mutex_unlock(&decode_lock);
			continue;
		}
		textures[asset].decode_state = _decode_decoding;
		pthread_mutex_unlock(&decode_lock);
		pixels = png_decode((const unsigned char *)embedded->png, embedded->png_size, &width, &height);
		pthread_mutex_lock(&decode_lock);
		textures[asset].pixels = pixels;
		textures[asset].width = width;
		textures[asset].height = height;
		textures[asset].decode_state = _decode_done;
		pthread_cond_broadcast(&decode_finished);
		pthread_mutex_unlock(&decode_lock);
	}
	pthread_mutex_lock(&decode_lock);
	decode_running = 0;
	pthread_cond_broadcast(&decode_finished);
	pthread_mutex_unlock(&decode_lock);
	return NULL;
}
#endif

void hud_hires_map_unloaded(void)
{
#ifdef HUD_HIRES_DECODE_AHEAD
	long asset;

	pthread_mutex_lock(&decode_lock);
	decode_stopping = 1;
	while (decode_running)
		pthread_cond_wait(&decode_finished, &decode_lock);
	pthread_mutex_unlock(&decode_lock);
	for (asset = 0; asset < MAXIMUM_TEXTURES; asset++)
	{
		free(textures[asset].pixels);
		textures[asset].pixels = NULL;
		textures[asset].decode_state = _decode_none;
	}
#endif
}

void hud_hires_map_loaded(const long *assets, long count)
{
#ifdef HUD_HIRES_DECODE_AHEAD
	long index, queued = 0;
	pthread_t thread;

	hud_hires_map_unloaded();
	/* (none for a test's run that draws nothing, or with the high-res HUD
	off: then each is decoded when first drawn, as titles are) */
	if (!config_boolean("display.high_res_hud") || config_boolean("debug.null_renderer"))
		return;
	for (index = 0; index < count; index++)
	{
		long asset = assets[index];

		if (asset >= 0 && asset < hud_hires_asset_count() && !hud_hires_embedded[asset].title &&
			!textures[asset].texture && !textures[asset].failed)
		{
			textures[asset].decode_state = _decode_queued;
			queued++;
		}
	}
	if (!queued)
		return;
	decode_stopping = 0;
	decode_running = 1;
	if (pthread_create(&thread, NULL, decode_thread_main, NULL) == 0)
	{
		pthread_detach(thread);
	}
	else
	{
		decode_running = 0;
		for (index = 0; index < MAXIMUM_TEXTURES; index++)
			textures[index].decode_state = _decode_none;
	}
#else
	(void)assets;
	(void)count;
#endif
}
