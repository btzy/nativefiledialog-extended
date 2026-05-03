/*
  Native File Dialog Extended
  Repository: https://github.com/btzy/nativefiledialog-extended
  License: Zlib
  Authors: Bernard Teo

  These are shared functions for Linux (GTK and Portal).
*/

#include <assert.h>
#include <stdlib.h>

#include "nfd.h"

#ifdef NFD_WAYLAND
#include <wayland-client.h>
#include "xdg-foreign-unstable-v1.h"
#endif

namespace {

template <typename T = void>
T* NFDi_Malloc(size_t bytes) {
    void* ptr = malloc(bytes);
    assert(ptr);  // Linux malloc never fails

    return static_cast<T*>(ptr);
}

template <typename T>
void NFDi_Free(T* ptr) {
    assert(ptr);
    free(static_cast<void*>(ptr));
}

template <typename T>
struct Free_Guard {
    T* data;
    Free_Guard(T* freeable) noexcept : data(freeable) {}
    ~Free_Guard() { NFDi_Free(data); }
};

template <typename T>
struct FreeCheck_Guard {
    T* data;
    FreeCheck_Guard(T* freeable = nullptr) noexcept : data(freeable) {}
    ~FreeCheck_Guard() {
        if (data) NFDi_Free(data);
    }
};

template <typename T>
T* copy(const T* begin, const T* end, T* out) {
    for (; begin != end; ++begin) {
        *out++ = *begin;
    }
    return out;
}

#ifndef NFD_CASE_SENSITIVE_FILTER
nfdnchar_t* emit_case_insensitive_glob(const nfdnchar_t* begin,
                                       const nfdnchar_t* end,
                                       nfdnchar_t* out) {
    // this code will only make regular Latin characters case-insensitive; other
    // characters remain case sensitive
    for (; begin != end; ++begin) {
        if ((*begin >= 'A' && *begin <= 'Z') || (*begin >= 'a' && *begin <= 'z')) {
            *out++ = '[';
            *out++ = *begin;
            // invert the case of the original character
            *out++ = *begin ^ static_cast<nfdnchar_t>(0x20);
            *out++ = ']';
        } else {
            *out++ = *begin;
        }
    }
    return out;
}
#endif

#ifdef NFD_WAYLAND
struct wl_display* wayland_display;
struct wl_registry* wayland_registry;
uint32_t wayland_xdg_exporter_v1_name;
struct zxdg_exporter_v1* wayland_xdg_exporter_v1;
constexpr const char* XDG_EXPORTER_V1 = "zxdg_exporter_v1";
#endif

void EmptyFn(void*) {}

struct DestroyFunc {
    DestroyFunc() : fn(&EmptyFn), context(nullptr) {}
    ~DestroyFunc() { (*fn)(context); }
    void (*fn)(void*);
    void* context;
};

#ifdef NFD_WAYLAND
void registry_handle_global(void* context,
                            struct wl_registry* registry,
                            uint32_t name,
                            const char* interface,
                            uint32_t version) {
    (void)context;
    (void)version;
    if (strcmp(interface, XDG_EXPORTER_V1) == 0) {
        wayland_xdg_exporter_v1_name = name;
        wayland_xdg_exporter_v1 = static_cast<struct zxdg_exporter_v1*>(wl_registry_bind(
            registry, name, &zxdg_exporter_v1_interface, zxdg_exporter_v1_interface.version));
    }
}

void registry_handle_global_remove(void* context, struct wl_registry* registry, uint32_t name) {
    (void)context;
    (void)registry;
    if (wayland_xdg_exporter_v1 && name == wayland_xdg_exporter_v1_name) {
        zxdg_exporter_v1_destroy(wayland_xdg_exporter_v1);
        wayland_xdg_exporter_v1 = nullptr;
    }
}

constexpr struct wl_registry_listener wayland_registry_listener = {&registry_handle_global,
                                                                   &registry_handle_global_remove};

void NFD_Wayland_Init(void) {
    wayland_display = nullptr;
}

void NFD_Wayland_Quit(void) {
    if (wayland_display) {
        if (wayland_xdg_exporter_v1) zxdg_exporter_v1_destroy(wayland_xdg_exporter_v1);
        wl_registry_destroy(wayland_registry);
    }
}

#endif

}  // namespace

nfdresult_t NFD_SetWaylandDisplay(wl_display* display) {
#ifdef NFD_WAYLAND
    NFD_Wayland_Quit();
    wayland_display = display;
    if (wayland_display) {
        wayland_registry = wl_display_get_registry(wayland_display);
        wayland_xdg_exporter_v1 = nullptr;
        // seems like registry can't be null
        wl_registry_add_listener(wayland_registry, &wayland_registry_listener, nullptr);
        wl_display_roundtrip(wayland_display);
    }
#else
    (void)display;
#endif
    return NFD_OKAY;
}
