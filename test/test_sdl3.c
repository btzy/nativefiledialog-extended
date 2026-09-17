#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <nfd.h>
#include <nfd_sdl3.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Small program meant to demonstrate and test nfd_sdl3.h with SDL3.  Note that it quits immediately
// when it encounters an error, without calling the opposite destroy/quit function. A real-world
// application should call destroy/quit appropriately.
void show_error(const char* message, SDL_Window* window) {
    if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message, window)) {
        printf("SDL_ShowSimpleMessageBox failed: %s\n", SDL_GetError());
        return;
    }
}

void show_path(const char* path, SDL_Window* window) {
    if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Success", path, window)) {
        printf("SDL_ShowSimpleMessageBox failed: %s\n", SDL_GetError());
        return;
    }
}

void show_paths(const nfdpathset_t* paths, SDL_Window* window) {
    size_t num_chars = 0;

    nfdpathsetsize_t num_paths;
    if (NFD_PathSet_GetCount(paths, &num_paths) != NFD_OKAY) {
        printf("NFD_PathSet_GetCount failed: %s\n", NFD_GetError());
        return;
    }

    nfdpathsetsize_t i;
    for (i = 0; i != num_paths; ++i) {
        char* path;
        if (NFD_PathSet_GetPathU8(paths, i, &path) != NFD_OKAY) {
            printf("NFD_PathSet_GetPathU8 failed: %s\n", NFD_GetError());
            return;
        }
        num_chars += strlen(path) + 1;
        NFD_PathSet_FreePathU8(path);
    }

    // We should never return NFD_OKAY with zero paths, but GCC doesn't know this and will emit a
    // warning that we're trying to malloc with size zero if we write the following line.
    if (!num_paths) num_chars = 1;

    char* message = malloc(num_chars);
    message[0] = '\0';

    for (i = 0; i != num_paths; ++i) {
        if (i != 0) {
            strcat(message, "\n");
        }
        char* path;
        if (NFD_PathSet_GetPathU8(paths, i, &path) != NFD_OKAY) {
            printf("NFD_PathSet_GetPathU8 failed: %s\n", NFD_GetError());
            free(message);
            return;
        }
        strcat(message, path);
        NFD_PathSet_FreePathU8(path);
    }

    if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Success", message, window)) {
        printf("SDL_ShowSimpleMessageBox failed: %s\n", SDL_GetError());
        free(message);
        return;
    }

    free(message);
}

void set_native_window(SDL_Window* sdlWindow, nfdwindowhandle_t* nativeWindow) {
    if (!NFD_GetNativeWindowFromSDLWindow(sdlWindow, nativeWindow)) {
        printf("NFD_GetNativeWindowFromSDLWindow failed: %s\n", SDL_GetError());
    }
}

void opendialog_handler(SDL_Window* window) {
    char* path;
    nfdopendialogu8args_t args = {0};
    set_native_window(window, &args.parentWindow);
    const nfdresult_t res = NFD_OpenDialogU8_With(&path, &args);
    switch (res) {
        case NFD_OKAY:
            show_path(path, window);
            NFD_FreePathU8(path);
            break;
        case NFD_ERROR:
            show_error(NFD_GetError(), window);
            break;
        default:
            break;
    }
}

void opendialogmultiple_handler(SDL_Window* window) {
    const nfdpathset_t* paths;
    nfdopendialogu8args_t args = {0};
    set_native_window(window, &args.parentWindow);
    const nfdresult_t res = NFD_OpenDialogMultipleU8_With(&paths, &args);
    switch (res) {
        case NFD_OKAY:
            show_paths(paths, window);
            NFD_PathSet_Free(paths);
            break;
        case NFD_ERROR:
            show_error(NFD_GetError(), window);
            break;
        default:
            break;
    }
}

void savedialog_handler(SDL_Window* window) {
    char* path;
    nfdsavedialogu8args_t args = {0};
    set_native_window(window, &args.parentWindow);
    const nfdresult_t res = NFD_SaveDialogU8_With(&path, &args);
    switch (res) {
        case NFD_OKAY:
            show_path(path, window);
            NFD_FreePathU8(path);
            break;
        case NFD_ERROR:
            show_error(NFD_GetError(), window);
            break;
        default:
            break;
    }
}

void pickfolder_handler(SDL_Window* window) {
    char* path;
    nfdpickfolderu8args_t args = {0};
    set_native_window(window, &args.parentWindow);
    const nfdresult_t res = NFD_PickFolderU8_With(&path, &args);
    switch (res) {
        case NFD_OKAY:
            show_path(path, window);
            NFD_FreePathU8(path);
            break;
        case NFD_ERROR:
            show_error(NFD_GetError(), window);
            break;
        default:
            break;
    }
}

