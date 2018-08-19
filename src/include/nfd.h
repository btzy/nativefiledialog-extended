/*
  Native File Dialog Extended
  Author: Bernard Teo
  License : Zlib
  Repository: https://github.com/btzy/nativefiledialog-extended
  Based on https://github.com/mlabbe/nativefiledialog

  This header contains the functions that can be called by user code.
 */


#ifndef _NFD_H
#define _NFD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#ifdef _WIN32
/* denotes UTF-16 char */
typedef wchar_t nfdnchar_t;
#else
/* denotes UTF-8 char */
typedef char nfdnchar_t;
#endif

/* opaque data structure -- see NFD_PathSet_* */
typedef void nfdpathset_t;

typedef unsigned int nfdfiltersize_t;

typedef enum {
    NFD_ERROR,       /* programmatic error */
    NFD_OKAY,        /* user pressed okay, or successful return */
    NFD_CANCEL       /* user pressed cancel */
}nfdresult_t;
    

typedef struct {
    const nfdnchar_t *name;
    const nfdnchar_t *spec;
} nfdnfilteritem_t;


/* nfd_<targetplatform>.c */

/* free a file path that was returned by the dialogs */
/* Note: use NFD_PathSet_FreePath to free path from pathset instead of this function */
void NFD_FreePathN(nfdnchar_t *filePath);

/* initialize NFD - call this for every thread that might use NFD, before calling any other NFD functions on that thread */
nfdresult_t NFD_Init(void);

/* call this to de-initialize NFD, if NFD_Init returned NFD_OKAY */
void NFD_Quit(void);

/* single file open dialog */
/* It is the caller's responsibility to free `outPath` via NFD_FreePathN() if this function returns NFD_OKAY */
/* If filterCount is zero, filterList is ignored (you can use NULL) */
/* If defaultPath is NULL, the operating system will decide */
nfdresult_t NFD_OpenDialogN( nfdnchar_t **outPath,
                             const nfdnfilteritem_t *filterList,
                             nfdfiltersize_t filterCount,
                             const nfdnchar_t *defaultPath );

/* multiple file open dialog */    
/* It is the caller's responsibility to free `outPaths` via NFD_PathSet_Free() if this function returns NFD_OKAY */
/* If filterCount is zero, filterList is ignored (you can use NULL) */
/* If defaultPath is NULL, the operating system will decide */
nfdresult_t NFD_OpenDialogMultipleN( const nfdpathset_t **outPaths,
                                     const nfdnfilteritem_t *filterList,
                                     nfdfiltersize_t filterCount,
                                     const nfdnchar_t *defaultPath );

/* save dialog */
/* It is the caller's responsibility to free `outPath` via NFD_FreePathN() if this function returns NFD_OKAY */
/* If filterCount is zero, filterList is ignored (you can use NULL) */
/* If defaultPath is NULL, the operating system will decide */
nfdresult_t NFD_SaveDialogN( nfdnchar_t **outPath,
                             const nfdnfilteritem_t *filterList,
                             nfdfiltersize_t filterCount,
                             const nfdnchar_t *defaultPath,
                             const nfdnchar_t *defaultName );


/* select folder dialog */
/* It is the caller's responsibility to free `outPath` via NFD_FreePathN() if this function returns NFD_OKAY */
/* If defaultPath is NULL, the operating system will decide */
nfdresult_t NFD_PickFolderN( nfdnchar_t **outPath,
                             const nfdnchar_t *defaultPath );


/* Get last error -- set when nfdresult_t returns NFD_ERROR */
/* Returns the last error that was set, or NULL if there is no error. */
/* The memory is owned by NFD and should not be freed by user code. */
/* This is *always* ASCII printable characters, so it can be interpreted as UTF-8 without any conversion. */
const char *NFD_GetError(void);
/* clear the error */
void NFD_ClearError(void);


/* path set operations */
#ifdef _WIN32
typedef unsigned long nfdpathsetsize_t;
#elif __APPLE__
typedef unsigned long nfdpathsetsize_t;
#else
typedef unsigned int nfdpathsetsize_t;
#endif

/* get the number of entries stored in pathSet */
/* note that some paths might be invalid (NFD_ERROR will be returned by NFD_PathSet_GetPath), so we might not actually have this number of usable paths */
nfdresult_t NFD_PathSet_GetCount( const nfdpathset_t *pathSet, nfdpathsetsize_t* count );
/* Get the UTF-8 path at offset index */
/* It is the caller's responsibility to free `outPath` via NFD_PathSet_FreePathN() if this function returns NFD_OKAY */
nfdresult_t NFD_PathSet_GetPathN( const nfdpathset_t *pathSet, nfdpathsetsize_t index, nfdnchar_t **outPath );
/* Free the path gotten by NFD_PathSet_GetPathN */
#ifdef _WIN32
#define NFD_PathSet_FreePathN NFD_FreePathN
#elif __APPLE__
#define NFD_PathSet_FreePathN NFD_FreePathN
#else
void        NFD_PathSet_FreePathN( const nfdnchar_t *filePath);
#endif
/* Free the pathSet */    
void        NFD_PathSet_Free( const nfdpathset_t *pathSet );


