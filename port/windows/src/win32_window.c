/*
WIN32_WINDOW.C

Keep borderless OpenGL windows on the desktop. NVIDIA's driver treats
SDL's WS_POPUP fullscreen window as exclusive even without a display mode,
which blanks the monitor when the player switches to another application.
*/

#include <windows.h>
#include <SDL3/SDL.h>

void win32_window_borderless_apply(SDL_Window *window)
{
	HWND handle;
	LONG_PTR exstyle;
	SDL_WindowFlags flags;

	if (!window)
		return;
	flags = SDL_GetWindowFlags(window);
	if (!(flags & SDL_WINDOW_FULLSCREEN) || SDL_GetWindowFullscreenMode(window))
		return;
	handle = SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
	if (!handle)
		return;
	exstyle = GetWindowLongPtr(handle, GWL_EXSTYLE);
	/* Drop topmost when unfocused so Alt+Tab can raise other windows over a
	borderless window without stripping WS_POPUP or sending SWP_FRAMECHANGED,
	which can stall Windows 11 DWM / WGL-on-DXGI presentation. */
	if (!(flags & SDL_WINDOW_INPUT_FOCUS) && (exstyle & WS_EX_TOPMOST))
	{
		SetWindowPos(handle, HWND_NOTOPMOST, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
	}
}
