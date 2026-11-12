#include "harness.h"
typedef unsigned long DWORD;
typedef unsigned int GLuint;
typedef int BOOL;
#define GL_FRAGMENT_SHADER 1
struct nv2a_pixel_shader_key { DWORD source, unused; };
static struct { const char *dump_shaders; } debug_settings;
static const char *sources[] = {"shader A", "shader B", "failed shader", "shader A extra"};
static unsigned int compiles, translations, freed;
static int fail_allocation;
static char *nv2a_pixel_shader_to_glsl(const struct nv2a_pixel_shader_key *key)
{
	char *text = malloc(strlen(sources[key->source]) + 1);
	CHECK(text != NULL, "source allocation");
	strcpy(text, sources[key->source]); translations++;
	return text;
}
static GLuint xgpu_compile_shader(int type, const char *source, const char *what)
{
	CHECK(type == GL_FRAGMENT_SHADER && !strcmp(what, "pixel"), "changed compile contract");
	compiles++;
	return !strcmp(source, sources[2]) ? 0 : compiles;
}
static void *cache_malloc(size_t size) { return fail_allocation ? NULL : malloc(size); }
static void cache_free(void *memory) { freed++; free(memory); }
static void *device_record_new(size_t size) { return cache_malloc(size); }
#define malloc cache_malloc
#define free cache_free
#include "under_test.inc"
#undef malloc
#undef free

int main(int argc, char **argv)
{
	struct nv2a_pixel_shader_key a = {0, 1}, b = {0, 2};
	GLuint first, second;
	unsigned int i, count = 0;
	CHECK(argc == 2, "case required");
	if (!strcmp(argv[1], "distinct")) b.source = 1;
	if (!strcmp(argv[1], "prefix")) b.source = 3;
	if (!strcmp(argv[1], "failure")) a.source = b.source = 2;
	if (!strcmp(argv[1], "allocation")) fail_allocation = 1;
	first = fragment_shader_get(&a);
	second = fragment_shader_get(&b);
	if (first)
		CHECK(first < shader_hashes_size && shader_hashes[first] == source_hash(sources[a.source]),
			"shared fragment missing from program-binary cache hash table");
	if (second)
		CHECK(second < shader_hashes_size && shader_hashes[second] == source_hash(sources[b.source]),
			"second fragment has the wrong program-binary cache hash");
	if (!strcmp(argv[1], "distinct") || !strcmp(argv[1], "prefix"))
		CHECK(first != second && compiles == 2, "different GLSL merged on hash/prefix");
	else if (!strcmp(argv[1], "allocation"))
		CHECK(first && second && compiles == 2 && freed == 2, "allocation fallback lost shader/source");
	else
		CHECK(first == second && compiles == 1 && freed == 1, "identical source compiled again");
	if (!strcmp(argv[1], "failure")) CHECK(!first && !second, "failed shader returned success");
	CHECK(translations == 2, "full keys incorrectly collapsed before translation");
	CHECK(fragment_shader_get(&a) == first && translations == 2, "recent key regenerated source");
	if (!strcmp(argv[1], "recent"))
	{
		for (i = 3; i < 20; i++)
		{
			b.unused = i;
			CHECK(fragment_shader_get(&b) == first, "many state keys changed shader");
		}
		CHECK(compiles == 1, "recent eviction discarded shared shader");
		CHECK(fragment_shader_get(&a) == first && translations == 19, "full-key bucket missed after recent eviction");
	}
	/* Account for owned sources and aliases independently; maps do not own either. */
	for (i = 0; i < FRAGMENT_BUCKETS; i++)
	{
		struct fragment_source_entry *source = fragment_source_buckets[i];
		struct fragment_entry *key = fragment_buckets[i];
		while (source)
		{
			struct fragment_source_entry *next = source->next;
			count++; free(source->source); free(source); source = next;
		}
		while (key) { struct fragment_entry *next = key->next; free(key); key = next; }
	}
	CHECK(freed + count == translations, "source leaked or owned more than once");
	free(shader_hashes);
	return 0;
}