#ifdef _WIN32

/* say that the U8 versions of functions are not just #defined to be the native versions */
#define NFD_DIFFERENT_NATIVE_FUNCTIONS

typedef char nfdu8char_t;

typedef struct {
    const nfdu8char_t *name;
    const nfdu8char_t *spec;
} nfdu8filteritem_t;

/* UTF-8 compatibility functions */

/* free a file path that was returned */
void NFD_FreePathU8(nfdu8char_t *outPath);


/* single file open dialog */
/* It is the caller's responsibility to free `outPath` via NFD_FreePathU8() if this function returns NFD_OKAY */
nfdresult_t NFD_OpenDialogU8( nfdu8char_t **outPath,
                              const nfdu8filteritem_t *filterList,
                              nfdfiltersize_t count,
                              const nfdu8char_t *defaultPath );

/* multiple file open dialog */    
/* It is the caller's responsibility to free `outPaths` via NFD_PathSet_Free() if this function returns NFD_OKAY */
nfdresult_t NFD_OpenDialogMultipleU8( const nfdpathset_t **outPaths,
                                      const nfdu8filteritem_t *filterList,
                                      nfdfiltersize_t count,
                                      const nfdu8char_t *defaultPath );

/* save dialog */
/* It is the caller's responsibility to free `outPath` via NFD_FreePathU8() if this function returns NFD_OKAY */
nfdresult_t NFD_SaveDialogU8( nfdu8char_t **outPath,
                              const nfdu8filteritem_t *filterList,
                              nfdfiltersize_t count,
                              const nfdu8char_t *defaultPath,
                              const nfdu8char_t *defaultName );


/* select folder dialog */
/* It is the caller's responsibility to free `outPath` via NFD_FreePathU8() if this function returns NFD_OKAY */
nfdresult_t NFD_PickFolderU8( nfdu8char_t **outPath,
                              const nfdu8char_t *defaultPath );

/* Get the UTF-8 path at offset index */
/* It is the caller's responsibility to free `outPath` via NFD_FreePathU8() if this function returns NFD_OKAY */
nfdresult_t NFD_PathSet_GetPathU8(const nfdpathset_t *pathSet, nfdpathsetsize_t index, nfdu8char_t **outPath);

#define NFD_PathSet_FreePathU8 NFD_FreePathU8

#ifdef NFD_NATIVE
typedef nfdnchar_t nfdchar_t;
typedef nfdnfilteritem_t nfdfilteritem_t;
#define NFD_FreePath NFD_FreePathN
#define NFD_OpenDialog NFD_OpenDialogN
#define NFD_OpenDialogMultiple NFD_OpenDialogMultipleN
#define NFD_SaveDialog NFD_SaveDialogN
#define NFD_PickFolder NFD_PickFolderN
#define NFD_PathSet_GetPath NFD_PathSet_GetPathN
#define NFD_PathSet_FreePath NFD_PathSet_FreePathN
#else
typedef nfdu8char_t nfdchar_t;
typedef nfdu8filteritem_t nfdfilteritem_t;
#define NFD_FreePath NFD_FreePathU8
#define NFD_OpenDialog NFD_OpenDialogU8
#define NFD_OpenDialogMultiple NFD_OpenDialogMultipleU8
#define NFD_SaveDialog NFD_SaveDialogU8
#define NFD_PickFolder NFD_PickFolderU8
#define NFD_PathSet_GetPath NFD_PathSet_GetPathU8
#define NFD_PathSet_FreePath NFD_PathSet_FreePathU8
#endif

#else

/* the native charset is already UTF-8 */
typedef nfdnchar_t nfdchar_t;
typedef nfdnfilteritem_t nfdfilteritem_t;
#define NFD_FreePath NFD_FreePathN
#define NFD_OpenDialog NFD_OpenDialogN
#define NFD_OpenDialogMultiple NFD_OpenDialogMultipleN
#define NFD_SaveDialog NFD_SaveDialogN
#define NFD_PickFolder NFD_PickFolderN
#define NFD_PathSet_GetPath NFD_PathSet_GetPathN
#define NFD_PathSet_FreePath NFD_PathSet_FreePathN
typedef nfdnchar_t nfdu8char_t;
typedef nfdnfilteritem_t nfdu8filteritem_t;
#define NFD_FreePathU8 NFD_FreePathN
#define NFD_OpenDialogU8 NFD_OpenDialogN
#define NFD_OpenDialogMultipleU8 NFD_OpenDialogMultipleN
#define NFD_SaveDialogU8 NFD_SaveDialogN
#define NFD_PickFolderU8 NFD_PickFolderN
#define NFD_PathSet_GetPathU8 NFD_PathSet_GetPathN
#define NFD_PathSet_FreePathU8 NFD_PathSet_FreePathN

#endif // _WIN32s


#ifdef __cplusplus
}
#endif

#endif
