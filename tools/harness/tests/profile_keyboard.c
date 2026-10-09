/* Real virtual keyboard exits and keyboard mapping over an in-memory name. */
#include "harness.h"
#include <wchar.h>
typedef unsigned short word;
typedef int BOOL;
typedef unsigned char BYTE;
typedef short SHORT;
#include "config.inc"

struct platform_input_state { unsigned char keys[TEST_SCANCODE_COUNT], mouse_buttons[8]; BOOL mouse_released, menus; };
#define SDL_SCANCODE_COUNT TEST_SCANCODE_COUNT
#define console_is_active() FALSE
#define platform_screen_keyboard(show, password) ((void)0)
typedef struct { unsigned short wButtons; BYTE bAnalogButtons[8]; SHORT sThumbLX, sThumbLY; } XINPUT_GAMEPAD;
#define SDL_BUTTON_X1 4
static BOOL text_typing, text_typing_enter_armed, text_typing_keyboard, text_typing_field;
static struct { boolean active, shift_active, caps_active, symbols_active;
    void *keyboard; short row, column; word buffer_size; short last_event, last_key,
    number_of_event_repeats, caption_index; boolean last_exit_saved_text, first_key_replaces_buffer;
    wchar_t *text_buffer, *cursor; unsigned long time_of_last_event;
    long caret_bitmap_index; wchar_t saved_text[MAXIMUM_VIRTUAL_KEYBOARD_SAVED_TEXT_LENGTH];
} virtual_keyboard_globals;
static boolean unique = TRUE, clean = TRUE, keyboard_available = TRUE;
static int processed, errors;
#define VIRTUAL_KEYBOARD_TAG 1
#define BITMAP_GROUP_TAG 2
#define tag_loaded(group, name) (keyboard_available ? 0 : NONE)
#define virtual_keyboard_definition_get(index) ((void *)1)
#define error(...) ((void)0)
#define _error_already_a_saved_game_file_with_that_name 1
#define _error_cannot_create_saved_game_file_with_empty_name 2
#define display_error(...) (++errors)
#define player_name_clean(buffer, count) (clean)
#define saved_game_file_name_unique(buffer) (unique)
#define event_manager_flush() ((void)0)
#define ui_play_audio_feedback_sound(sound) ((void)0)
#define system_milliseconds() 0UL
#define ustrlen wcslen
#define ustrncpy wcsncpy
#define ustrcmp wcscmp
#define ustrcpy wcscpy
#define csmemset memset
#define csmemmove memmove
#define MIN(a,b) ((a)<(b)?(a):(b))
#define virtual_keyboard_backspace() ((void)0)
#define virtual_keyboard_free_space_in_text_buffer() 0
#define virtual_keyboard_get_current_character() L'a'
static void virtual_keyboard_process_internal(void) { ++processed; }
static boolean virtual_keyboard_cancel(void);
#include "under_test.inc"

static void launch(wchar_t *name)
{
    CHECK(virtual_keyboard_initialize(), "keyboard initialization failed");
    CHECK(virtual_keyboard_launch(name, 12 * sizeof(wchar_t), 8), "keyboard launch failed");
    CHECK(text_typing, "launch did not immediately enable typing");
    /* The old code enabled typing only when the active keyboard processed. */
    virtual_keyboard_process();
    CHECK(text_typing && virtual_keyboard_globals.active, "keyboard did not own typing");
    struct platform_input_state input = {0}; XINPUT_GAMEPAD pad = {0};
    keyboard_gamepad(&input, &pad); /* let go of the Enter that opened it */
    input.keys[SDL_SCANCODE_RETURN] = 1;
    keyboard_gamepad(&input, &pad);
    CHECK(pad.wButtons & XINPUT_GAMEPAD_START, "typing Enter stopped choosing Done");
    CHECK(!pad.bAnalogButtons[XINPUT_GAMEPAD_A], "typing Enter selected a character");
}

