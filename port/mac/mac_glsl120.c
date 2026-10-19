/* Strict compatibility boundary for the shared NV2A GLSL generators.
   This does not emulate newer shader operations: unsupported source fails. */
/* This source owns transient compiler text, not game tracked allocations. */
#undef memset
#undef memcpy
#undef memcmp
#undef memmove
#undef malloc
#undef realloc
#undef free
#undef strlen
#undef strncmp
#undef strncpy
#include "mac_glsl120.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct text_buffer { char *data; size_t length, capacity; };

/* port: the GLSL version the driver takes. 120 (Mac OS X 10.5 and later) is
   the text this file has always made, byte for byte. 110 (Tiger's GLSL) is
   that text with the few 1.20-only constructs rewritten (downgrade_110). Set
   once, before the first shader, from the GL context (mac_window.c). */
static int glsl_version = 120;

void mac_glsl120_set_version(int version)
{ glsl_version = version == 110 ? 110 : 120; }

int mac_glsl120_version(void)
{ return glsl_version; }

/* HALO_MAC_GLSL_DUMP=<existing folder>: every shader text handed to the driver,
   once, as <kind>-<hash>.vert or .frag. For checking the text with a GLSL
   validator; nothing else reads it. */
void mac_glsl_dump(const char *kind, const char *source)
{
    const char *folder = getenv("HALO_MAC_GLSL_DUMP");
    unsigned long hash = 2166136261UL;
    const unsigned char *p;
    char path[512];
    FILE *file;
    if (!folder || !*folder || !kind || !source) return;
    for (p = (const unsigned char *)source; *p; ++p) hash = ((hash ^ *p) * 16777619UL) & 0xffffffffUL;
    if (strlen(folder) > 400) return;
    sprintf(path, "%s/%s-%08lx.%s", folder, kind, hash, kind[0] == 'v' ? "vert" : "frag");
    file = fopen(path, "rb");
    if (file) { fclose(file); return; }
    file = fopen(path, "wb");
    if (!file) return;
    fwrite(source, 1, strlen(source), file);
    fclose(file);
}

static int append(struct text_buffer *out, const char *data, size_t length)
{
    size_t capacity;
    char *grown;
    if (length > (size_t)-1 - out->length - 1) return 0;
    if (out->length + length + 1 > out->capacity) {
        capacity = out->capacity ? out->capacity : 256;
        while (capacity < out->length + length + 1) {
            if (capacity > (size_t)-1 / 2) return 0;
            capacity *= 2;
        }
        grown = (char *)realloc(out->data, capacity);
        if (!grown) return 0;
        out->data = grown; out->capacity = capacity;
    }
    memcpy(out->data + out->length, data, length);
    out->length += length; out->data[out->length] = 0;
    return 1;
}

static void set_error(char *error, size_t size, const char *message)
{
    if (error && size) {
        strncpy(error, message, size - 1);
        error[size - 1] = 0;
    }
}

static int starts(const char *line, const char *prefix)
{ return strncmp(line, prefix, strlen(prefix)) == 0; }

static int identifier_char(char value);

