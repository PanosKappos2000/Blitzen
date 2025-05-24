#pragma once

#if defined(_WIN32)
#include <windows.h>
#elif defined(linux)
#include <X11/XKBlib.h>  // sudo apt-get install libx11-dev
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>  // sudo apt-get install libxkbcommon-x11-dev
#endif

namespace BlitzenPlatform
{
#if defined(_WIN32)

	struct PlatformContext
	{
		HWND m_hwnd;
		HINSTANCE m_hinstance;
		HGLRC m_hglrc;
	};
#elif defined(linux)
	struct PlatformContext
	{
		Display* m_pDisplay;
		xcb_connection_t* m_pConnection;
		xcb_window_t m_window;
		xcb_screen_t* m_pScreen;
		xcb_atom_t m_wmProtocols;
		xcb_atom_t m_wmDeleteWin;

		void* m_pEvents;
	};
#else
	static_assert(true, "Platform not supported");
#endif
}