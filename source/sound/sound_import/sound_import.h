/*
SOUND_IMPORT.H

header included in hcex build.
*/

#ifndef __SOUND_IMPORT_H
#define __SOUND_IMPORT_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct file_reference;
struct sound_file_info;

/* ---------- prototypes/EXAMPLE.C */

/* ---------- globals */

/* ---------- public code */

boolean sound_file_info_get(struct sound_file_info *info, struct file_reference const *file);

#endif // __SOUND_IMPORT_H
