/*
MARKETING_AND_STRATEGIC_BUSINESS_DEVELOPMENT.C

symbols in this file:
000CF960 0060:
	_xbox_demos_available (0000)
000CF9C0 0010:
	_code_000cf9c0 (0000)
000CF9D0 0060:
	_code_000cf9d0 (0000)
000CFA30 0070:
	_xbox_demos_launch (0000)
000CFAA0 0080:
	_xbox_dashboard_launch (0000)
00270AD0 0015:
	??_C@_0BF@BDEFGBHJ@d?3?2XDemos?2XDemos?4xbe?$AA@ (0000)
00270AE8 0048:
	??_C@_0EI@IDHGDKAK@IDirect3DDevice8_PersistDisplay?$CI@ (0000)
00270B30 0048:
	??_C@_0EI@DFIBOGNG@c?3?2halo?2SOURCE?2interface?2marketi@ (0000)
00270B78 0020:
	??_C@_0CA@EKJELPEF@stopping?5map?5file?5precaching?4?4?4?$AA@ (0000)
00270B98 0023:
	??_C@_0CD@NDGKLBNL@preparing?5for?5XLaunchNewImage?$CI?$CJ?4@ (0000)
00270BBC 0012:
	??_C@_0BC@KCGCFOKM@XDEMOS?5not?5found?$CB?$AA@ (0000)
002FD5A0 0001:
	_data_002fd5a0 (0000)
00453CCC 0001:
	_bss_00453ccc (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "cseries_windows.h"
#include "marketing_and_strategic_business_development.h"
#include "errors.h"
#include "files.h"
#include "ui_widget.h"
#include "cache_files.h"
#include "saved_game_files.h"
#include "rasterizer/xbox/rasterizer_xbox.h"

/* ---------- prototypes */

static void clean_up_for_image_launch(void);

/* ---------- globals */

static boolean bss_00453ccc;
static boolean data_002fd5a0= TRUE;

/* ---------- public code */

boolean xbox_demos_available(
	void)
{
	struct file_reference file;
	
	if (data_002fd5a0==TRUE)
	{
		if (file_reference_create_from_path(&file, "d:\\XDemos\\XDemos.xbe", FALSE))
		{
			if (file_exists(&file))
			{
				bss_00453ccc= TRUE;
			}
		}

		data_002fd5a0= FALSE;
	}

	return bss_00453ccc;
}

void xbox_demos_launch(
	void)
{
	if (xbox_demos_available())
	{
		LAUNCH_DATA launch_data= {0};

		clean_up_for_image_launch();

		XLaunchNewImageA("d:\\XDemos\\XDemos.xbe", &launch_data);

		XLaunchNewImageA(NULL, &launch_data);

		while (TRUE)
		{
			;
		}
	}
	else
	{
		error(_error_silent, "XDEMOS not found!");
	}

	return;
}

void xbox_dashboard_launch(
	void)
{
	LAUNCH_DATA launch_data= {0};
	long abort_error;

	clean_up_for_image_launch();

	abort_error= dashboard_abort_error;
	switch (abort_error)
	{
	case 0x21:
	case 0x22:
	{
		((PLD_LAUNCH_DASHBOARD)&launch_data)->dwReason= XLD_LAUNCH_DASHBOARD_MEMORY;
		break;
	}
	case 0x23:
	{
		((PLD_LAUNCH_DASHBOARD)&launch_data)->dwReason= XLD_LAUNCH_DASHBOARD_MAIN_MENU;
		break;
	}
	}

	XLaunchNewImageA(NULL, &launch_data);

	while (TRUE)
	{
		;
	}

	return;
}

/* ---------- private code */

static void clean_up_for_image_launch(
	void)
{
	error(_error_silent, "preparing for XLaunchNewImage()...");
	saved_game_files_take_mutex();

	if (cache_files_precache_in_progress())
	{
		error(_error_silent, "stopping map file precaching...");
		cache_files_precache_map_end();
	}

	match_vassert("c:\\halo\\SOURCE\\interface\\marketing_and_strategic_business_development.c", 64,
		SUCCEEDED(IDirect3DDevice8_PersistDisplay(&global_d3d_device)),
		"IDirect3DDevice8_PersistDisplay() failed in clean_up_for_image_launch()");
}
