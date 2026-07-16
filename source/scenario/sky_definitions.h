/*
SKY_DEFINITIONS.H

header included in hcex build.
*/

#ifndef __SKY_DEFINITIONS_H
#define __SKY_DEFINITIONS_H
#pragma once

/* ---------- headers */

#include "tag_files/tag_files.h"

/* ---------- constants */

#define SKY_DEFINITION_TAG 'sky '

/* ---------- macros */

#define sky_definition_get(index) ((struct sky *)tag_get(SKY_DEFINITION_TAG, (index)))

/* ---------- structures */

/* ---------- prototypes/EXAMPLE.C */

/* ---------- globals */

/* ---------- public code */

#endif // __SKY_DEFINITIONS_H