void pickfoldermultiple_handler(SDL_Window* window) {
    const nfdpathset_t* paths;
    nfdpickfolderu8args_t args = {0};
    set_native_window(window, &args.parentWindow);
    const nfdresult_t res = NFD_PickFolderMultipleU8_With(&paths, &args);
    switch (res) {
        case NFD_OKAY:
            show_paths(paths, window);
            NFD_PathSet_Free(paths);
            break;
        case NFD_ERROR:
            show_error(NFD_GetError(), window);
            break;
        default:
            break;
    }
}

#if defined(_WIN32)
const char* font_file[] = {"C:\\Windows\\Fonts\\calibri.ttf"};
#elif defined(__APPLE__)
const char* font_file[] = {"/System/Library/Fonts/SFNS.ttf"};
#else
const char* font_file[] = {
    "/usr/share/fonts/noto/NotoSans-Regular.ttf",           // Arch/OpenSUSE
    "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",  // Ubuntu/Debian
    "/usr/share/fonts/google-noto/NotoSans-Regular.ttf",    // Fedora

    // Fallback if noto fonts are not found
    "/usr/share/fonts/dejavu/DejaVuSans.ttf",             // Arch/OpenSUSE
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",    // Ubuntu/Debian
    "/usr/share/fonts/dejavu-sans-fonts/DejaVuSans.ttf",  // Fedora
};
#endif
const size_t num_font_files = sizeof(font_file) / sizeof(const char*);

#define NUM_STATES 3
#define NUM_BUTTONS 5
const char* button_text[NUM_BUTTONS] = {"Open File",
                                        "Open Files",
                                        "Save File",
                                        "Select Folder",
                                        "Select Folders"};
const int BUTTON_WIDTH = 400;
const int BUTTON_HEIGHT = 40;

