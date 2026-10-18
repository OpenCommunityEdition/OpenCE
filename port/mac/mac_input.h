#ifndef HALO_MAC_INPUT_H
#define HALO_MAC_INPUT_H

/* Tiger/Leopard main-thread input. Snapshot consumes edges and motion, not
   held keys. This adapter does not install engine hooks or capture the cursor. */
enum mac_input_action {
    MAC_INPUT_JUMP = 1, MAC_INPUT_CROUCH = 2, MAC_INPUT_ACTION = 4,
    MAC_INPUT_FIRE = 8, MAC_INPUT_GRENADE = 16, MAC_INPUT_PAUSE = 32
};
enum mac_input_modifier {
    MAC_INPUT_MOD_SHIFT = 1U, MAC_INPUT_MOD_RIGHT_SHIFT = 2U,
    MAC_INPUT_MOD_CONTROL = 4U, MAC_INPUT_MOD_RIGHT_CONTROL = 8U,
    MAC_INPUT_MOD_OPTION = 16U, MAC_INPUT_MOD_RIGHT_OPTION = 32U,
    MAC_INPUT_MOD_COMMAND = 64U
};
struct mac_input_frame {
    float forward, left, yaw, pitch;
    unsigned int held, pressed, released;
    unsigned int mouse_buttons;
    float wheel;
    float pointer_x,pointer_y,click_x,click_y;
    unsigned char pointer_valid,pointer_moved,left_clicks,right_clicks;
    unsigned char keys[128];
    unsigned char text_count;
    struct mac_input_text_event { unsigned char key; char character; } text[32];
};
struct mac_input {
    unsigned char keys[128];
    unsigned int buttons, held, pressed, released;
    int focused;
    /* port: nonzero while the window holds the mouse (mac_window.c): the
       motion then comes from mac_input_raw_motion, not the events */
    int raw_motion;
    float dx, dy, wheel;
    float pointer_x,pointer_y,click_x,click_y;
    unsigned char pointer_valid,pointer_moved,left_clicks,right_clicks;
    unsigned char text_count;
    struct mac_input_text_event text[32];
};
void mac_input_init(struct mac_input *input);
void mac_input_focus(struct mac_input *input, int focused);
void mac_input_key(struct mac_input *input, unsigned int key, int down);
void mac_input_character(struct mac_input *input, unsigned int key, char character);
void mac_input_modifiers(struct mac_input *input, unsigned int modifiers);
void mac_input_button(struct mac_input *input, unsigned int button, int down);
void mac_input_motion(struct mac_input *input, float dx, float dy);
void mac_input_wheel(struct mac_input *input, float delta);
/* port: motion read from the hardware while the mouse is held; the events' own
   motion is ignored then (mac_input.raw_motion) */
void mac_input_raw_motion(struct mac_input *input, float dx, float dy);
void mac_input_pointer(struct mac_input *input,float x,float y,int moved,unsigned int button_down);
void mac_input_snapshot(struct mac_input *input, struct mac_input_frame *frame);
/* Returns zero when no native gamepad/haptic device is available. */
int mac_input_rumble(unsigned int gamepad_index, unsigned short left, unsigned short right);

#if defined(__APPLE__) && !defined(__LP64__) && !defined(MAC_INPUT_NO_CARBON)
#include <Carbon/Carbon.h>
/* The owner pumps Carbon events on the same thread as snapshot. The event
   callback never creates windows, grabs devices, or changes cursor visibility. */
/* port: told at once when the application or the window loses the keys, so
   that the window can give the mouse back (mac_window.c) */
extern void (*mac_input_focus_lost_hook)(void);
OSStatus mac_input_event(EventHandlerCallRef next, EventRef event, void *context);
OSStatus mac_input_install(struct mac_input *input, EventHandlerRef *handler);
void mac_input_remove(EventHandlerRef handler);
#endif
#endif
