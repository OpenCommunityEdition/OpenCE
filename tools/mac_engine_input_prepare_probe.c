/* Tests the native adapter against actual engine types/constants; no game
   service stubs, biped update, world aiming or queue delivery are involved. */
#include "engine_input.h"
#include "mac_input.h"
#include "game/players.h"
#include "units/units.h"
#include "units/unit_control_data.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
/* Native adapter owns this memory; no engine allocator/service is involved. */
#undef memset
#undef memcmp
#undef vprintf
#undef vfprintf
static unsigned checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; printf("FAIL line %d: %s\n", __LINE__, #x); } } while (0)

/* Only the engine's byte-clear helper is supplied by this pure adapter test. */
void *csmemset(void *buffer, long value, unsigned long size)
{
	return memset(buffer, (int)value, (size_t)size);
}

int halo_linux_printf(const char *format, ...)
{
	va_list arguments;
	int result;
	va_start(arguments, format);
	result = vprintf(format, arguments);
	va_end(arguments);
	return result;
}

int halo_linux_fprintf(FILE *stream, const char *format, ...)
{
	va_list arguments;
	int result;
	va_start(arguments, format);
	result = vfprintf(stream, format, arguments);
	va_end(arguments);
	return result;
}
int main(void)
{
    struct mac_input input;
    struct mac_input_frame frame;
    struct mac_engine_input prepared;
    struct player_control control, before;
    struct player_action action;
    unsigned mask;
    unsigned long expected;
    CHECK(sizeof(struct player_action) == 0x20);
    CHECK(sizeof(struct unit_control_data) == 0x40);
    CHECK(sizeof(struct player_control) == 0x40);
    CHECK(offsetof(struct player_action, desired_facing) == 4);
    CHECK(offsetof(struct player_action, throttle) == 12);
    memset(&control, 0, sizeof(control));
    control.desired_angles.yaw = 1.25f;
    control.desired_angles.pitch = -0.25f;
    control.desired_weapon_index = 2;
    control.desired_grenade_index = 1;
    control.zoom_level = -1;
    before = control;
    memset(&frame, 0, sizeof(frame));
    frame.forward = 0.5f; frame.left = -0.25f;
    frame.yaw = 0.125f; frame.pitch = -0.0625f;
    /* Exhaust all action subsets to detect accidental local-mask copying. */
    for (mask = 0; mask < 64; ++mask) {
        frame.held = frame.pressed = mask;
        expected = 0;
        if (mask & MAC_INPUT_JUMP) expected |= FLAG(_unit_control_jump_bit);
        if (mask & MAC_INPUT_CROUCH) expected |= FLAG(_unit_control_crouch_modifier_bit);
        if (mask & MAC_INPUT_ACTION) expected |= FLAG(_unit_control_action_bit) | FLAG(UNIT_CONTROL_PORT_ACTION_ONLY_BIT);
        if (mask & MAC_INPUT_FIRE) expected |= FLAG(_unit_control_weapon_primary_trigger_bit);
        if (mask & MAC_INPUT_GRENADE) expected |= FLAG(_unit_control_throw_grenade_bit) | FLAG(_unit_control_weapon_secondary_trigger_bit);
        mac_engine_input_prepare(&frame, 63, 1, 1, &prepared);
        CHECK(prepared.unit_control_flags == expected);
        CHECK(prepared.pause_pressed == !!(mask & MAC_INPUT_PAUSE));
        CHECK(prepared.primary_trigger == ((mask & MAC_INPUT_FIRE) ? 1.f : 0.f));
        CHECK(prepared.facing_delta.yaw == frame.yaw && prepared.facing_delta.pitch == frame.pitch);
        mac_engine_input_action(&prepared, &control, &action);
        CHECK(action.control_flags == expected);
        CHECK(action.throttle.i == frame.forward && action.throttle.j == frame.left);
        CHECK(action.desired_facing.yaw == 1.25f && action.desired_facing.pitch == -0.25f);
        CHECK(action.desired_weapon_index == 2 && action.desired_grenade_index == 1 && action.desired_zoom_level == -1);
        CHECK(action.pad == 0 && !memcmp(&control, &before, sizeof(control)));
        mac_engine_input_prepare(&frame, 0, 0, 0, &prepared);
        CHECK(prepared.unit_control_flags == 0 && prepared.primary_trigger == 0 && !prepared.pause_pressed);
        CHECK(prepared.throttle.i == 0 && prepared.throttle.j == 0);
        CHECK(prepared.facing_delta.yaw == 0 && prepared.facing_delta.pitch == 0);
    }
    mac_input_init(&input); mac_input_focus(&input, 1);
    mac_input_key(&input, 13, 1); mac_input_key(&input, 0, 1);
    mac_input_key(&input, 49, 1); mac_input_button(&input, 1, 1);
    mac_input_motion(&input, 20, -10);
    mac_input_snapshot(&input, &frame);
    mac_engine_input_prepare(&frame, MAC_INPUT_JUMP, 1, 1, &prepared);
    CHECK(prepared.throttle.i == 0.70710678f && prepared.throttle.j == 0.70710678f);
    CHECK(prepared.unit_control_flags == FLAG(_unit_control_jump_bit) && prepared.primary_trigger == 0);
    CHECK(prepared.facing_delta.yaw == -20.f * 0.0022f && prepared.facing_delta.pitch == 10.f * 0.0022f);
    mac_input_snapshot(&input, &frame);
    mac_engine_input_prepare(&frame, 63, 1, 1, &prepared);
    CHECK(prepared.facing_delta.yaw == 0 && prepared.facing_delta.pitch == 0);
    CHECK(prepared.unit_control_flags == (FLAG(_unit_control_jump_bit) | FLAG(_unit_control_weapon_primary_trigger_bit)));
    mac_input_focus(&input, 0); mac_input_snapshot(&input, &frame);
    mac_engine_input_prepare(&frame, 63, 1, 1, &prepared);
    mac_engine_input_action(&prepared, &control, &action);
    CHECK(action.control_flags == 0 && action.primary_trigger == 0 && action.throttle.i == 0 && action.throttle.j == 0);
    printf("engine input boundary: %u checks, %u failures; no simulated movement\n", checks, failures);
    return failures ? 1 : 0;
}