static int transform_lines(const char *source, int vertex,
    struct text_buffer *out, unsigned long *attribute_mask,
    char *error, size_t error_size)
{
    const char *line = source;
    int saw_version = 0;
    while (*line) {
        const char *end = strchr(line, '\n');
        size_t length = end ? (size_t)(end - line) : strlen(line);
        const char *body = line;
        size_t body_length = length;
        char line_copy[256];
        char replacement[128];
        int replacement_length = -1;
        if (length < sizeof(line_copy)) { memcpy(line_copy,line,length); line_copy[length]=0; }
        else { set_error(error,error_size,"generated shader line exceeds compatibility limit"); return 0; }
        if (length == 17 && memcmp(line, "#version 450 core", 17) == 0) {
            if (saw_version) { set_error(error,error_size,"duplicate shader version"); return 0; }
            saw_version = 1;
            body = "#version 120"; body_length = 12;
        } else if (vertex && length == 22 && memcmp(line,"invariant gl_Position;",22)==0) {
            body_length = 0;
        } else if (vertex && starts(line,"layout(location = ")) {
            unsigned long location, attribute; int consumed = 0;
            if (sscanf(line_copy,"layout(location = %lu) in vec4 v%lu_in;%n",
                &location, &attribute, &consumed) != 2 || location != attribute ||
                consumed != (int)length || location >= 16) {
                set_error(error,error_size,"unsupported vertex layout declaration"); return 0;
            }
            replacement_length = snprintf(replacement,sizeof(replacement),"attribute vec4 v%lu_in;",location);
            if (replacement_length < 0 || (size_t)replacement_length >= sizeof(replacement)) return 0;
            *attribute_mask |= 1UL << location;
        } else if (!vertex && starts(line,"layout(location = 0) out vec4 fragment_color;")) {
            body_length = 0;
        } else if ((vertex && (starts(line,"out vec4 ") || starts(line,"out vec3 ") || starts(line,"out float "))) ||
                   (!vertex && (starts(line,"in vec4 ") || starts(line,"in vec3 ") || starts(line,"in float ")))) {
            replacement_length = snprintf(replacement,sizeof(replacement),"varying %s",
                vertex ? line_copy + 4 : line_copy + 3);
            if (replacement_length < 0 || (size_t)replacement_length >= sizeof(replacement)) return 0;
        } else if ((vertex && starts(line,"out ")) || (!vertex && starts(line,"in "))) {
            set_error(error,error_size,"unsupported shader interface type"); return 0;
        }
        if (!saw_version) { set_error(error,error_size,"missing GLSL 4.50 version line"); return 0; }
        if (body_length) {
            if (replacement_length >= 0) {
                if (!append(out,replacement,(size_t)replacement_length)) return 0;
            } else if (!append(out,body,body_length)) return 0;
            if (!end || !append(out,"\n",1)) {
                if (end) return 0;
            }
        }
        line = end ? end + 1 : line + length;
    }
    return saw_version;
}

/* Remove the generator's unused packed-attribute helper. Its uint and bit
   shifts are not legal GLSL 1.20; packed attribute declarations are rejected. */
static int remove_packed_helper(struct text_buffer *text)
{
    const char *start = strstr(text->data,"vec4 unpack_normpacked3(uint p)\n{\n");
    const char *end;
    size_t prefix, suffix;
    if (!start) return 1;
    end = strstr(start,"\n}\n");
    if (!end) return 0;
    end += 3;
    prefix = (size_t)(start - text->data); suffix = text->length - (size_t)(end - text->data);
    memmove(text->data + prefix,text->data + (end - text->data),suffix + 1);
    text->length = prefix + suffix;
    return 1;
}

static int has_word(const char *text, const char *word)
{
    const char *p = text; size_t n = strlen(word);
    while ((p = strstr(p,word)) != NULL) {
        int left = p == text || !(p[-1] == '_' || (p[-1] >= 'A' && p[-1] <= 'Z') ||
            (p[-1] >= 'a' && p[-1] <= 'z') || (p[-1] >= '0' && p[-1] <= '9'));
        int right = !(p[n] == '_' || (p[n] >= 'A' && p[n] <= 'Z') ||
            (p[n] >= 'a' && p[n] <= 'z') || (p[n] >= '0' && p[n] <= '9'));
        if (left && right) return 1;
        p += n;
    }
    return 0;
}

static int identifier_char(char value)
{
    return value == '_' || (value >= 'a' && value <= 'z') ||
        (value >= 'A' && value <= 'Z') || (value >= '0' && value <= '9');
}

static size_t occurrences(const char *text, const char *needle)
{
    size_t count = 0, length = strlen(needle);
    const char *at = text;
    while ((at = strstr(at, needle)) != NULL) { ++count; at += length; }
    return count;
}

static size_t identifier_occurrences(const char *text, const char *identifier)
{
    size_t count = 0, length = strlen(identifier);
    const char *at = text;
    while ((at = strstr(at, identifier)) != NULL) {
        if ((at == text || !identifier_char(at[-1])) && !identifier_char(at[length])) ++count;
        at += length;
    }
    return count;
}