void (*button_handler[NUM_BUTTONS])(SDL_Window*) = {&opendialog_handler,
                                                    &opendialogmultiple_handler,
                                                    &savedialog_handler,
                                                    &pickfolder_handler,
                                                    &pickfoldermultiple_handler};

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    // initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 0;
    }

    // initialize SDL_ttf
    if (!TTF_Init()) {
        printf("TTF_Init failed: %s\n", SDL_GetError());
        return 0;
    }

    // initialize NFD
    if (NFD_Init() != NFD_OKAY) {
        printf("NFD_Init failed: %s\n", NFD_GetError());
        return 0;
    }

    // create window
    SDL_Window* const window = SDL_CreateWindow(
        "Welcome", BUTTON_WIDTH, BUTTON_HEIGHT * NUM_BUTTONS, SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window) {
        printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
        return 0;
    }

    // this gives NFD the wl_display* on Wayland; this is needed to set the parent window
    if (!NFD_SetDisplayPropertiesFromSDL()) {
        printf("NFD_SetDisplayPropertiesFromSDL failed: %s\n", SDL_GetError());
    }

    float window_scale = SDL_GetWindowDisplayScale(window);
    window_scale = window_scale == 0.0f ? 1.0f : window_scale;

    // Create renderer
    SDL_Renderer* const renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        printf("SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return 0;
    }

    // Properly support HiDPI
    SDL_SetRenderLogicalPresentation(
        renderer, BUTTON_WIDTH, BUTTON_HEIGHT * NUM_BUTTONS, SDL_LOGICAL_PRESENTATION_STRETCH);

    // prepare the buttons and handlers
    SDL_Texture* textures_normal[NUM_BUTTONS][NUM_STATES];

    TTF_Font* font = NULL;
    for (size_t i = 0; i != num_font_files; ++i) {
        font = TTF_OpenFont(font_file[i], 20.0f * window_scale);
        if (font) break;
    }
    if (!font) {
        printf("TTF_OpenFont failed: %s\n", SDL_GetError());
        return 0;
    }

    const SDL_Color back_color[NUM_STATES] = {{0, 0, 0, SDL_ALPHA_OPAQUE},
                                              {51, 51, 51, SDL_ALPHA_OPAQUE},
                                              {102, 102, 102, SDL_ALPHA_OPAQUE}};
    const SDL_Color text_color = {255, 255, 255, SDL_ALPHA_OPAQUE};
    const uint8_t text_alpha[NUM_STATES] = {153, 204, 255};

    for (size_t i = 0; i != NUM_BUTTONS; ++i) {
        SDL_Surface* const text_surface =
            TTF_RenderText_Blended(font, button_text[i], 0, text_color);
        if (!text_surface) {
            printf("TTF_RenderUTF8_Blended failed: %s\n", SDL_GetError());
            return 0;
        }

        if (!SDL_SetSurfaceBlendMode(text_surface, SDL_BLENDMODE_BLEND)) {
            printf("SDL_SetSurfaceBlendMode failed: %s\n", SDL_GetError());
            return 0;
        }

        for (size_t j = 0; j != NUM_STATES; ++j) {
            SDL_Surface* button_surface = SDL_CreateSurface((int)(BUTTON_WIDTH * window_scale),
                                                            (int)(BUTTON_HEIGHT * window_scale),
                                                            SDL_PIXELFORMAT_RGBA32);
            if (!button_surface) {
                printf("SDL_CreateRGBSurface failed: %s\n", SDL_GetError());
                return 0;
            }

            if (!SDL_FillSurfaceRect(button_surface,
                                     NULL,
                                     SDL_MapRGBA(SDL_GetPixelFormatDetails(button_surface->format),
                                                 NULL,
                                                 back_color[j].r,
                                                 back_color[j].g,
                                                 back_color[j].b,
                                                 back_color[j].a))) {
                printf("SDL_FillRect failed: %s\n", SDL_GetError());
                return 0;
            }

            SDL_SetSurfaceAlphaMod(text_surface, text_alpha[j]);

            SDL_Rect dstrect = {((int)(BUTTON_WIDTH * window_scale) - text_surface->w) / 2,
                                ((int)(BUTTON_HEIGHT * window_scale) - text_surface->h) / 2,
                                text_surface->w,
                                text_surface->h};
            if (!SDL_BlitSurface(text_surface, NULL, button_surface, &dstrect)) {
                printf("SDL_BlitSurface failed: %s\n", SDL_GetError());
                return 0;
            }

            SDL_Texture* const texture = SDL_CreateTextureFromSurface(renderer, button_surface);
            if (!texture) {
                printf("SDL_CreateTextureFromSurface failed: %s\n", SDL_GetError());
                return 0;
            }

            SDL_DestroySurface(button_surface);

            textures_normal[i][j] = texture;
        }

        SDL_DestroySurface(text_surface);
    }

    TTF_CloseFont(font);

    // event loop
    bool quit = false;
    size_t button_index = (size_t)-1;
    bool pressed = false;
    do {
        // render
        for (size_t i = 0; i != NUM_BUTTONS; ++i) {
            const SDL_FRect rect = {
                0.0f, (float)i * BUTTON_HEIGHT, (float)BUTTON_WIDTH, (float)BUTTON_HEIGHT};
            int button_state = button_index == i ? (pressed ? 2 : 1) : 0;
            SDL_RenderTexture(renderer, textures_normal[i][button_state], NULL, &rect);
        }
        SDL_RenderPresent(renderer);

        // process events
        SDL_Event event;
        if (!SDL_WaitEvent(&event)) {
            printf("SDL_WaitEvent failed: %s\n", SDL_GetError());
            return 0;
        }
        do {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    quit = true;
                    break;

                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                    quit = true;
                    break;

                case SDL_EVENT_WINDOW_MOUSE_LEAVE:
                    button_index = (size_t)-1;
                    break;

                case SDL_EVENT_MOUSE_MOTION: {
                    if (event.motion.x < 0 || event.motion.x >= BUTTON_WIDTH ||
                        event.motion.y < 0) {
                        button_index = (size_t)-1;
                        break;
                    }
                    const int index = (int)(event.motion.y / BUTTON_HEIGHT);
                    if (index < 0 || index >= NUM_BUTTONS) {
                        button_index = (size_t)-1;
                        break;
                    }
                    button_index = (size_t)index;
                    pressed = (event.motion.state & SDL_BUTTON_LMASK) != 0;
                    break;
                }
                case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        pressed = true;
                    }
                    break;
                }
                case SDL_EVENT_MOUSE_BUTTON_UP: {
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        pressed = false;
                        if (button_index != (size_t)-1) {
                            (*button_handler[button_index])(window);
                        }
                    }
                    break;
                }
            }
        } while (SDL_PollEvent(&event));
    } while (!quit);

    // destroy textures
    for (size_t i = 0; i != NUM_BUTTONS; ++i) {
        for (size_t j = 0; j != NUM_STATES; ++j) {
            SDL_DestroyTexture(textures_normal[i][j]);
        }
    }

    // destroy renderer
    SDL_DestroyRenderer(renderer);

    // destroy window
    SDL_DestroyWindow(window);

    // quit NFD
    NFD_Quit();

    // quit SDL_ttf
    TTF_Quit();

    // quit SDL
    SDL_Quit();

    return 0;
}
