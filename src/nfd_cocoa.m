/*
  Native File Dialog

  http://www.frogtoss.com/labs
 */

#include <AppKit/AppKit.h>
#include "nfd.h"

static const char* g_errorstr = NULL;

static void NFDi_SetError(const char *msg)
{
    g_errorstr = msg;
}

static void *NFDi_Malloc(size_t bytes)
{
    void *ptr = malloc(bytes);
    if (!ptr)
        NFDi_SetError("NFDi_Malloc failed.");
    
    return ptr;
}

static void NFDi_Free(void *ptr)
{
    assert(ptr);
    free(ptr);
}

static NSArray *BuildAllowedFileTypes( const nfdnfilteritem_t *filterList, nfdfiltersize_t filterCount )
{
    // Commas and semicolons are the same thing on this platform

    NSMutableArray *buildFilterList = [[NSMutableArray alloc] init];
    
    
    for (nfdfiltersize_t filterIndex = 0; filterIndex != filterCount ; ++filterIndex)
    {
        // this is the spec to parse (we don't use the friendly name on OS X)
        const nfdnchar_t *filterSpec = filterList[filterIndex].spec;
        
        const nfdnchar_t *p_currentFilterBegin = filterSpec;
        for (const nfdnchar_t *p_filterSpec = filterSpec; *p_filterSpec; ++p_filterSpec)
        {
            if (*p_filterSpec == ',') {
                // add the extension to the array
                NSString *filterStr = [[[NSString alloc] initWithBytes:(const void*)p_currentFilterBegin length:(sizeof(nfdnchar_t) * (p_filterSpec - p_currentFilterBegin)) encoding: NSUTF8StringEncoding] autorelease];
                [buildFilterList addObject:filterStr];
                p_currentFilterBegin = p_filterSpec+1;
            }
        }
        // add the extension to the array
        NSString *filterStr = [NSString stringWithUTF8String:p_currentFilterBegin];
        [buildFilterList addObject:filterStr];
    }

    NSArray *returnArray = [NSArray arrayWithArray:buildFilterList];

    [buildFilterList release];
    
    assert([returnArray count] != 0);
    
    return returnArray;
}

static void AddFilterListToDialog( NSSavePanel *dialog, const nfdnfilteritem_t *filterList, nfdfiltersize_t filterCount )
{
    // note: NSOpenPanel inherits from NSSavePanel.
    
    if ( !filterCount ) return;
    
    assert(filterList);

    // make NSArray of file types
    NSArray *allowedFileTypes = BuildAllowedFileTypes( filterList, filterCount );
    
    // set it on the dialog
    [dialog setAllowedFileTypes:allowedFileTypes];
}

static void SetDefaultPath( NSSavePanel *dialog, const nfdnchar_t *defaultPath )
{
    if ( !defaultPath || !*defaultPath ) return;

    NSString *defaultPathString = [NSString stringWithUTF8String:defaultPath];
    NSURL *url = [NSURL fileURLWithPath:defaultPathString isDirectory:YES];
    [dialog setDirectoryURL:url];    
}

static void SetDefaultName( NSSavePanel *dialog, const nfdnchar_t *defaultName )
{
    if ( !defaultName || !*defaultName ) return;
    
    NSString *defaultNameString = [NSString stringWithUTF8String:defaultName];
    [dialog setNameFieldStringValue:defaultNameString];
}


/* public */


const char *NFD_GetError(void)
{
    return g_errorstr;
}

void NFD_FreePathN(nfdnchar_t *filePath)
{
    NFDi_Free((void*)filePath);
}

nfdresult_t NFD_Init(void) {
    return NFD_OKAY;
}

/* call this to de-initialize NFD, if NFD_Init returned NFD_OKAY */
void NFD_Quit(void) {}



nfdresult_t NFD_OpenDialogN( nfdnchar_t **outPath,
                             const nfdnfilteritem_t *filterList,
                             nfdfiltersize_t filterCount,
                             const nfdnchar_t *defaultPath )
{
    nfdresult_t result = NFD_CANCEL;
    @autoreleasepool {
        NSWindow *keyWindow = [[NSApplication sharedApplication] keyWindow];

        NSOpenPanel *dialog = [NSOpenPanel openPanel];
        [dialog setAllowsMultipleSelection:NO];

        // Build the filter list
        AddFilterListToDialog(dialog, filterList, filterCount);

        // Set the starting directory
        SetDefaultPath(dialog, defaultPath);

        if ( [dialog runModal] == NSModalResponseOK )
        {
            const NSURL *url = [dialog URL];
            const char *utf8Path = [[url path] UTF8String];

            // byte count, not char count
            size_t len = strlen(utf8Path);

            // too bad we have to use additional memory for this
            *outPath = (nfdchar_t*)NFDi_Malloc( len+1 );
            if (*outPath) {
                strcpy(*outPath, utf8Path);
                result = NFD_OKAY;
            }
        }
        
        // return focus to the key window (i.e. main window)
        [keyWindow makeKeyAndOrderFront:nil];
    }
    return result;
}