static int replace_exact(struct text_buffer *text, const char *needle,
    const char *replacement)
{
    struct text_buffer out = {0,0,0};
    const char *cursor = text->data, *match;
    size_t length = strlen(needle);
    while ((match = strstr(cursor, needle)) != NULL) {
        if (!append(&out, cursor, (size_t)(match - cursor)) ||
            !append(&out, replacement, strlen(replacement))) {
            free(out.data); return 0;
        }
        cursor = match + length;
    }
    if (!append(&out, cursor, strlen(cursor))) { free(out.data); return 0; }
    free(text->data); *text = out; return 1;
}

/* The shared vertex generator emits all sixteen input declarations and
   copies them into locals, even when the translated program never reads a
   particular local. Keep the input ABI unchanged, but let GLSL 1.20's linker
   remove only this exact generated pair when both identifier-use counts prove
   it is unused. Unknown or changed source grammar is left untouched. */
static int remove_unused_vertex_inputs(struct text_buffer *text,
    unsigned long *attribute_mask)
{
    unsigned index;
    unsigned long retained = *attribute_mask;
    for (index = 0; index < 16; ++index) {
        char input_name[24], local_name[16], declaration[48], initializer[48];
        int n1, n2;
        unsigned long bit = 1UL << index;
        if (!(retained & bit)) continue;
        n1 = snprintf(input_name, sizeof(input_name), "v%u_in", index);
        n2 = snprintf(local_name, sizeof(local_name), "v%u", index);
        if (n1 < 0 || (size_t)n1 >= sizeof(input_name) ||
            n2 < 0 || (size_t)n2 >= sizeof(local_name)) continue;
        n1 = snprintf(declaration, sizeof(declaration),
            "attribute vec4 %s;\n", input_name);
        n2 = snprintf(initializer, sizeof(initializer),
            "\tvec4 %s = %s;\n", local_name, input_name);
        if (n1 < 0 || (size_t)n1 >= sizeof(declaration) ||
            n2 < 0 || (size_t)n2 >= sizeof(initializer)) continue;
        if (occurrences(text->data, declaration) == 1 &&
            occurrences(text->data, initializer) == 1 &&
            identifier_occurrences(text->data, local_name) == 1 &&
            identifier_occurrences(text->data, input_name) == 2) {
            if (!replace_exact(text, declaration, "") ||
                !replace_exact(text, initializer, "")) return 0;
            retained &= ~bit;
        }
    }
    *attribute_mask = retained;
    return 1;
}

/* Radeon R300's GLSL compiler falls back to CPU vertex processing for the
   generated integer ARL storage in skinned shaders. Preserve floor and every
   bounded read, but keep its integer-valued result in a float until indexing.
   Only promote the exact generator grammar; unknown address uses stay intact.
   Validated model indices are small, exactly representable integers. */
static int floating_address_register(struct text_buffer *text)
{
    static const char declaration[] = "\tint a0 = 0;\n";
    static const char assignment[] = "\ta0 = int(floor(mac.x + 0.001));\n";
    const char *at = text->data;
    size_t words = 0, writes, reads = 0;
    if (occurrences(text->data, declaration) != 1) return 1;
    writes = occurrences(text->data, assignment);
    if (!writes) return 1;
    while ((at = strstr(at, "a0")) != NULL) {
        if ((at == text->data || !identifier_char(at[-1])) &&
            !identifier_char(at[2])) ++words;
        at += 2;
    }
    at = text->data;
    while ((at = strstr(at, "c[clamp(a0 + ")) != NULL) {
        unsigned long offset;
        int consumed = 0;
        if (sscanf(at, "c[clamp(a0 + %lu, 0, 191)]%n", &offset, &consumed) != 1 ||
            !consumed || offset > 255) return 1;
        ++reads; at += consumed;
    }
    if (!reads || words != 1 + writes + reads) return 1;
    return replace_exact(text, declaration, "\tfloat a0 = 0.0;\n") &&
        replace_exact(text, assignment, "\ta0 = floor(mac.x + 0.001);\n");
}

