/*
UI_WIDGET_DEFINITIONS.H

January-derived UI widget tag-definition prefix.
*/

#ifndef __UI_WIDGET_DEFINITIONS_H
#define __UI_WIDGET_DEFINITIONS_H
#pragma once

/* ---------- headers */

#include "tag_files/tag_groups.h"

/* ---------- constants */

enum
{
	UI_WIDGET_DEFINITION_TAG = 'DeLa'
};

/* ---------- macros */

#define ui_widget_definition_get(index) \
	((struct ui_widget_definition *)tag_get(UI_WIDGET_DEFINITION_TAG, (index)))

/* ---------- structures */

struct ui_widget_definition
{
	short type;
	byte unknown002[0x3DE];
	long child_count;
};

#endif // __UI_WIDGET_DEFINITIONS_H
