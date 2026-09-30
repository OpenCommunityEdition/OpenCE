/*
TOUCH_MENU_TEST.C

Tests of the menus' touch gestures (port/linux/src/touch_menu.c). Built and
run by tools/test_touch_menu.py; exits nonzero on a failure.
*/

#include "../src/touch_menu.h"

#include <stdio.h>

static int failures;

#define CHECK(condition) \
	do \
	{ \
		if (!(condition)) \
		{ \
			printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
			failures++; \
		} \
	} while (0)

/* 3 pixels a dp, as on the S23: slop 12 dp, step 40 dp; 2 steps a read, to
test the limit (the game uses 1) */
static void setup(struct touch_menu *menu)
{
	struct touch_menu_settings settings = { 36.0f, 120.0f, 2 };

	touch_menu_init(menu, &settings);
}

/* first, so that a unit that lost its settings (a step of 0 never ends its
loops) fails here and not by hanging a later test */
static void test_init_stores_the_settings(void)
{
	struct touch_menu menu;

	setup(&menu);
	CHECK(menu.settings.slop == 36.0f);
	CHECK(menu.settings.step == 120.0f);
	CHECK(menu.settings.steps_per_read == 2);
}

static void test_tap_clicks_where_the_finger_went_down(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;

	setup(&menu);
	touch_menu_down(&menu, 7, 500.0f, 300.0f);
	touch_menu_move(&menu, 7, 510.0f, 305.0f);
	touch_menu_up(&menu, 7, 512.0f, 306.0f);
	touch_menu_read(&menu, &out);
	CHECK(out.moved == 1);
	CHECK(out.clicks == 1);
	CHECK(out.x == 500.0f && out.y == 300.0f);
	CHECK(out.click_x == 500.0f && out.click_y == 300.0f);
	CHECK(out.wheel_steps == 0);
}

static void test_read_clears_the_tap(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;

	setup(&menu);
	touch_menu_down(&menu, 1, 10.0f, 10.0f);
	touch_menu_up(&menu, 1, 10.0f, 10.0f);
	touch_menu_read(&menu, &out);
	touch_menu_read(&menu, &out);
	CHECK(out.moved == 0);
	CHECK(out.clicks == 0);
}

static void test_nothing_until_the_finger_lifts(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;

	setup(&menu);
	touch_menu_down(&menu, 1, 10.0f, 10.0f);
	touch_menu_read(&menu, &out);
	CHECK(out.moved == 0);
	CHECK(out.clicks == 0);
}

static void test_a_tap_may_move_the_whole_slop(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;

	setup(&menu);
	touch_menu_down(&menu, 1, 100.0f, 100.0f);
	touch_menu_up(&menu, 1, 136.0f, 100.0f);
	touch_menu_read(&menu, &out);
	CHECK(out.clicks == 1);
}

static void test_two_taps_between_reads(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;

	setup(&menu);
	touch_menu_down(&menu, 1, 10.0f, 10.0f);
	touch_menu_up(&menu, 1, 10.0f, 10.0f);
	touch_menu_down(&menu, 2, 70.0f, 80.0f);
	touch_menu_up(&menu, 2, 70.0f, 80.0f);
	touch_menu_read(&menu, &out);
	CHECK(out.clicks == 2);
	CHECK(out.click_x == 70.0f && out.click_y == 80.0f);
}

static void test_drag_down_scrolls_back_without_clicking(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;

	setup(&menu);
	touch_menu_down(&menu, 1, 500.0f, 100.0f);
	/* past the slop (counted from 136), then 250 further: 2 steps */
	touch_menu_move(&menu, 1, 500.0f, 140.0f);
	touch_menu_move(&menu, 1, 500.0f, 390.0f);
	touch_menu_up(&menu, 1, 500.0f, 390.0f);
	touch_menu_read(&menu, &out);
	CHECK(out.wheel_steps == 2);
	CHECK(out.clicks == 0);
	CHECK(out.moved == 0);
}

static void test_drag_up_scrolls_forward(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;

	setup(&menu);
	touch_menu_down(&menu, 1, 500.0f, 800.0f);
	touch_menu_move(&menu, 1, 500.0f, 760.0f);
	touch_menu_move(&menu, 1, 500.0f, 630.0f);
	touch_menu_read(&menu, &out);
	CHECK(out.wheel_steps == -1);
}

