#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_ttf.h>
#include <nfd.h>
#include <nfd_sdl2.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Small program meant to demonstrate and test nfd_sdl2.h with SDL2.  Note that it quits immediately
// when it encounters an error, without calling the opposite destroy/quit function.  A real-world
// application should call destroy/quit appropriately.

void show_error(const char* message, SDL_Window* window) {
    if (SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message, window) != 0) {
        printf("SDL_ShowSimpleMessageBox failed: %s\n", SDL_GetError());
        return;
    }
}

void show_path(const char* path, SDL_Window* window) {
    if (SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Success", path, window) != 0) {
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

    if (SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Success", message, window) != 0) {
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
const char font_file[] = "C:\\Windows\\Fonts\\calibri.ttf";
#elif defined(__APPLE__)
const char font_file[] = "/System/Library/Fonts/SFNS.ttf";
#else
const char font_file[] = "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf";
#endif

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

#ifdef _WIN32
int WINAPI WinMain(void)
#else
int main(void)
#endif
{
#ifdef _WIN32
    // Enable DPI awareness on Windows
    SDL_SetHint("SDL_HINT_WINDOWS_DPI_AWARENESS", "permonitorv2");
    SDL_SetHint("SDL_HINT_WINDOWS_DPI_SCALING", "1");
#endif

    // initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 0;
    }

    // initialize SDL_ttf
    if (TTF_Init() != 0) {
        printf("TTF_Init failed: %s\n", TTF_GetError());
        return 0;
    }

    // initialize NFD
    if (NFD_Init() != NFD_OKAY) {
        printf("NFD_Init failed: %s\n", NFD_GetError());
        return 0;
    }

    // create window
    SDL_Window* const window = SDL_CreateWindow("Welcome",
                                                SDL_WINDOWPOS_UNDEFINED,
                                                SDL_WINDOWPOS_UNDEFINED,
                                                BUTTON_WIDTH,
                                                BUTTON_HEIGHT * NUM_BUTTONS,
                                                SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
        return 0;
    }

    // create renderer
    SDL_Renderer* const renderer =
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        printf("SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return 0;
    }

    // prepare the buttons and handlers
    SDL_Texture* textures_normal[NUM_BUTTONS][NUM_STATES];

    TTF_Font* const font = TTF_OpenFont(font_file, 20);
    if (!font) {
        printf("TTF_OpenFont failed: %s\n", TTF_GetError());
        return 0;
    }

    const SDL_Color back_color[NUM_STATES] = {{0, 0, 0, SDL_ALPHA_OPAQUE},
                                              {51, 51, 51, SDL_ALPHA_OPAQUE},
                                              {102, 102, 102, SDL_ALPHA_OPAQUE}};
    const SDL_Color text_color = {255, 255, 255, SDL_ALPHA_OPAQUE};
    const uint8_t text_alpha[NUM_STATES] = {153, 204, 255};

    for (size_t i = 0; i != NUM_BUTTONS; ++i) {
        SDL_Surface* const text_surface = TTF_RenderUTF8_Blended(font, button_text[i], text_color);
        if (!text_surface) {
            printf("TTF_RenderUTF8_Blended failed: %s\n", TTF_GetError());
            return 0;
        }

        if (SDL_SetSurfaceBlendMode(text_surface, SDL_BLENDMODE_BLEND) != 0) {
            printf("SDL_SetSurfaceBlendMode failed: %s\n", SDL_GetError());
            return 0;
        }

        for (size_t j = 0; j != NUM_STATES; ++j) {
            SDL_Surface* button_surface =
                SDL_CreateRGBSurface(0, BUTTON_WIDTH, BUTTON_HEIGHT, 32, 0, 0, 0, 0);
            if (!button_surface) {
                printf("SDL_CreateRGBSurface failed: %s\n", SDL_GetError());
                return 0;
            }

            if (SDL_FillRect(button_surface,
                             NULL,
                             SDL_MapRGBA(button_surface->format,
                                         back_color[j].r,
                                         back_color[j].g,
                                         back_color[j].b,
                                         back_color[j].a)) != 0) {
                printf("SDL_FillRect failed: %s\n", SDL_GetError());
                return 0;
            }

            SDL_SetSurfaceAlphaMod(text_surface, text_alpha[j]);

            SDL_Rect dstrect = {(BUTTON_WIDTH - text_surface->w) / 2,
                                (BUTTON_HEIGHT - text_surface->h) / 2,
                                text_surface->w,
                                text_surface->h};
            if (SDL_BlitSurface(text_surface, NULL, button_surface, &dstrect) != 0) {
                printf("SDL_BlitSurface failed: %s\n", SDL_GetError());
                return 0;
            }

            SDL_Texture* const texture = SDL_CreateTextureFromSurface(renderer, button_surface);
            if (!texture) {
                printf("SDL_CreateTextureFromSurface failed: %s\n", SDL_GetError());
                return 0;
            }

            SDL_FreeSurface(button_surface);

            textures_normal[i][j] = texture;
        }

        SDL_FreeSurface(text_surface);
    }

    TTF_CloseFont(font);

    // event loop
    bool quit = false;
    size_t button_index = (size_t)-1;
    bool pressed = false;
    do {
        // render
        for (size_t i = 0; i != NUM_BUTTONS; ++i) {
            const SDL_Rect rect = {0, (int)i * BUTTON_HEIGHT, BUTTON_WIDTH, BUTTON_HEIGHT};
            SDL_RenderCopy(
                renderer, textures_normal[i][button_index == i ? pressed ? 2 : 1 : 0], NULL, &rect);
        }
        SDL_RenderPresent(renderer);

        // process events
        SDL_Event event;
        if (SDL_WaitEvent(&event) == 0) {
            printf("SDL_WaitEvent failed: %s\n", SDL_GetError());
            return 0;
        }
        do {
            switch (event.type) {
                case SDL_QUIT: {
                    quit = true;
                    break;
                }
                case SDL_WINDOWEVENT: {
                    switch (event.window.event) {
                        case SDL_WINDOWEVENT_CLOSE:
                            quit = true;
                            break;
                        case SDL_WINDOWEVENT_LEAVE:
                            button_index = (size_t)-1;
                            break;
                    }
                    break;
                }
                case SDL_MOUSEMOTION: {
                    if (event.motion.x < 0 || event.motion.x >= BUTTON_WIDTH ||
                        event.motion.y < 0) {
                        button_index = (size_t)-1;
                        break;
                    }
                    const int index = event.motion.y / BUTTON_HEIGHT;
                    if (index < 0 || index >= NUM_BUTTONS) {
                        button_index = (size_t)-1;
                        break;
                    }
                    button_index = index;
                    pressed = event.motion.state & SDL_BUTTON(1);
                    break;
                }
                case SDL_MOUSEBUTTONDOWN: {
                    if (event.button.button == 1) {
                        pressed = true;
                    }
                    break;
                }
                case SDL_MOUSEBUTTONUP: {
                    if (event.button.button == 1) {
                        pressed = false;
                        if (button_index != (size_t)-1) {
                            (*button_handler[button_index])(window);
                        }
                    }
                    break;
                }
            }
        } while (SDL_PollEvent(&event) != 0);
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
