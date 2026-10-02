/*
UI_WIDGET_GAME_DATA_INPUT_FUNCTIONS.H

header included in hcex build.
*/

#ifndef __UI_WIDGET_GAME_DATA_INPUT_FUNCTIONS_H
#define __UI_WIDGET_GAME_DATA_INPUT_FUNCTIONS_H
#pragma once

/* ---------- headers */

#include "cseries/cseries.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct widget_instance;

typedef void (*ui_widget_game_data_function)(
	struct widget_instance *widget);

/* ---------- prototypes/UI_WIDGET_GAME_DATA_INPUT_FUNCTIONS.C */

void ui_widget_game_data_function_invoke(
	struct widget_instance *widget,
	word function);

/* ---------- globals */

/* ---------- public code */

#endif // __UI_WIDGET_GAME_DATA_INPUT_FUNCTIONS_H
