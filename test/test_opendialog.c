#include "nfd.h"

#include <stdio.h>
#include <stdlib.h>


/* this test should compile on all supported platforms */

int main( void )
{
    // initialize NFD (once per thread only)
    NFD_Init();

    nfdchar_t *outPath = NULL;
    // 
    nfdfilteritem_t filterItem[2] = { { L"Source code", L"c,cpp,cc" }, { L"Headers", L"h,hpp" } };
    nfdresult_t result = NFD_OpenDialog(&filterItem, 2, NULL, &outPath);
    if ( result == NFD_OKAY )
    {
        puts("Success!");
        fputws(outPath, stdout);
        NFD_FreePath(outPath);
    }
    else if ( result == NFD_CANCEL )
    {
        puts("User pressed cancel.");
    }
    else 
    {
        printf("Error: %s\n", NFD_GetError() );
    }

    // Quit NFD
    NFD_Quit();

    return 0;
}
