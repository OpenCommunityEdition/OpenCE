/* No windows, handler installation, synthetic global input or cursor changes. */
#include "../port/mac/mac_input.h"
#include <stdio.h>
#include <string.h>
static unsigned checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; printf("FAIL line %d: %s\n", __LINE__, #x); } } while (0)
#if defined(__APPLE__) && !defined(__LP64__)
static void event(struct mac_input *input, UInt32 cls, UInt32 kind,
    EventParamName name, EventParamType type, UInt32 size, const void *value)
{
    EventRef e = NULL;
    OSStatus status = CreateEvent(NULL, cls, kind, 0, 0, &e);
    CHECK(status == noErr);
    if (status != noErr) return;
    if (value) CHECK(SetEventParameter(e, name, type, size, value) == noErr);
    CHECK(mac_input_event(NULL, e, input) == eventNotHandledErr);
    ReleaseEvent(e);
}
#endif
int main(void)
{
    struct mac_input input;
    struct mac_input_frame frame;
    unsigned int i;
    mac_input_init(&input);
    mac_input_key(&input, 13, 1);
    mac_input_snapshot(&input, &frame);
    CHECK(frame.forward == 0);
    mac_input_focus(&input, 1);
    mac_input_key(&input, 13, 1);
    mac_input_snapshot(&input, &frame);
    CHECK(frame.forward == 1 && frame.left == 0);
    mac_input_key(&input, 0, 1);
    mac_input_snapshot(&input, &frame);
    CHECK(frame.forward == 0.70710678f && frame.left == 0.70710678f);
    mac_input_key(&input, 1, 1);
    mac_input_key(&input, 2, 1);
    mac_input_snapshot(&input, &frame);
    CHECK(frame.forward == 0 && frame.left == 0);
    mac_input_key(&input, 49, 1);
    mac_input_key(&input, 49, 1);
    mac_input_key(&input, 49, 0);
    mac_input_snapshot(&input, &frame);
    CHECK(frame.held == 0 && frame.pressed == MAC_INPUT_JUMP && frame.released == MAC_INPUT_JUMP);
    mac_input_snapshot(&input, &frame);
    CHECK(frame.pressed == 0 && frame.released == 0);
    mac_input_motion(&input, 10, -5);
    mac_input_snapshot(&input, &frame);
    CHECK(frame.yaw == -10.0f * 0.0022f && frame.pitch == 5.0f * 0.0022f);
    mac_input_snapshot(&input, &frame);
    CHECK(frame.yaw == 0 && frame.pitch == 0);
    for (i = 0; i < 100; ++i) mac_input_motion(&input, 1e30f, -1e30f);
    CHECK(input.dx == 4096 && input.dy == -4096);
    mac_input_key(&input, 999, 1);
    mac_input_button(&input, 999, 1);
    mac_input_button(&input, 1, 1);
    mac_input_key(&input, 59, 1);
    mac_input_snapshot(&input, &frame);
    CHECK(frame.held == (MAC_INPUT_CROUCH | MAC_INPUT_FIRE));
    mac_input_focus(&input, 0);
    mac_input_motion(&input, 5, 6);
    mac_input_button(&input, 2, 1);
    mac_input_snapshot(&input, &frame);
    CHECK(frame.held == 0 && frame.forward == 0 && frame.left == 0 && frame.yaw == 0 && frame.pitch == 0);
    CHECK(frame.released == (MAC_INPUT_CROUCH | MAC_INPUT_FIRE));
    mac_input_focus(&input, 1);
    mac_input_snapshot(&input, &frame);
    CHECK(frame.held == 0 && frame.pressed == 0 && frame.released == 0);
#if defined(__APPLE__) && !defined(__LP64__)
    {
        UInt32 key = 13, modifiers = controlKey;
        EventMouseButton button = 1;
        HIPoint delta;
        delta.x = 4; delta.y = -8;
        event(&input, kEventClassKeyboard, kEventRawKeyDown,
            kEventParamKeyCode, typeUInt32, sizeof(key), &key);
        event(&input, kEventClassMouse, kEventMouseDown,
            kEventParamMouseButton, typeMouseButton, sizeof(button), &button);
        event(&input, kEventClassMouse, kEventMouseDragged,
            kEventParamMouseDelta, typeHIPoint, sizeof(delta), &delta);
        event(&input, kEventClassKeyboard, kEventRawKeyModifiersChanged,
            kEventParamKeyModifiers, typeUInt32, sizeof(modifiers), &modifiers);
        mac_input_snapshot(&input, &frame);
        CHECK(frame.forward == 1 && frame.held == (MAC_INPUT_FIRE | MAC_INPUT_CROUCH));
        CHECK(frame.yaw == -4.0f * 0.0022f && frame.pitch == 8.0f * 0.0022f);
        event(&input, kEventClassKeyboard, kEventRawKeyUp,
            kEventParamKeyCode, typeUInt32, sizeof(key), &key);
        event(&input, kEventClassMouse, kEventMouseUp,
            kEventParamMouseButton, typeMouseButton, sizeof(button), &button);
        modifiers = 0;
        event(&input, kEventClassKeyboard, kEventRawKeyModifiersChanged,
            kEventParamKeyModifiers, typeUInt32, sizeof(modifiers), &modifiers);
        mac_input_snapshot(&input, &frame);
        CHECK(frame.forward == 0 && frame.held == 0);
        event(&input, kEventClassApplication, kEventAppDeactivated, 0, 0, 0, NULL);
        CHECK(input.focused == 0);
        event(&input, kEventClassApplication, kEventAppActivated, 0, 0, 0, NULL);
        CHECK(input.focused == 1);
        event(&input, kEventClassWindow, kEventWindowDeactivated, 0, 0, 0, NULL);
        CHECK(input.focused == 0);
        event(&input, kEventClassWindow, kEventWindowActivated, 0, 0, 0, NULL);
        CHECK(input.focused == 1);
    }
    printf("Carbon event decoder exercised; no windows or global input events.\n");
#endif
    printf("mac input: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