static void test_horizontal_drag_uses_the_x_axis(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;

	setup(&menu);
	touch_menu_down(&menu, 1, 1500.0f, 500.0f);
	/* mostly left: forward */
	touch_menu_move(&menu, 1, 1460.0f, 510.0f);
	touch_menu_move(&menu, 1, 1330.0f, 540.0f);
	touch_menu_read(&menu, &out);
	CHECK(out.wheel_steps == -1);
}

static void test_equal_dx_dy_scrolls_vertically(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;

	setup(&menu);
	touch_menu_down(&menu, 1, 500.0f, 500.0f);
	touch_menu_move(&menu, 1, 530.0f, 530.0f);
	touch_menu_move(&menu, 1, 530.0f, 680.0f);
	touch_menu_read(&menu, &out);
	CHECK(out.wheel_steps == 1);
}

static void test_the_axis_stays_after_the_start(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;

	setup(&menu);
	touch_menu_down(&menu, 1, 500.0f, 500.0f);
	touch_menu_move(&menu, 1, 500.0f, 540.0f);
	touch_menu_move(&menu, 1, 900.0f, 540.0f);
	touch_menu_read(&menu, &out);
	CHECK(out.wheel_steps == 0);
}

static void test_a_fast_drag_counts_from_the_slop(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;
	int total = 0, index;

	setup(&menu);
	touch_menu_down(&menu, 1, 500.0f, 0.0f);
	/* one sample far past the slop, as a flick or a slow frame gives */
	touch_menu_move(&menu, 1, 500.0f, 600.0f);
	touch_menu_up(&menu, 1, 500.0f, 600.0f);
	for (index = 0; index < 5; index++)
	{
		touch_menu_read(&menu, &out);
		total += out.wheel_steps;
	}
	/* (600 - 36) / 120 = 4.7 */
	CHECK(total == 4);
	CHECK(out.clicks == 0);
}

static void test_steps_come_out_gradually(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;

	setup(&menu);
	touch_menu_down(&menu, 1, 500.0f, 0.0f);
	touch_menu_move(&menu, 1, 500.0f, 40.0f);
	/* five steps at once */
	touch_menu_move(&menu, 1, 500.0f, 640.0f);
	touch_menu_read(&menu, &out);
	CHECK(out.wheel_steps == 2);
	touch_menu_read(&menu, &out);
	CHECK(out.wheel_steps == 2);
	touch_menu_read(&menu, &out);
	CHECK(out.wheel_steps == 1);
	touch_menu_read(&menu, &out);
	CHECK(out.wheel_steps == 0);
}

static void test_forward_steps_come_out_gradually(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;

	setup(&menu);
	touch_menu_down(&menu, 1, 500.0f, 1000.0f);
	touch_menu_move(&menu, 1, 500.0f, 960.0f);
	touch_menu_move(&menu, 1, 500.0f, 360.0f);
	touch_menu_read(&menu, &out);
	CHECK(out.wheel_steps == -2);
	touch_menu_read(&menu, &out);
	CHECK(out.wheel_steps == -2);
	touch_menu_read(&menu, &out);
	CHECK(out.wheel_steps == -1);
	touch_menu_read(&menu, &out);
	CHECK(out.wheel_steps == 0);
}

static void test_part_of_a_step_is_kept(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;

	setup(&menu);
	touch_menu_down(&menu, 1, 500.0f, 0.0f);
	touch_menu_move(&menu, 1, 500.0f, 40.0f);
	touch_menu_move(&menu, 1, 500.0f, 100.0f);
	touch_menu_read(&menu, &out);
	CHECK(out.wheel_steps == 0);
	touch_menu_move(&menu, 1, 500.0f, 160.0f);
	touch_menu_read(&menu, &out);
	CHECK(out.wheel_steps == 1);
}

static void test_lift_far_away_is_no_tap(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;

	setup(&menu);
	touch_menu_down(&menu, 1, 100.0f, 100.0f);
	touch_menu_up(&menu, 1, 100.0f, 600.0f);
	touch_menu_read(&menu, &out);
	CHECK(out.clicks == 0);
	CHECK(out.moved == 0);
	/* the lift's own move scrolls: (600 - 100 - 36) / 120 = 3.9 steps, 2 a read */
	CHECK(out.wheel_steps == 2);
}

