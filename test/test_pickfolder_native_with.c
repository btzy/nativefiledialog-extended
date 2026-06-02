#define NFD_NATIVE
#include <nfd.h>

#include <stdio.h>
#include <stdlib.h>

/* this test should compile on all supported platforms */

int main(void) {
    // initialize NFD
    // either call NFD_Init at the start of your program and NFD_Quit at the end of your program,
    // or before/after every time you want to show a file dialog.
    NFD_Init();

    nfdchar_t* outPath;

    // customize the window title and button labels (leave any of these unset to use the OS default)
#ifdef _WIN32
    const nfdnchar_t* title = L"Choose a project folder";
    const nfdnchar_t* acceptLabel = L"Use this folder";
    const nfdnchar_t* cancelLabel = L"Never mind";
#else
    const nfdnchar_t* title = "Choose a project folder";
    const nfdnchar_t* acceptLabel = "Use this folder";
    const nfdnchar_t* cancelLabel = "Never mind";
#endif

    // show the dialog
    nfdpickfoldernargs_t args = {0};
    args.title = title;
    args.acceptLabel = acceptLabel;
    args.cancelLabel = cancelLabel;
    nfdresult_t result = NFD_PickFolderN_With(&outPath, &args);
    if (result == NFD_OKAY) {
        puts("Success!");
#ifdef _WIN32
#ifdef _MSC_VER
        _putws(outPath);
#else
        fputws(outPath, stdout);
#endif
#else
        puts(outPath);
#endif
        // remember to free the memory (since NFD_OKAY is returned)
        NFD_FreePath(outPath);
    } else if (result == NFD_CANCEL) {
        puts("User pressed cancel.");
    } else {
        printf("Error: %s\n", NFD_GetError());
    }

    // Quit NFD
    NFD_Quit();

    return 0;
}
