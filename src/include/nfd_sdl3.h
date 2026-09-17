/*
  Native File Dialog Extended
  Repository: https://github.com/btzy/nativefiledialog-extended
  License: Zlib
  Authors: Bernard Teo

  This header contains a function to convert an SDL window handle to a native window handle for
  passing to NFDe.

  This is meant to be used with SDL3.
 */

#ifndef _NFD_SDL3_H
#define _NFD_SDL3_H

#include <SDL3/SDL.h>
#include <nfd.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#define NFD_INLINE inline
#else
#define NFD_INLINE static inline
#endif  // __cplusplus

/**
 * Sets the wayland display if the process is running under Wayland, otherwise does nothing.
 * @return Either true to indicate success, or false to indicate failure.  If false is returned,
 * you can call SDL_GetError() for more information.
 */
NFD_INLINE bool NFD_SetDisplayPropertiesFromSDL(void) {
#if !defined(SDL_PLATFORM_WINDOWS) && !defined(SDL_PLATFORM_APPLE)
    const char* const driver = SDL_GetCurrentVideoDriver();
    if (!driver) {
        SDL_SetError("The video subsystem has not been initialized.");
        return false;
    }
    if (SDL_strcmp(driver, "wayland") != 0) {
        return true;
    }
    struct wl_display* const display = (struct wl_display*)SDL_GetPointerProperty(
        SDL_GetGlobalProperties(), SDL_PROP_GLOBAL_VIDEO_WAYLAND_WL_DISPLAY_POINTER, NULL);
    if (!display) {
        SDL_SetError("Could not get the Wayland display from SDL.");
        return false;
    }
    return NFD_SetWaylandDisplay(display) == NFD_OKAY;
#else
    return true;
#endif
}

/**
 * Converts an SDL window handle to a native window handle that can be passed to NFDe.
 * @param sdlWindow The SDL window handle.
 * @param[out] nativeWindow The output native window handle, populated if and only if this function
 * returns true.
 * @return Either true to indicate success, or false to indicate failure.  If false is returned,
 * you can call SDL_GetError() for more information.  However, it is intended that users ignore the
 * error and simply pass a value-initialized nfdwindowhandle_t to NFDe if this function fails. */
NFD_INLINE bool NFD_GetNativeWindowFromSDLWindow(SDL_Window* sdlWindow,
                                                 nfdwindowhandle_t* nativeWindow) {
    // Get the properties container for this specific window
    const SDL_PropertiesID props = SDL_GetWindowProperties(sdlWindow);
    if (!props) {
        // SDL_GetWindowProperties() has already set the error message.
        return false;
    }

    const char* const driver = SDL_GetCurrentVideoDriver();
    if (!driver) {
        SDL_SetError("The video subsystem has not been initialized.");
        return false;
    }

    // Check the active driver and pull the corresponding native property
    size_t type;
    void* handle;
    if (SDL_strcmp(driver, "wayland") == 0) {
        type = NFD_WINDOW_HANDLE_TYPE_WAYLAND;
        handle = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL);
    } else if (SDL_strcmp(driver, "x11") == 0) {
        type = NFD_WINDOW_HANDLE_TYPE_X11;
        // X11 Window ID is a number in SDL3 (Uint64).
        // We need to cast it to void* for NFD's struct.
        handle =
            (void*)(uintptr_t)SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
    } else if (SDL_strcmp(driver, "windows") == 0) {
        type = NFD_WINDOW_HANDLE_TYPE_WINDOWS;
        handle = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    } else if (SDL_strcmp(driver, "cocoa") == 0) {
        type = NFD_WINDOW_HANDLE_TYPE_COCOA;
        handle = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL);
    } else {
        SDL_SetError("Unsupported video driver \"%s\".", driver);
        return false;
    }
    if (!handle) {
        SDL_SetError("Could not get the native window from SDL.");
        return false;
    }

    // Only write to the output parameter if we are going to return true.
    nativeWindow->type = type;
    nativeWindow->handle = handle;
    return true;
}

#undef NFD_INLINE
#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // _NFD_SDL3_H