static void test_a_scroll_keeps_the_last_tap_point_without_moving(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;

	setup(&menu);
	touch_menu_down(&menu, 1, 10.0f, 20.0f);
	touch_menu_up(&menu, 1, 10.0f, 20.0f);
	touch_menu_read(&menu, &out);
	touch_menu_down(&menu, 2, 500.0f, 0.0f);
	touch_menu_move(&menu, 2, 500.0f, 40.0f);
	touch_menu_move(&menu, 2, 500.0f, 200.0f);
	touch_menu_up(&menu, 2, 500.0f, 200.0f);
	touch_menu_read(&menu, &out);
	CHECK(out.moved == 0);
	CHECK(out.x == 10.0f && out.y == 20.0f);
}

static void test_a_new_finger_stops_pending_steps(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;

	setup(&menu);
	touch_menu_down(&menu, 1, 500.0f, 0.0f);
	touch_menu_move(&menu, 1, 500.0f, 40.0f);
	touch_menu_move(&menu, 1, 500.0f, 640.0f);
	touch_menu_up(&menu, 1, 500.0f, 640.0f);
	/* the tap hits the list as the finger saw it */
	touch_menu_down(&menu, 2, 300.0f, 300.0f);
	touch_menu_up(&menu, 2, 300.0f, 300.0f);
	touch_menu_read(&menu, &out);
	CHECK(out.clicks == 1);
	CHECK(out.wheel_steps == 0);
}

static void test_second_finger_is_ignored(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;

	setup(&menu);
	touch_menu_down(&menu, 1, 100.0f, 100.0f);
	touch_menu_down(&menu, 2, 900.0f, 900.0f);
	touch_menu_move(&menu, 2, 900.0f, 100.0f);
	touch_menu_up(&menu, 2, 900.0f, 100.0f);
	touch_menu_up(&menu, 1, 100.0f, 100.0f);
	touch_menu_read(&menu, &out);
	CHECK(out.clicks == 1);
	CHECK(out.x == 100.0f && out.y == 100.0f);
	CHECK(out.wheel_steps == 0);
}

static void test_lift_of_another_finger_is_ignored(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;

	setup(&menu);
	touch_menu_down(&menu, 1, 100.0f, 100.0f);
	touch_menu_up(&menu, 9, 100.0f, 100.0f);
	touch_menu_read(&menu, &out);
	CHECK(out.clicks == 0 && out.moved == 0);
	touch_menu_up(&menu, 1, 100.0f, 100.0f);
	touch_menu_read(&menu, &out);
	CHECK(out.clicks == 1);
}

static void test_move_and_lift_without_down_do_nothing(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;

	setup(&menu);
	touch_menu_move(&menu, 1, 100.0f, 900.0f);
	touch_menu_up(&menu, 1, 100.0f, 900.0f);
	touch_menu_read(&menu, &out);
	CHECK(out.clicks == 0 && out.moved == 0 && out.wheel_steps == 0);
}

static void test_cancel_drops_the_gesture(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;

	setup(&menu);
	touch_menu_down(&menu, 1, 500.0f, 0.0f);
	touch_menu_move(&menu, 1, 500.0f, 40.0f);
	touch_menu_move(&menu, 1, 500.0f, 400.0f);
	touch_menu_cancel(&menu);
	touch_menu_up(&menu, 1, 500.0f, 400.0f);
	touch_menu_read(&menu, &out);
	CHECK(out.wheel_steps == 0);
	CHECK(out.clicks == 0);
	/* and the next finger works */
	touch_menu_down(&menu, 3, 20.0f, 20.0f);
	touch_menu_up(&menu, 3, 20.0f, 20.0f);
	touch_menu_read(&menu, &out);
	CHECK(out.clicks == 1);
}

static void test_reset_clears_everything(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;

	setup(&menu);
	touch_menu_down(&menu, 1, 10.0f, 10.0f);
	touch_menu_up(&menu, 1, 10.0f, 10.0f);
	touch_menu_reset(&menu);
	touch_menu_read(&menu, &out);
	CHECK(out.clicks == 0);
	CHECK(out.moved == 0);
}

static void test_reset_during_a_touch_ignores_its_lift(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;

	setup(&menu);
	touch_menu_down(&menu, 1, 100.0f, 100.0f);
	touch_menu_reset(&menu);
	touch_menu_move(&menu, 1, 100.0f, 500.0f);
	touch_menu_up(&menu, 1, 100.0f, 500.0f);
	touch_menu_read(&menu, &out);
	CHECK(out.clicks == 0 && out.wheel_steps == 0);
}

