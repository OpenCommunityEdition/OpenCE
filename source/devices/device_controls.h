/*
DEVICE_CONTROLS.H
*/

#ifndef __DEVICE_CONTROLS_H
#define __DEVICE_CONTROLS_H
#pragma once

/* ---------- headers */

#include "device_definitions.h"
#include "devices.h"

/* ---------- constants */

enum
{
	CONTROL_DEFINITION_TAG = 'ctrl',
};

/* ---------- macros */

#define control_definition_get(index) ((struct control_definition *)tag_get(CONTROL_DEFINITION_TAG, (index)))
#define control_get(index) ((struct control_datum *)object_get_and_verify_type((index), _object_mask_control))
#define control_try_and_get(index) ((struct control_datum *)object_try_and_get_and_verify_type((index), _object_mask_control))

/* ---------- structures */

struct control_definition;

struct control_datum
{
	long definition_index;
	struct _object_datum object;
	struct _device_datum device;
};

/* ---------- prototypes/DEVICE_CONTROLS.C */

void controls_initialize(
	void);
void controls_dispose(
	void);
void controls_initialize_for_new_map(
	void);
void controls_dispose_from_old_map(
	void);
boolean control_new(
	long control_index);
void control_delete(
	long control_index);
boolean control_update(
	long control_index);

#endif // __DEVICE_CONTROLS_H
