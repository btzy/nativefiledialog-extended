#include <GLFW/glfw3.h>
#include <nfd.h>
#include <nfd_glfw3.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

// Small program meant to demonstrate and test nfd_glfw3.h with GLFW.  Note that it quits
// immediately when it encounters an error, without calling the opposite destroy/quit function.  A
// real-world application should call destroy/quit appropriately.
//
// Left-click to show the open file dialog
// Middle-click to show the pick folder dialog
// Right-click to show the save file dialog

void error_callback(int error_code, const char* description) {
    printf("GLFW call failed (code %d): %s\n", error_code, description);
}

void set_native_window(GLFWwindow* glfwWindow, nfdwindowhandle_t* nativeWindow) {
    if (!NFD_GetNativeWindowFromGLFWWindow(glfwWindow, nativeWindow)) {
        printf("NFD_GetNativeWindowFromGLFWWindow failed\n");
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    (void)mods;
    if (action != GLFW_PRESS) return;
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT: {
            char* path;
            nfdopendialogu8args_t args = {0};
            set_native_window(window, &args.parentWindow);
            const nfdresult_t res = NFD_OpenDialogU8_With(&path, &args);
            switch (res) {
                case NFD_OKAY:
                    printf("NFD_OpenDialogU8_With success: %s\n", path);
                    NFD_FreePathU8(path);
                    break;
                case NFD_CANCEL:
                    printf("NFD_OpenDialogU8_With cancelled\n");
                    break;
                case NFD_ERROR:
                    printf("NFD_OpenDialogU8_With error: %s\n", NFD_GetError());
                    break;
                default:
                    break;
            }
            break;
        }
        case GLFW_MOUSE_BUTTON_MIDDLE: {
            char* path;
            nfdpickfolderu8args_t args = {0};
            set_native_window(window, &args.parentWindow);
            const nfdresult_t res = NFD_PickFolderU8_With(&path, &args);
            switch (res) {
                case NFD_OKAY:
                    printf("NFD_PickFolderU8_With success: %s\n", path);
                    NFD_FreePathU8(path);
                    break;
                case NFD_CANCEL:
                    printf("NFD_PickFolderU8_With cancelled\n");
                    break;
                case NFD_ERROR:
                    printf("NFD_PickFolderU8_With error: %s\n", NFD_GetError());
                    break;
                default:
                    break;
            }
            break;
        }
        case GLFW_MOUSE_BUTTON_RIGHT: {
            char* path;
            nfdsavedialogu8args_t args = {0};
            set_native_window(window, &args.parentWindow);
            const nfdresult_t res = NFD_SaveDialogU8_With(&path, &args);
            switch (res) {
                case NFD_OKAY:
                    printf("NFD_SaveDialogU8_With success: %s\n", path);
                    NFD_FreePathU8(path);
                    break;
                case NFD_CANCEL:
                    printf("NFD_SaveDialogU8_With cancelled\n");
                    break;
                case NFD_ERROR:
                    printf("NFD_SaveDialogU8_With error: %s\n", NFD_GetError());
                    break;
                default:
                    break;
            }
            break;
        }
    }
}

#ifdef _WIN32
int __stdcall WinMain(void)
#else
int main(void)
#endif
{
    // initialize GLFW
    if (!glfwInit()) {
        return 0;
    }

    // initialize NFD
    if (NFD_Init() != NFD_OKAY) {
        printf("NFD_Init failed: %s\n", NFD_GetError());
        return 0;
    }

    // this gives NFD the wl_display* on Wayland; this is needed to set the parent window
    if (!NFD_SetDisplayPropertiesFromGLFW()) {
        printf("NFD_SetDisplayPropertiesFromGLFW failed\n");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // create window
    GLFWwindow* window = glfwCreateWindow(640, 480, "OpenGL Triangle", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return 0;
    }

    // add a callback when the mouse button is pressed
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    // paint the window black
    glfwMakeContextCurrent(window);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    glfwSwapBuffers(window);

    while (!glfwWindowShouldClose(window)) {
        glfwWaitEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}