/* a step is exactly the step setting: 120 pixels, not 119.9 or 120.1 */
static void test_a_step_is_exactly_the_step_setting(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;

	setup(&menu);
	touch_menu_down(&menu, 1, 500.0f, 0.0f);
	touch_menu_move(&menu, 1, 500.0f, 40.0f);
	touch_menu_move(&menu, 1, 500.0f, 156.0f);
	touch_menu_read(&menu, &out);
	CHECK(out.wheel_steps == 1);
	touch_menu_cancel(&menu);
	touch_menu_down(&menu, 2, 500.0f, 1000.0f);
	touch_menu_move(&menu, 2, 500.0f, 960.0f);
	touch_menu_move(&menu, 2, 500.0f, 844.0f);
	touch_menu_read(&menu, &out);
	CHECK(out.wheel_steps == -1);
}

static void test_horizontal_drag_right_scrolls_back(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;

	setup(&menu);
	touch_menu_down(&menu, 1, 100.0f, 500.0f);
	/* no vertical movement at all: the distance is all in x */
	touch_menu_move(&menu, 1, 140.0f, 500.0f);
	touch_menu_move(&menu, 1, 180.0f, 500.0f);
	touch_menu_move(&menu, 1, 245.0f, 500.0f);
	touch_menu_read(&menu, &out);
	/* 245 - 136 = 109 of the 120 */
	CHECK(out.wheel_steps == 0);
	touch_menu_move(&menu, 1, 257.0f, 500.0f);
	touch_menu_read(&menu, &out);
	CHECK(out.wheel_steps == 1);
	CHECK(out.clicks == 0);
}

static void test_a_new_finger_starts_a_fresh_step(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;

	setup(&menu);
	touch_menu_down(&menu, 1, 500.0f, 0.0f);
	touch_menu_move(&menu, 1, 500.0f, 40.0f);
	touch_menu_move(&menu, 1, 500.0f, 100.0f);
	touch_menu_up(&menu, 1, 500.0f, 100.0f);
	/* the same 64 pixels again: 128 together would be a step */
	touch_menu_down(&menu, 2, 500.0f, 0.0f);
	touch_menu_move(&menu, 2, 500.0f, 40.0f);
	touch_menu_move(&menu, 2, 500.0f, 100.0f);
	touch_menu_read(&menu, &out);
	CHECK(out.wheel_steps == 0);
}

static void test_a_lifted_finger_cannot_move_again(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;

	setup(&menu);
	touch_menu_down(&menu, 1, 100.0f, 100.0f);
	touch_menu_up(&menu, 1, 100.0f, 100.0f);
	touch_menu_read(&menu, &out);
	/* the same id, no finger down */
	touch_menu_move(&menu, 1, 100.0f, 900.0f);
	touch_menu_read(&menu, &out);
	CHECK(out.wheel_steps == 0);
}

static void test_reset_keeps_the_settings(void)
{
	struct touch_menu menu;

	setup(&menu);
	touch_menu_reset(&menu);
	CHECK(menu.settings.slop == 36.0f);
	CHECK(menu.settings.step == 120.0f);
	CHECK(menu.settings.steps_per_read == 2);
}

static void test_cancel_keeps_a_finished_tap(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;

	setup(&menu);
	touch_menu_down(&menu, 1, 30.0f, 40.0f);
	touch_menu_up(&menu, 1, 30.0f, 40.0f);
	touch_menu_cancel(&menu);
	touch_menu_read(&menu, &out);
	CHECK(out.clicks == 1);
	CHECK(out.moved == 1);
	CHECK(out.click_x == 30.0f && out.click_y == 40.0f);
}

/* the game's value: one step a read */
static void test_one_step_a_read(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;
	struct touch_menu_settings settings = { 36.0f, 120.0f, 1 };
	int index;

	touch_menu_init(&menu, &settings);
	touch_menu_down(&menu, 1, 500.0f, 0.0f);
	touch_menu_move(&menu, 1, 500.0f, 40.0f);
	touch_menu_move(&menu, 1, 500.0f, 640.0f);
	for (index = 0; index < 5; index++)
	{
		touch_menu_read(&menu, &out);
		CHECK(out.wheel_steps == 1);
	}
	touch_menu_read(&menu, &out);
	CHECK(out.wheel_steps == 0);
	touch_menu_cancel(&menu);
	touch_menu_down(&menu, 2, 500.0f, 1000.0f);
	touch_menu_move(&menu, 2, 500.0f, 960.0f);
	touch_menu_move(&menu, 2, 500.0f, 360.0f);
	for (index = 0; index < 5; index++)
	{
		touch_menu_read(&menu, &out);
		CHECK(out.wheel_steps == -1);
	}
	touch_menu_read(&menu, &out);
	CHECK(out.wheel_steps == 0);
}