/* GLSL 1.20 has floating-point clamp overloads only. The NV2A generator
   bounds relative constant reads to [0,191]; this preserves that bound and
   converts the result back to an integer array index. Add in floating point
   so a large address cannot overflow an integer before being clamped. With
   offsets in [0,255], any address yielding an interior index is in [-255,191]
   and exactly representable; larger magnitudes remain on the same endpoint. */
static int relative_constants(struct text_buffer *text)
{
    struct text_buffer out = {0,0,0};
    const char *cursor=text->data,*match;
    while((match=strstr(cursor,"c[clamp("))!=NULL) {
        unsigned long offset;
        int consumed=0,length;
        char replacement[96];
        if(sscanf(match,"c[clamp(a0 + %lu, 0, 191)]%n",&offset,&consumed)!=1 ||
           !consumed || offset>255) {free(out.data);return 0;}
        length=snprintf(replacement,sizeof(replacement),
            "c[int(clamp(float(a0) + %lu.0, 0.0, 191.0))]",offset);
        if(length<0 || (size_t)length>=sizeof(replacement) ||
           !append(&out,cursor,(size_t)(match-cursor)) ||
           !append(&out,replacement,(size_t)length)) {free(out.data);return 0;}
        cursor=match+consumed;
    }
    if(!append(&out,cursor,strlen(cursor))) {free(out.data);return 0;}
    free(text->data);*text=out;return 1;
}

/* The shared generator declares each of its four samplers explicitly.
   GLSL 1.20 chooses the lookup function by that declared dimensionality. */
static int texture_samples(struct text_buffer *text)
{
    struct text_buffer out={0,0,0};const char *cursor=text->data,*match;
    const char *function[4]={NULL,NULL,NULL,NULL};
    static const char *types[3]={"sampler2D","sampler3D","samplerCube"};
    static const char *calls[3]={"texture2D","texture3D","textureCube"};
    unsigned stage,type;
    for(stage=0;stage<4;++stage)for(type=0;type<3;++type) {
        char declaration[48];
        sprintf(declaration,"uniform %s tex%u;",types[type],stage);
        if(strstr(text->data,declaration)) {
            if(function[stage])return 0;
            function[stage]=calls[type];
        }
    }
    while((match=strstr(cursor,"texture("))!=NULL) {
        int consumed=0;char replacement[48];int length;
        if(sscanf(match,"texture(tex%u,%n",&stage,&consumed)!=1 ||
           !consumed || stage>=4 || !function[stage]) {free(out.data);return 0;}
        length=sprintf(replacement,"%s(tex%u,",function[stage],stage);
        if(!append(&out,cursor,(size_t)(match-cursor)) ||
           !append(&out,replacement,(size_t)length)) {free(out.data);return 0;}
        cursor=match+consumed;
    }
    if(!append(&out,cursor,strlen(cursor))) {free(out.data);return 0;}
    free(text->data);*text=out;return 1;
}

/* D3D clip depth is [0,w], while desktop OpenGL clips against [-w,w].
   The Xbox rasterizer's viewport constants have already been undone by the
   NV2A generator, so remap the final position exactly once at this boundary.
   The GLES generator has its own clip-control equivalent and does not use
   this Mac GLSL 1.20 path. */
static int remap_clip_depth(struct text_buffer *text)
{
    static const char assignment[] = "gl_Position =";
    static const char remap[] = "\n\tgl_Position.z = 2.0 * gl_Position.z - gl_Position.w;";
    char *match = strstr(text->data, assignment);
    char *end;
    char *grown;
    size_t offset, tail, remap_size = sizeof(remap) - 1;
    if (!match || strstr(match + sizeof(assignment) - 1, assignment))
        return 0;
    end = strchr(match + sizeof(assignment) - 1, ';');
    if (!end)
        return 0;
    offset = (size_t)(end - text->data) + 1;
    tail = text->length - offset;
    if (remap_size > (size_t)-1 - text->length - 1)
        return 0;
    grown = (char *)realloc(text->data, text->length + remap_size + 1);
    if (!grown)
        return 0;
    text->data = grown;
    memmove(text->data + offset + remap_size, text->data + offset, tail + 1);
    memcpy(text->data + offset, remap, remap_size);
    text->length += remap_size;
    return 1;
}

