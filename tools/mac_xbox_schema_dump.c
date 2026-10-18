/* Prints the validator's schemas (port/linux/game/tag_schema_*.c), after the macros: for every group its
   definitions and fields, as lines for tools/mac_xbox_tag_tables.py.
   group <tag> <parent0> <parent1> <definition>
   def <name> <size>
   field <definition> <type> <size> <count> <offset> <maximum> <target definition|-> <field name>
   (a block's target is its element definition; a reference's, the groups) */
#include "cseries.h"
#include "tag_schema.h"
#undef printf
#include <stdio.h>

/* the checks the fields name call these, which the dump does not run */
boolean tag_validate_any_claimed(void const *address, unsigned long size) { (void)address; (void)size; return FALSE; }
boolean tag_validate_custom_edition(struct tag_validation *validation) { (void)validation; return FALSE; }
boolean tag_validate_file_contains(struct tag_validation *validation, long offset, long size) { (void)validation; (void)offset; (void)size; return FALSE; }
void *tag_validate_index_buffer_data(struct tag_validation *validation, void const *buffer) { (void)validation; (void)buffer; return NULL; }
void *tag_validate_vertex_buffer_data(struct tag_validation *validation, void const *buffer) { (void)validation; (void)buffer; return NULL; }
void *tag_validate_tag_get(struct tag_validation *validation, long tag_index, unsigned long group_tag) { (void)validation; (void)tag_index; (void)group_tag; return NULL; }

void tag_validate_refuse(struct tag_validation *validation, char const *format, ...) { (void)validation; (void)format; }
void tag_validate_correct(struct tag_validation *validation, char const *format, ...) { (void)validation; (void)format; }
void tag_validate_non_negative(struct tag_validation *validation, char const *name, real *value) { (void)validation; (void)name; (void)value; }
boolean tag_validate_contains(struct tag_validation *validation, void const *address, unsigned long size) { (void)validation; (void)address; (void)size; return FALSE; }
void *tag_validate_root(struct tag_validation *validation) { (void)validation; return NULL; }

#undef csmemset
#undef csstrcmp
void *csmemset(void *destination, long value, unsigned long size) { unsigned char *p = (unsigned char *)destination; while (size--) *p++ = (unsigned char)value; return destination; }
long csstrcmp(const char *a, const char *b) { while (*a && *a == *b) { ++a; ++b; } return (unsigned char)*a - (unsigned char)*b; }

static const struct tag_schema_definition *seen[4096];
static unsigned seen_count;
static void dump_definition(const struct tag_schema_definition *d);
static void tag_text(unsigned long t, char *out)
{
    int i;
    for (i = 0; i < 4; ++i) { unsigned char c = (unsigned char)(t >> (24 - 8 * i)); out[i] = c >= 32 && c < 127 && c != ' ' ? (char)c : '_'; }
    out[4] = 0;
}
static void dump_definition(const struct tag_schema_definition *d)
{
    const struct tag_schema_field *f;
    unsigned i;
    for (i = 0; i < seen_count; ++i) if (seen[i] == d) return;
    seen[seen_count++] = d;
    printf("def %s %ld\n", d->name, d->size);
    for (f = d->fields; f->type != _tag_schema_terminator; ++f) {
        const char *target = "-";
        if ((f->type == _tag_schema_block || f->type == _tag_schema_struct) && f->definition) {
            if (getenv("SCHEMA_DEBUG")) printf("# %s.%s -> %p name %p\n", d->name, f->name, f->definition, (void *)((const struct tag_schema_definition *)f->definition)->name);
            target = ((const struct tag_schema_definition *)f->definition)->name;
        }
        printf("field %s %d %d %d %ld %ld %s %s\n", d->name, f->type, f->size, f->count, f->offset, f->maximum, target, f->name ? f->name : "-");
    }
    for (f = d->fields; f->type != _tag_schema_terminator; ++f)
        if ((f->type == _tag_schema_block || f->type == _tag_schema_struct) && f->definition)
            dump_definition((const struct tag_schema_definition *)f->definition);
}
int main(void)
{
    struct tag_schema_group const *const *list;
    setvbuf(stdout, NULL, _IOLBF, 0);
    for (list = tag_schema_group_lists; *list; ++list) {
        const struct tag_schema_group *g;
        for (g = *list; g->group_tag; ++g) {
            char a[5], b[5], c[5];
            tag_text(g->group_tag, a); tag_text(g->parent_group_tags[0], b); tag_text(g->parent_group_tags[1], c);
            printf("group %s %s %s %s\n", a, b, c, g->definition ? g->definition->name : "-");
            if (g->definition) dump_definition(g->definition);
        }
    }
    return 0;
}
