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

    // prepare filters for the dialog
#ifdef _WIN32
    nfdfilteritem_t filterItem[2] = {{L"Source code", L"c,cpp,cc"}, {L"Headers", L"h,hpp"}};
#else
    nfdfilteritem_t filterItem[2] = {{"Source code", "c,cpp,cc"}, {"Headers", "h,hpp"}};
#endif

    // customize the window title and button labels (leave any of these unset to use the OS default)
#ifdef _WIN32
    const nfdnchar_t* title = L"Open a source file";
    const nfdnchar_t* acceptLabel = L"Open it";
    const nfdnchar_t* cancelLabel = L"Never mind";
#else
    const nfdnchar_t* title = "Open a source file";
    const nfdnchar_t* acceptLabel = "Open it";
    const nfdnchar_t* cancelLabel = "Never mind";
#endif

    // show the dialog
    nfdopendialognargs_t args = {0};
    args.filterList = filterItem;
    args.filterCount = 2;
    args.title = title;
    args.acceptLabel = acceptLabel;
    args.cancelLabel = cancelLabel;
    nfdresult_t result = NFD_OpenDialogN_With(&outPath, &args);
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