/* port: the text made for 1.20, as GLSL 1.10 takes it (nothing yet) */
static int downgrade_110(struct text_buffer *text)
{
    return replace_exact(text, "#version 120\n", "#version 110\n");
}

int mac_glsl120_vertex(const char *source, char **translated,
    unsigned long *attribute_mask, char *error, size_t error_size)
{
    struct text_buffer out = {0,0,0};
    unsigned long mask = 0;
    if (translated) *translated = NULL;
    if (attribute_mask) *attribute_mask = 0;
    if (!source || !translated || !attribute_mask || !starts(source,"#version 450 core\n")) {
        set_error(error,error_size,"invalid generated vertex shader input"); return 0;
    }
    if (!transform_lines(source,1,&out,&mask,error,error_size) || !remove_packed_helper(&out) ||
        !floating_address_register(&out) || !relative_constants(&out) || !remap_clip_depth(&out) ||
        !remove_unused_vertex_inputs(&out,&mask) ||
        has_word(out.data,"uint") || has_word(out.data,"layout") || has_word(out.data,"flat")) {
        free(out.data); set_error(error,error_size,"vertex shader uses unsupported GLSL 1.20 features or lacks one final position"); return 0;
    }
    if (!mask) { free(out.data); set_error(error,error_size,"vertex shader has no ordinary attributes"); return 0; }
    if (glsl_version == 110 && !downgrade_110(&out)) {
        free(out.data); set_error(error,error_size,"vertex shader cannot be made GLSL 1.10"); return 0;
    }
    mac_glsl_dump("vertex", out.data);
    *translated=out.data; *attribute_mask=mask; return 1;
}

int mac_glsl120_pixel(const char *source, char **translated,
    char *error, size_t error_size)
{
    struct text_buffer out = {0,0,0};
    unsigned long unused = 0;
    char *p;
    if (translated) *translated = NULL;
    if (!source || !translated || !starts(source,"#version 450 core\n")) {
        set_error(error,error_size,"invalid generated pixel shader input"); return 0;
    }
    if (strstr(source,"gl_SampleMask") || strstr(source,"atomicCounter") || strstr(source,"fwidth(") ||
        has_word(source,"textureSize") || has_word(source,"texelFetch") || has_word(source,"textureLod")) {
        set_error(error,error_size,"pixel shader uses unsupported texture or sample feature"); return 0;
    }
    if (!transform_lines(source,0,&out,&unused,error,error_size) || !texture_samples(&out) || has_word(out.data,"texture(") || has_word(out.data,"layout") ||
        has_word(out.data,"in") || has_word(out.data,"out") || has_word(out.data,"uint")) {
        free(out.data); set_error(error,error_size,"pixel shader uses unsupported GLSL 1.20 features"); return 0;
    }
    p=out.data;
    while ((p=strstr(p,"fragment_color"))!=NULL) {
        const char replacement[]="gl_FragColor";
        size_t old_length = sizeof("fragment_color") - 1;
        size_t new_length = sizeof(replacement) - 1;
        size_t tail = out.length - (size_t)(p - out.data) - old_length;
        if (new_length != old_length)
            memmove(p + new_length,p + old_length,tail + 1);
        memcpy(p,replacement,new_length);
        out.length = out.length - old_length + new_length;
        p+=sizeof(replacement)-1;
    }
    if (glsl_version == 110 && !downgrade_110(&out)) {
        free(out.data); set_error(error,error_size,"pixel shader cannot be made GLSL 1.10"); return 0;
    }
    mac_glsl_dump("pixel", out.data);
    *translated=out.data; return 1;
}
