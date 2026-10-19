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
	LONG_PTR style;

	if (!(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) || SDL_GetWindowFullscreenMode(window))
		return;
	handle = SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
	if (!handle)
		return;
	style = GetWindowLongPtr(handle, GWL_STYLE);
	if (!(style & WS_POPUP) && !(GetWindowLongPtr(handle, GWL_EXSTYLE) & WS_EX_TOPMOST))
		return;
	SetWindowLongPtr(handle, GWL_STYLE, style & ~WS_POPUP);
	SetWindowPos(handle, HWND_NOTOPMOST, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}