/* the debug view (debug.touch_targets) shows where a drag began, which no
tap or hover reports */
static void test_a_drag_reports_where_the_finger_went_down(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;

	setup(&menu);
	touch_menu_down(&menu, 3, 400.0f, 200.0f);
	touch_menu_move(&menu, 3, 400.0f, 500.0f);
	touch_menu_read(&menu, &out);
	CHECK(out.downs == 1);
	CHECK(out.down_x == 400.0f && out.down_y == 200.0f);
	CHECK(out.clicks == 0);
	touch_menu_read(&menu, &out);
	CHECK(out.downs == 0);
}

static void test_a_tap_reports_its_down(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;

	setup(&menu);
	touch_menu_down(&menu, 1, 50.0f, 60.0f);
	touch_menu_up(&menu, 1, 50.0f, 60.0f);
	touch_menu_read(&menu, &out);
	CHECK(out.downs == 1);
	CHECK(out.down_x == 50.0f && out.down_y == 60.0f);
}

static void test_two_downs_between_reads_count_twice(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;

	setup(&menu);
	touch_menu_down(&menu, 1, 50.0f, 60.0f);
	touch_menu_up(&menu, 1, 50.0f, 60.0f);
	touch_menu_down(&menu, 2, 70.0f, 80.0f);
	touch_menu_read(&menu, &out);
	CHECK(out.downs == 2);
	CHECK(out.down_x == 70.0f && out.down_y == 80.0f);
}

/* an ignored second finger is not a down */
static void test_a_second_finger_is_no_down(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;

	setup(&menu);
	touch_menu_down(&menu, 1, 50.0f, 60.0f);
	touch_menu_down(&menu, 2, 70.0f, 80.0f);
	touch_menu_read(&menu, &out);
	CHECK(out.downs == 1);
	CHECK(out.down_x == 50.0f && out.down_y == 60.0f);
}

static void test_reset_and_cancel_drop_the_unread_downs(void)
{
	struct touch_menu menu;
	struct touch_menu_output out;

	setup(&menu);
	touch_menu_down(&menu, 1, 50.0f, 60.0f);
	touch_menu_reset(&menu);
	touch_menu_read(&menu, &out);
	CHECK(out.downs == 0);
	touch_menu_down(&menu, 1, 50.0f, 60.0f);
	touch_menu_cancel(&menu);
	touch_menu_read(&menu, &out);
	CHECK(out.downs == 0);
}

int main(void)
{
	test_init_stores_the_settings();
	if (failures)
		return 1;
	test_tap_clicks_where_the_finger_went_down();
	test_read_clears_the_tap();
	test_nothing_until_the_finger_lifts();
	test_a_tap_may_move_the_whole_slop();
	test_two_taps_between_reads();
	test_drag_down_scrolls_back_without_clicking();
	test_drag_up_scrolls_forward();
	test_horizontal_drag_uses_the_x_axis();
	test_equal_dx_dy_scrolls_vertically();
	test_the_axis_stays_after_the_start();
	test_a_fast_drag_counts_from_the_slop();
	test_steps_come_out_gradually();
	test_forward_steps_come_out_gradually();
	test_part_of_a_step_is_kept();
	test_lift_far_away_is_no_tap();
	test_a_scroll_keeps_the_last_tap_point_without_moving();
	test_a_new_finger_stops_pending_steps();
	test_second_finger_is_ignored();
	test_lift_of_another_finger_is_ignored();
	test_move_and_lift_without_down_do_nothing();
	test_cancel_drops_the_gesture();
	test_reset_clears_everything();
	test_reset_during_a_touch_ignores_its_lift();
	test_a_step_is_exactly_the_step_setting();
	test_horizontal_drag_right_scrolls_back();
	test_a_new_finger_starts_a_fresh_step();
	test_a_lifted_finger_cannot_move_again();
	test_reset_keeps_the_settings();
	test_cancel_keeps_a_finished_tap();
	test_one_step_a_read();
	test_a_drag_reports_where_the_finger_went_down();
	test_a_tap_reports_its_down();
	test_two_downs_between_reads_count_twice();
	test_a_second_finger_is_no_down();
	test_reset_and_cancel_drop_the_unread_downs();
	if (failures)
		printf("%d failures\n", failures);
	else
		printf("all tests passed\n");
	return failures != 0;
}