static void ordinary_input(void)
{
    struct platform_input_state input = {0}; XINPUT_GAMEPAD pad = {0};
    CHECK(!virtual_keyboard_globals.active, "keyboard remained active");
    CHECK(!text_typing, "closed keyboard left typing mode enabled");
    input.keys[SDL_SCANCODE_RETURN] = 1;
    input.keys[SDL_SCANCODE_W] = 1;
    keyboard_gamepad(&input, &pad);
    CHECK(pad.bAnalogButtons[XINPUT_GAMEPAD_A] && !(pad.wButtons & XINPUT_GAMEPAD_START),
        "menu Enter was not A after closing keyboard");
    CHECK(pad.sThumbLY == 32767, "keyboard movement still suppressed");
    memset(&pad, 0, sizeof(pad)); input.keys[SDL_SCANCODE_RETURN] = 0;
    input.keys[SDL_SCANCODE_KP_ENTER] = 1;
    keyboard_gamepad(&input, &pad);
    CHECK(pad.bAnalogButtons[XINPUT_GAMEPAD_A] && !(pad.wButtons & XINPUT_GAMEPAD_START),
        "numpad Enter was not A");
}

int main(int argc, char **argv)
{
    const char *case_name = argc > 1 ? argv[1] : "";
    wchar_t name[12] = L"Probe";
    launch(name);
    CASE("done") { virtual_keyboard_select(); CHECK(virtual_keyboard_globals.last_exit_saved_text, "Done lost the name"); }
    else CASE("cancel") { wcscpy(name, L"Changed"); virtual_keyboard_cancel(); CHECK(!wcscmp(name,L"Probe"), "cancel did not restore name"); }
    else CASE("external-close") { virtual_keyboard_close(); }
    else CASE("dispose") { virtual_keyboard_dispose(); }
    else CASE("initialize") { virtual_keyboard_initialize(); }
    else CASE("invalid-name") { clean = FALSE; virtual_keyboard_select(); CHECK(errors==1 && !virtual_keyboard_globals.last_exit_saved_text, "invalid name accepted"); }
    else CASE("duplicate-name") { unique = FALSE; wcscpy(name,L"Other"); virtual_keyboard_select(); CHECK(errors==1 && !virtual_keyboard_globals.last_exit_saved_text, "duplicate name accepted"); }
    else CASE("field-owner")
    {
        platform_text_field(TRUE, FALSE); virtual_keyboard_select();
        CHECK(!virtual_keyboard_globals.active && text_typing, "closing keyboard cleared text field ownership");
        platform_text_field(FALSE, FALSE);
    }
    else CASE("reopen")
    {
        for (int i=0; i<20; ++i) { virtual_keyboard_select(); ordinary_input(); launch(name); }
        virtual_keyboard_cancel();
    }
    else CASE("launch-failure")
    {
        virtual_keyboard_dispose(); keyboard_available=FALSE; virtual_keyboard_initialize();
        CHECK(!virtual_keyboard_launch(name,sizeof(name),8), "missing keyboard launched");
    }
    else CASE("held-enter")
    {
        struct platform_input_state input = { .menus = TRUE }; XINPUT_GAMEPAD pad = {0};
        keys_held_over_switch(&input);
        input.keys[SDL_SCANCODE_RETURN] = 1;
        keys_held_over_switch(&input); keyboard_gamepad(&input,&pad);
        CHECK(pad.wButtons & XINPUT_GAMEPAD_START, "Enter did not accept keyboard");
        virtual_keyboard_select();
        memset(&pad,0,sizeof(pad));
        keys_held_over_switch(&input); keyboard_gamepad(&input,&pad);
        CHECK(!pad.wButtons && !pad.bAnalogButtons[XINPUT_GAMEPAD_A], "held Done also activated the next menu");
        input.keys[SDL_SCANCODE_RETURN] = 0; keys_held_over_switch(&input);
        input.keys[SDL_SCANCODE_RETURN] = 1; keys_held_over_switch(&input);
        keyboard_gamepad(&input,&pad);
        CHECK(pad.bAnalogButtons[XINPUT_GAMEPAD_A], "fresh Enter was still suppressed");
    }
    else CHECK(FALSE, "unknown case");
    ordinary_input();
    int before = processed; virtual_keyboard_process();
    CHECK(before == processed, "inactive keyboard processed input");
    return 0;
}
