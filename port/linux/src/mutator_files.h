/*
MUTATOR_FILES.H

The stock mutators (port/assets/mutators: a folder each, with its
mutator.toml), embedded by tools/embed_assets.py; port_mutators.c writes
them into the data root's stock_mutators folder and plays them.
*/

#ifndef MUTATOR_FILES_H
#define MUTATOR_FILES_H

/* a stock mutator: its folder's name, and its mutator.toml (size bytes,
then a NUL) */
struct mutator_file_embedded
{
	const char *folder;
	const unsigned int *data;
	unsigned int size;
};

extern const struct mutator_file_embedded mutator_files_embedded[];
extern const unsigned int mutator_files_embedded_count;

#endif
