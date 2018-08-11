/*
  Native File Dialog

  http://www.frogtoss.com/labs
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "nfd.h"



/* public routines */



size_t NFD_PathSet_GetCount( const nfdpathset_t *pathset )
{
    assert(pathset);
    return pathset->count;
}

nfdchar_t *NFD_PathSet_GetPath( const nfdpathset_t *pathset, size_t num )
{
    assert(pathset);
    assert(num < pathset->count);
    
    return pathset->buf + pathset->indices[num];
}

void NFD_PathSet_Free( nfdpathset_t *pathset )
{
    assert(pathset);
    NFDi_Free( pathset->indices );
    NFDi_Free( pathset->buf );
}

/* internal routines */



