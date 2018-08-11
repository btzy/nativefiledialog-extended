/*
  Native File Dialog

  User API

  http://www.frogtoss.com/labs
 */


#ifndef _NFD_H
#define _NFD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#ifdef _WIN32
/* denotes UTF-16 char */
typedef wchar_t nfdchar_t;
#else
/* denotes UTF-8 char */
typedef char nfdchar_t;
#endif

/* opaque data structure -- see NFD_PathSet_* */
typedef void nfdpathset_t;

typedef enum {
    NFD_ERROR,       /* programmatic error */
    NFD_OKAY,        /* user pressed okay, or successful return */
    NFD_CANCEL       /* user pressed cancel */
}nfdresult_t;
    

typedef struct {
    const nfdchar_t *name;
    const nfdchar_t *spec;
} nfdfilteritem_t;


/* nfd_<targetplatform>.c */

/* free a file path that was returned */
void NFD_FreePath(nfdchar_t *outPath);

/* initialize NFD - call this for every thread that might use NFD, before calling any other NFD functions on that thread */
nfdresult_t NFD_Init(void);

/* call this to de-initialize NFD, if NFD_Init returned NFD_OKAY */
void NFD_Quit(void);

/* single file open dialog */
/* It is the caller's responsibility to free `outPath` via NFD_FreePath() if this function returns NFD_OKAY */
nfdresult_t NFD_OpenDialog( const nfdfilteritem_t *filterList,
                            size_t count,
                            const nfdchar_t *defaultPath,
                            nfdchar_t **outPath );

/* multiple file open dialog */    
nfdresult_t NFD_OpenDialogMultiple( const nfdfilteritem_t *filterList,
                                    size_t count,
                                    const nfdchar_t *defaultPath,
                                    nfdpathset_t **outPaths );

/* save dialog */
/* It is the caller's responsibility to free `outPath` via NFD_FreePath() if this function returns NFD_OKAY */
nfdresult_t NFD_SaveDialog( const nfdfilteritem_t *filterList,
                            size_t count,
                            const nfdchar_t *defaultPath,
                            nfdchar_t **outPath );


/* select folder dialog */
nfdresult_t NFD_PickFolder( const nfdchar_t *defaultPath,
                            nfdchar_t **outPath );


/* get last error -- set when nfdresult_t returns NFD_ERROR */
const char *NFD_GetError(void);


/* path set operations */
typedef unsigned long nfd_pathsetsize_t;

/* get the number of entries stored in pathSet */
/* note that some might be invalid (NFD_ERROR will be returned by NFD_PathSet_GetPath) */
nfdresult_t NFD_PathSet_GetCount( const nfdpathset_t *pathSet, nfd_pathsetsize_t* count );
/* Get the UTF-8 path at offset index */
/* It is the caller's responsibility to free `outPath` via NFD_FreePath() if this function returns NFD_OKAY */
nfdresult_t NFD_PathSet_GetPath( const nfdpathset_t *pathSet, nfd_pathsetsize_t index, nfdchar_t **outPath );
/* Free the pathSet */    
void        NFD_PathSet_Free( nfdpathset_t *pathSet );


#ifdef __cplusplus
}
#endif

#endif