nfdresult_t NFD_OpenDialogMultipleN( const nfdpathset_t **outPaths,
                                     const nfdnfilteritem_t *filterList,
                                     nfdfiltersize_t filterCount,
                                     const nfdnchar_t *defaultPath )
{
    nfdresult_t result = NFD_CANCEL;
    @autoreleasepool {
        NSWindow *keyWindow = [[NSApplication sharedApplication] keyWindow];
        
        NSOpenPanel *dialog = [NSOpenPanel openPanel];
        [dialog setAllowsMultipleSelection:YES];
        
        // Build the filter list
        AddFilterListToDialog(dialog, filterList, filterCount);
        
        // Set the starting directory
        SetDefaultPath(dialog, defaultPath);
        
        if ( [dialog runModal] == NSModalResponseOK )
        {
            const NSArray *urls = [dialog URLs];
            
            if ([urls count] > 0) {
                // have at least one URL, we return this NSArray
                [urls retain];
                *outPaths = (const nfdpathset_t*)urls;
            }
            result = NFD_OKAY;
        }
        
        // return focus to the key window (i.e. main window)
        [keyWindow makeKeyAndOrderFront:nil];
    }
    return result;
}


nfdresult_t NFD_SaveDialogN( nfdnchar_t **outPath,
                             const nfdnfilteritem_t *filterList,
                             nfdfiltersize_t filterCount,
                             const nfdnchar_t *defaultPath,
                             const nfdnchar_t *defaultName )
{
    nfdresult_t result = NFD_CANCEL;
    @autoreleasepool {
        NSWindow *keyWindow = [[NSApplication sharedApplication] keyWindow];
        
        NSSavePanel *dialog = [NSSavePanel savePanel];
        [dialog setExtensionHidden:NO];
        // allow other file types, to give the user an escape hatch since you can't select "*.*" on Mac
        [dialog setAllowsOtherFileTypes:TRUE];
        
        // Build the filter list
        AddFilterListToDialog(dialog, filterList, filterCount);
        
        // Set the starting directory
        SetDefaultPath(dialog, defaultPath);
        
        // Set the default file name
        SetDefaultName(dialog, defaultName);
        
        if ( [dialog runModal] == NSModalResponseOK )
        {
            const NSURL *url = [dialog URL];
            const char *utf8Path = [[url path] UTF8String];
            
            // byte count, not char count
            size_t len = strlen(utf8Path);
            
            // too bad we have to use additional memory for this
            *outPath = (nfdchar_t*)NFDi_Malloc( len+1 );
            if (*outPath) {
                strcpy(*outPath, utf8Path);
                result = NFD_OKAY;
            }
        }
        
        // return focus to the key window (i.e. main window)
        [keyWindow makeKeyAndOrderFront:nil];
    }
    return result;
}

nfdresult_t NFD_PickFolderN( nfdnchar_t **outPath,
                             const nfdnchar_t *defaultPath )
{
    nfdresult_t result = NFD_CANCEL;
    @autoreleasepool {
        NSWindow *keyWindow = [[NSApplication sharedApplication] keyWindow];
        
        NSOpenPanel *dialog = [NSOpenPanel openPanel];
        [dialog setAllowsMultipleSelection:NO];
        [dialog setCanChooseDirectories:YES];
        [dialog setCanCreateDirectories:YES];
        [dialog setCanChooseFiles:NO];
        
        // Set the starting directory
        SetDefaultPath(dialog, defaultPath);
        
        if ( [dialog runModal] == NSModalResponseOK )
        {
            const NSURL *url = [dialog URL];
            const char *utf8Path = [[url path] UTF8String];
            
            // byte count, not char count
            size_t len = strlen(utf8Path);
            
            // too bad we have to use additional memory for this
            *outPath = (nfdchar_t*)NFDi_Malloc( len+1 );
            if (*outPath) {
                strcpy(*outPath, utf8Path);
                result = NFD_OKAY;
            }
        }
        
        // return focus to the key window (i.e. main window)
        [keyWindow makeKeyAndOrderFront:nil];
    }
    return result;
}



nfdresult_t NFD_PathSet_GetCount( const nfdpathset_t *pathSet, nfdpathsetsize_t* count )
{
    const NSArray *urls = (const NSArray*)pathSet;
    *count = [urls count];
    return NFD_OKAY;
}

nfdresult_t NFD_PathSet_GetPathN( const nfdpathset_t *pathSet, nfdpathsetsize_t index, nfdnchar_t **outPath )
{
    const NSArray *urls = (const NSArray*)pathSet;
    const NSURL *url = [urls objectAtIndex:index];
    
    @autoreleasepool {
        // autoreleasepool needed because UTF8String method might use the pool
        const char *utf8Path = [[url path] UTF8String];
        
        // byte count, not char count
        size_t len = strlen(utf8Path);
        
        // too bad we have to use additional memory for this
        *outPath = (nfdchar_t*)NFDi_Malloc( len+1 );
        if (*outPath) {
            strcpy(*outPath, utf8Path);
            return NFD_OKAY;
        }
    }
    
    return NFD_ERROR;
}

void NFD_PathSet_Free( const nfdpathset_t *pathSet ) {
    const NSArray *urls = (const NSArray*)pathSet;
    [urls release];
}
