#include "nfd.h"

#include <stdio.h>
#include <stdlib.h>


/* this test should compile on all supported platforms */

int main( void )
{
    // initialize NFD (once per thread only)
    NFD_Init();

    nfdu8char_t *outPath = NULL;
    // 
    nfdu8filteritem_t filterItem[2] = { { "Source code", "c,cpp,cc" }, { "Headers", "h,hpp" } };
    nfdresult_t result = NFD_OpenDialogU8(&filterItem, 2, NULL, &outPath);
    if ( result == NFD_OKAY )
    {
        puts("Success!");
        puts(outPath);
        NFD_FreePathU8(outPath);
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
