#include "mac_input.h"
#include <string.h>

/* Physical ANSI key positions: W/S/A/D, space, control, E, escape. These
   virtual key codes are independent of the selected text keyboard layout. */
static void refresh(struct mac_input *input)
{
    unsigned int held = 0;
    if (input->keys[49]) held |= MAC_INPUT_JUMP;
    if (input->keys[59] || input->keys[62]) held |= MAC_INPUT_CROUCH;
    if (input->keys[14]) held |= MAC_INPUT_ACTION;
    if (input->keys[53]) held |= MAC_INPUT_PAUSE;
    if (input->buttons & 1) held |= MAC_INPUT_FIRE;
    if (input->buttons & 2) held |= MAC_INPUT_GRENADE;
    input->pressed |= held & ~input->held;
    input->released |= input->held & ~held;
    input->held = held;
}
void mac_input_init(struct mac_input *input)
{
    memset(input, 0, sizeof(*input));
}
void mac_input_focus(struct mac_input *input, int focused)
{
    if (!focused) {
        memset(input->keys, 0, sizeof(input->keys));
        input->buttons = 0;
        input->dx = input->dy = input->wheel = 0;
        input->pointer_valid=input->pointer_moved=0;
        input->left_clicks=input->right_clicks=0;
        input->pressed = 0;
        input->text_count = 0;
        refresh(input);
    }
    input->focused = !!focused;
}
void mac_input_key(struct mac_input *input, unsigned int key, int down)
{
    if (!input->focused || key >= sizeof(input->keys)) return;
    input->keys[key] = !!down;
    refresh(input);
}
void mac_input_character(struct mac_input *input, unsigned int key, char character)
{
    struct mac_input_text_event *event;
    if (!input->focused || key >= sizeof(input->keys) || !character || input->text_count >= 32) return;
    event=&input->text[input->text_count++];
    event->key=(unsigned char)key;
    event->character=character;
}
static void refresh_modifier_pair(struct mac_input *input, unsigned int held,
    unsigned int right_held, unsigned int left_key, unsigned int right_key)
{
    if (!held) {
        input->keys[left_key] = input->keys[right_key] = 0;
    } else {
        if (right_held) input->keys[right_key] = 1;
        if (!input->keys[left_key] && !input->keys[right_key])
            input->keys[left_key] = 1;
    }
}
void mac_input_modifiers(struct mac_input *input, unsigned int modifiers)
{
    if (!input->focused) return;
    refresh_modifier_pair(input, modifiers & (MAC_INPUT_MOD_SHIFT | MAC_INPUT_MOD_RIGHT_SHIFT),
        modifiers & MAC_INPUT_MOD_RIGHT_SHIFT, 56, 60);
    refresh_modifier_pair(input, modifiers & (MAC_INPUT_MOD_CONTROL | MAC_INPUT_MOD_RIGHT_CONTROL),
        modifiers & MAC_INPUT_MOD_RIGHT_CONTROL, 59, 62);
    refresh_modifier_pair(input, modifiers & (MAC_INPUT_MOD_OPTION | MAC_INPUT_MOD_RIGHT_OPTION),
        modifiers & MAC_INPUT_MOD_RIGHT_OPTION, 58, 61);
    if (!(modifiers & MAC_INPUT_MOD_COMMAND))
        input->keys[54] = input->keys[55] = 0;
    else if (!input->keys[54] && !input->keys[55])
        input->keys[55] = 1;
    refresh(input);
}
void mac_input_button(struct mac_input *input, unsigned int button, int down)
{
    unsigned int mask;
    if (!input->focused || button < 1 || button > 5) return;
    mask = 1U << (button - 1);
    if (down) input->buttons |= mask;
    else input->buttons &= ~mask;
    refresh(input);
}
static float bounded(float value)
{
    if (value > 4096.0f) return 4096.0f;
    if (value < -4096.0f) return -4096.0f;
    return value;
}
void mac_input_motion(struct mac_input *input, float dx, float dy)
{
    if (!input->focused || input->raw_motion) return;
    mac_input_raw_motion(input, dx, dy);
}
void mac_input_raw_motion(struct mac_input *input, float dx, float dy)
{
    if (!input->focused) return;
    /* NaN is ignored; infinities saturate. Bound before adding as well. */
    if (dx == dx) input->dx = bounded(input->dx + bounded(dx));
    if (dy == dy) input->dy = bounded(input->dy + bounded(dy));
}
void mac_input_wheel(struct mac_input *input, float delta)
{
    if (!input->focused || delta != delta) return;
    input->wheel = bounded(input->wheel + bounded(delta));
}
void mac_input_pointer(struct mac_input *input,float x,float y,int moved,unsigned int button_down)
{
    if(!input->focused||x!=x||y!=y)return;
    input->pointer_x=x;input->pointer_y=y;input->pointer_valid=1;
    if(moved)input->pointer_moved=1;
    if(button_down==1&&input->left_clicks<255)input->left_clicks++;
    else if(button_down==2&&input->right_clicks<255)input->right_clicks++;
    if(button_down==1||button_down==2){input->click_x=x;input->click_y=y;}
}
void mac_input_snapshot(struct mac_input *input, struct mac_input_frame *frame)
{
    float scale;
    memset(frame, 0, sizeof(*frame));
    frame->forward = (float)(input->keys[13] - input->keys[1]);
    frame->left = (float)(input->keys[0] - input->keys[2]);
    /* Same unit-circle keyboard movement as xinput_sdl.c. */
    scale = frame->forward && frame->left ? 0.70710678f : 1.0f;
    frame->forward *= scale;
    frame->left *= scale;
    frame->yaw = -input->dx * 0.0022f;
    frame->pitch = -input->dy * 0.0022f;
    frame->held = input->held;
    frame->pressed = input->pressed;
    frame->released = input->released;
    frame->mouse_buttons = input->buttons;
    frame->wheel = input->wheel;
    frame->pointer_x=input->pointer_x;frame->pointer_y=input->pointer_y;
    frame->click_x=input->click_x;frame->click_y=input->click_y;
    frame->pointer_valid=input->pointer_valid;frame->pointer_moved=input->pointer_moved;
    frame->left_clicks=input->left_clicks;frame->right_clicks=input->right_clicks;
    memcpy(frame->keys, input->keys, sizeof(frame->keys));
    frame->text_count=input->text_count;
    memcpy(frame->text,input->text,(size_t)input->text_count*sizeof(input->text[0]));
    input->text_count=0;
    input->pressed = input->released = 0;
    input->dx = input->dy = input->wheel = 0;
    input->pointer_moved=0;input->left_clicks=input->right_clicks=0;
}
int mac_input_rumble(unsigned int gamepad_index,unsigned short left,unsigned short right)
{
    /* Carbon keyboard/mouse input exposes no gamepad or haptic device. Report
       absence explicitly; do not claim that feedback reached hardware. */
    (void)gamepad_index;(void)left;(void)right;
    return 0;
}
#if defined(__APPLE__) && !defined(__LP64__) && !defined(MAC_INPUT_NO_CARBON)
void (*mac_input_focus_lost_hook)(void);
OSStatus mac_input_event(EventHandlerCallRef next, EventRef event, void *context)
{
    struct mac_input *input = (struct mac_input *)context;
    UInt32 kind = GetEventKind(event), cls = GetEventClass(event), key, modifiers;
    EventMouseButton button;
    SInt32 wheel_delta;
    HIPoint delta;
    (void)next;
    if (cls == kEventClassApplication) {
        if (kind == kEventAppActivated) mac_input_focus(input, 1);
        else if (kind == kEventAppDeactivated) {
            mac_input_focus(input, 0);
            if (mac_input_focus_lost_hook) mac_input_focus_lost_hook();
        }
    } else if (cls == kEventClassWindow && kind == kEventWindowDeactivated) {
        mac_input_focus(input, 0);
        if (mac_input_focus_lost_hook) mac_input_focus_lost_hook();
    } else if (cls == kEventClassWindow && kind == kEventWindowActivated) {
        mac_input_focus(input, 1);
    } else if (cls == kEventClassKeyboard) {
        if (kind == kEventRawKeyModifiersChanged) {
            if (GetEventParameter(event, kEventParamKeyModifiers, typeUInt32,
                NULL, sizeof(modifiers), NULL, &modifiers) == noErr) {
                unsigned int native = 0;
                if (modifiers & shiftKey) native |= MAC_INPUT_MOD_SHIFT;
                if (modifiers & rightShiftKey) native |= MAC_INPUT_MOD_RIGHT_SHIFT;
                if (modifiers & controlKey) native |= MAC_INPUT_MOD_CONTROL;
                if (modifiers & rightControlKey) native |= MAC_INPUT_MOD_RIGHT_CONTROL;
                if (modifiers & optionKey) native |= MAC_INPUT_MOD_OPTION;
                if (modifiers & rightOptionKey) native |= MAC_INPUT_MOD_RIGHT_OPTION;
                if (modifiers & cmdKey) native |= MAC_INPUT_MOD_COMMAND;
                mac_input_modifiers(input, native);
            }
        } else if (GetEventParameter(event, kEventParamKeyCode, typeUInt32,
            NULL, sizeof(key), NULL, &key) == noErr) {
            if (kind == kEventRawKeyDown || kind == kEventRawKeyRepeat) {
                char character;
                mac_input_key(input, key, 1);
                if (GetEventParameter(event, kEventParamKeyMacCharCodes, typeChar,
                    NULL, sizeof(character), NULL, &character) == noErr)
                    mac_input_character(input,key,character);
            } else if (kind == kEventRawKeyUp) mac_input_key(input, key, 0);
        }
    } else if (cls == kEventClassMouse) {
        HIPoint location;WindowRef window;Rect bounds;
        if(GetEventParameter(event,kEventParamMouseLocation,typeHIPoint,NULL,sizeof(location),NULL,&location)==noErr&&
            GetEventParameter(event,kEventParamWindowRef,typeWindowRef,NULL,sizeof(window),NULL,&window)==noErr&&window&&
            GetWindowBounds(window,kWindowContentRgn,&bounds)==noErr) {
            unsigned int click_button=0;
            if(kind==kEventMouseDown&&GetEventParameter(event,kEventParamMouseButton,typeMouseButton,
                NULL,sizeof(button),NULL,&button)==noErr)click_button=(unsigned int)button;
            mac_input_pointer(input,location.x-(float)bounds.left,location.y-(float)bounds.top,
                kind==kEventMouseMoved||kind==kEventMouseDragged,click_button);
        }
        if (kind == kEventMouseDown || kind == kEventMouseUp) {
            if (GetEventParameter(event, kEventParamMouseButton, typeMouseButton,
                NULL, sizeof(button), NULL, &button) == noErr)
                mac_input_button(input, button, kind == kEventMouseDown);
        } else if (kind == kEventMouseMoved || kind == kEventMouseDragged) {
            if (GetEventParameter(event, kEventParamMouseDelta, typeHIPoint,
                NULL, sizeof(delta), NULL, &delta) == noErr)
                mac_input_motion(input, delta.x, delta.y);
        } else if (kind == kEventMouseWheelMoved) {
            if (GetEventParameter(event, kEventParamMouseWheelDelta, typeLongInteger,
                NULL, sizeof(wheel_delta), NULL, &wheel_delta) == noErr)
                mac_input_wheel(input, (float)wheel_delta);
        }
    }
    /* Let normal application/window handlers process the event too. */
    return eventNotHandledErr;
}
OSStatus mac_input_install(struct mac_input *input, EventHandlerRef *handler)
{
    static const EventTypeSpec events[] = {
        { kEventClassApplication, kEventAppActivated },
        { kEventClassApplication, kEventAppDeactivated },
        { kEventClassWindow, kEventWindowActivated },
        { kEventClassWindow, kEventWindowDeactivated },
        { kEventClassKeyboard, kEventRawKeyDown },
        { kEventClassKeyboard, kEventRawKeyUp },
        { kEventClassKeyboard, kEventRawKeyRepeat },
        { kEventClassKeyboard, kEventRawKeyModifiersChanged },
        { kEventClassMouse, kEventMouseDown },
        { kEventClassMouse, kEventMouseUp },
        { kEventClassMouse, kEventMouseMoved },
        { kEventClassMouse, kEventMouseDragged },
        { kEventClassMouse, kEventMouseWheelMoved }
    };
    /* Mach-O uses native function pointers for EventHandlerUPP. */
    return InstallApplicationEventHandler(mac_input_event,
        sizeof(events) / sizeof(events[0]), events, input, handler);
}
void mac_input_remove(EventHandlerRef handler)
{
    if (handler) RemoveEventHandler(handler);
}
#endif
