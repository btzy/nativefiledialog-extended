/*
  Native File Dialog

  http://www.frogtoss.com/labs
 */

/* only locally define UNICODE in this compilation unit */
#ifndef UNICODE
#define UNICODE
#endif

#ifdef __MINGW32__
// Explicitly setting NTDDI version, this is necessary for the MinGW compiler
#define NTDDI_VERSION NTDDI_VISTA
#define _WIN32_WINNT _WIN32_WINNT_VISTA
#endif

#include <wchar.h>
#include <stdio.h>
#include <assert.h>
#include <windows.h>
#include <shobjidl.h>
#include "nfd.h"


namespace {
    template <typename T>
    struct Release_Guard {
        T* data;
        Release_Guard(T* releasable) noexcept : data(releasable) {}
        ~Release_Guard() {
            data->Release();
        }
    };
    template <typename T>
    struct Free_Guard {
        T* data;
        Free_Guard(T* freeable) noexcept : data(freeable) {}
        ~Free_Guard() {
            NFDi_Free(data);
        }
    };



    const char* g_errorstr = NULL;

    void NFDi_SetError(const char *msg)
    {
        g_errorstr = msg;
    }


    void *NFDi_Malloc(size_t bytes)
    {
        void *ptr = malloc(bytes);
        if (!ptr)
            NFDi_SetError("NFDi_Malloc failed.");

        return ptr;
    }

    void NFDi_Free(void *ptr)
    {
        assert(ptr);
        free(ptr);
    }

    nfdresult_t AddFiltersToDialog(::IFileDialog *fileOpenDialog, const nfdfilteritem_t *filterList, size_t filterCount)
    {

        if (!filterList) {
            assert(!filterCount);
            return NFD_OKAY;
        }



        /* filterCount plus 1 because we hardcode the *.* wildcard after the while loop */
        COMDLG_FILTERSPEC *specList = (COMDLG_FILTERSPEC*)NFDi_Malloc(sizeof(COMDLG_FILTERSPEC) * (filterCount + 1));
        if (!specList)
        {
            return NFD_ERROR;
        }

        struct COMDLG_FILTERSPEC_Guard {
            COMDLG_FILTERSPEC* _specList;
            size_t index;
            COMDLG_FILTERSPEC_Guard(COMDLG_FILTERSPEC* specList) noexcept : _specList(specList), index(0) {}
            ~COMDLG_FILTERSPEC_Guard() {
                for (size_t i = 0; i != index; ++i) {
                    NFDi_Free((void*)_specList[i].pszSpec);
                }
                NFDi_Free(_specList);
            }
        };

        COMDLG_FILTERSPEC_Guard specListGuard(specList);

        size_t& index = specListGuard.index;

        for (; index != filterCount; ++index) {
            // set the friendly name of this filter
            specList[index].pszName = filterList[index].name;

            // set the specification of this filter...

            // count number of file extensions
            size_t sep = 1;
            for (const nfdchar_t *p_spec = filterList[index].spec; *p_spec; ++p_spec) {
                if (*p_spec == L',') {
                    ++sep;
                }
            }

            // calculate space needed (including the trailing '\0')
            size_t specSize = sep * 2 + wcslen(filterList[index].spec) + 1;

            // malloc the required memory and populate it
            nfdchar_t *specBuf = (nfdchar_t*)NFDi_Malloc(specSize * sizeof(nfdchar_t));

            if (!specBuf) {
                // automatic freeing of memory via COMDLG_FILTERSPEC_Guard
                return NFD_ERROR;
            }

            nfdchar_t *p_specBuf = specBuf;
            *p_specBuf++ = L'*';
            *p_specBuf++ = L'.';
            for (const nfdchar_t *p_spec = filterList[index].spec; *p_spec; ++p_spec) {
                if (*p_spec == L',') {
                    *p_specBuf++ = L';';
                    *p_specBuf++ = L'*';
                    *p_specBuf++ = L'.';
                }
                else {
                    *p_specBuf++ = *p_spec;
                }
            }
            *p_specBuf++ = L'\0';

            assert(p_specBuf - specBuf == specSize);

            specList[index].pszSpec = specBuf;
        }

        /* Add wildcard */
        specList[filterCount].pszName = L"All files";
        specList[filterCount].pszSpec = L"*.*";

        fileOpenDialog->SetFileTypes(filterCount + 1, specList);

        // automatic freeing of memory via COMDLG_FILTERSPEC_Guard
        return NFD_OKAY;
    }

    nfdresult_t SetDefaultPath(IFileDialog *dialog, const nfdchar_t *defaultPath)
    {
        if (!defaultPath || wcslen(defaultPath) == 0)
            return NFD_OKAY;


        IShellItem *folder;
        HRESULT result = SHCreateItemFromParsingName(defaultPath, NULL, IID_PPV_ARGS(&folder));

        // Valid non results.
        if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) || result == HRESULT_FROM_WIN32(ERROR_INVALID_DRIVE))
        {
            return NFD_OKAY;
        }

        if (!SUCCEEDED(result))
        {
            NFDi_SetError("Error creating ShellItem");
            return NFD_ERROR;
        }

        // Could also call SetDefaultFolder(), but this guarantees defaultPath -- more consistency across API.
        dialog->SetFolder(folder);

        folder->Release();

        return NFD_OKAY;
    }
}


const char *NFD_GetError(void)
{
    return g_errorstr;
}



/* public */

nfdresult_t NFD_Init(void) {
    // Init COM library.
    HRESULT result = ::CoInitializeEx(NULL, ::COINIT_APARTMENTTHREADED | ::COINIT_DISABLE_OLE1DDE);

    if (!SUCCEEDED(result)) {
        return NFD_ERROR;
    }
    else {
        return NFD_OKAY;
    }
}
void NFD_Quit(void) {
    ::CoUninitialize();
}

void NFD_FreePath(nfdchar_t* filePath) {
    assert(filePath);
    ::CoTaskMemFree(filePath);
}


nfdresult_t NFD_OpenDialog( const nfdfilteritem_t *filterList,
                            size_t count,
                            const nfdchar_t *defaultPath,
                            nfdchar_t **outPath )
{

    ::IFileOpenDialog *fileOpenDialog;

    // Create dialog
    HRESULT result = ::CoCreateInstance(::CLSID_FileOpenDialog, NULL,
                                CLSCTX_ALL, ::IID_IFileOpenDialog,
                                reinterpret_cast<void**>(&fileOpenDialog) );
                                
    if ( !SUCCEEDED(result) )
    {
        NFDi_SetError("Could not create dialog.");
        return NFD_ERROR;
    }

    // make sure we remember to free the dialog
    Release_Guard<::IFileOpenDialog> fileOpenDialogGuard(fileOpenDialog);

    // Build the filter list
    if ( !AddFiltersToDialog( fileOpenDialog, filterList, count ) )
    {
        return NFD_ERROR;
    }

    // Set the default path
    if ( !SetDefaultPath( fileOpenDialog, defaultPath ) )
    {
        return NFD_ERROR;
    }    

    // Show the dialog.
    result = fileOpenDialog->Show(NULL);
    if ( SUCCEEDED(result) )
    {
        // Get the file name
        ::IShellItem *psiResult;
        result = fileOpenDialog->GetResult(&psiResult);
        if ( !SUCCEEDED(result) )
        {
            NFDi_SetError("Could not get shell item from dialog.");
            return NFD_ERROR;
        }
        Release_Guard<::IShellItem> psiResultGuard(psiResult);

        nfdchar_t *filePath;
        result = psiResult->GetDisplayName(::SIGDN_FILESYSPATH, &filePath);
        if ( !SUCCEEDED(result) )
        {
            NFDi_SetError("Could not get file path for selected.");
            return NFD_ERROR;
        }

        *outPath = filePath;

        return NFD_OKAY;
    }
    else if (result == HRESULT_FROM_WIN32(ERROR_CANCELLED) )
    {
        return NFD_CANCEL;
    }
    else
    {
        NFDi_SetError("File dialog box show failed.");
        return NFD_ERROR;
    }
}

nfdresult_t NFD_OpenDialogMultiple( const nfdfilteritem_t *filterList,
                                    size_t count,
                                    const nfdchar_t *defaultPath,
                                    nfdpathset_t **outPaths )
{
    ::IFileOpenDialog *fileOpenDialog(NULL);

    // Create dialog
    HRESULT result = ::CoCreateInstance(::CLSID_FileOpenDialog, NULL,
                                CLSCTX_ALL, ::IID_IFileOpenDialog,
                                reinterpret_cast<void**>(&fileOpenDialog) );
                                
    if ( !SUCCEEDED(result) )
    {
        NFDi_SetError("Could not create dialog.");
        return NFD_ERROR;
    }

    // make sure we remember to free the dialog
    Release_Guard<::IFileOpenDialog> fileOpenDialogGuard(fileOpenDialog);

    // Build the filter list
    if ( !AddFiltersToDialog( fileOpenDialog, filterList, count ) )
    {
        return NFD_ERROR;
    }

    // Set the default path
    if ( !SetDefaultPath( fileOpenDialog, defaultPath ) )
    {
        return NFD_ERROR;
    }

    // Set a flag for multiple options
    DWORD dwFlags;
    result = fileOpenDialog->GetOptions(&dwFlags);
    if ( !SUCCEEDED(result) )
    {
        NFDi_SetError("Could not get options.");
        return NFD_ERROR;
    }
    result = fileOpenDialog->SetOptions(dwFlags | FOS_ALLOWMULTISELECT);
    if ( !SUCCEEDED(result) )
    {
        NFDi_SetError("Could not set options.");
        return NFD_ERROR;
    }
 
    // Show the dialog.
    result = fileOpenDialog->Show(NULL);
    if ( SUCCEEDED(result) )
    {
        ::IShellItemArray *shellItems;
        result = fileOpenDialog->GetResults( &shellItems );
        if ( !SUCCEEDED(result) )
        {
            NFDi_SetError("Could not get shell items.");
            return NFD_ERROR;
        }
        
        // save the path set to the output
        *outPaths = static_cast<void*>(shellItems);

        return NFD_OKAY;
    }
    else if (result == HRESULT_FROM_WIN32(ERROR_CANCELLED) )
    {
        return NFD_CANCEL;
    }
    else
    {
        NFDi_SetError("File dialog box show failed.");
        return NFD_ERROR;
    }
}

nfdresult_t NFD_SaveDialog( const nfdfilteritem_t *filterList,
                            size_t count,
                            const nfdchar_t *defaultPath,
                            nfdchar_t **outPath )
{
    
    ::IFileSaveDialog *fileSaveDialog;

    // Create dialog
    HRESULT result = ::CoCreateInstance(::CLSID_FileSaveDialog, NULL,
                                CLSCTX_ALL, ::IID_IFileSaveDialog,
                                reinterpret_cast<void**>(&fileSaveDialog) );

    if ( !SUCCEEDED(result) )
    {
        NFDi_SetError("Could not create dialog.");
        return NFD_ERROR;
    }

    // make sure we remember to free the dialog
    Release_Guard<::IFileSaveDialog> fileSaveDialogGuard(fileSaveDialog);

    // Build the filter list
    if ( !AddFiltersToDialog( fileSaveDialog, filterList, count ) )
    {
        return NFD_ERROR;
    }

    // Set the default path
    if ( !SetDefaultPath( fileSaveDialog, defaultPath ) )
    {
        return NFD_ERROR;
    }

    // Show the dialog.
    result = fileSaveDialog->Show(NULL);
    if ( SUCCEEDED(result) )
    {
        // Get the file name
        ::IShellItem *psiResult;
        result = fileSaveDialog->GetResult(&psiResult);
        if ( !SUCCEEDED(result) )
        {
            NFDi_SetError("Could not get shell item from dialog.");
            return NFD_ERROR;
        }
        Release_Guard<::IShellItem> psiResultGuard(psiResult);

        nfdchar_t *filePath;
        result = psiResult->GetDisplayName(::SIGDN_FILESYSPATH, &filePath);
        if ( !SUCCEEDED(result) )
        {
            NFDi_SetError("Could not get file path for selected.");
            return NFD_ERROR;
        }

        *outPath = filePath;

        return NFD_OKAY;
    }
    else if (result == HRESULT_FROM_WIN32(ERROR_CANCELLED) )
    {
        return NFD_CANCEL;
    }
    else
    {
        NFDi_SetError("File dialog box show failed.");
        return NFD_ERROR;
    }
}


// VS2010 hasn't got a copy of CComPtr - this was first added in the 2003 SDK, so we make our own small CComPtr instead
template<class T>
class ComPtr
{
public:
    ComPtr() : mPtr(NULL) { }
    ~ComPtr()
    {
        if (mPtr)
        {
            mPtr->Release();
        }
    }

    T* Ptr() const { return mPtr; }
    T** operator&() { return &mPtr; }
    T* operator->() const { return mPtr; }
private:
    // Don't allow copy or assignment
    ComPtr(const ComPtr&);
    ComPtr& operator = (const ComPtr&) const;
    T* mPtr;
};

nfdresult_t NFD_PickFolder(const nfdchar_t *defaultPath,
    nfdchar_t **outPath)
{
    ::IFileOpenDialog *fileOpenDialog;

    // Create dialog
    if (!SUCCEEDED( ::CoCreateInstance(::CLSID_FileOpenDialog, NULL,
                                CLSCTX_ALL, ::IID_IFileOpenDialog,
                                reinterpret_cast<void**>(&fileOpenDialog) ) ))
    {
        NFDi_SetError("Could not create dialog.");
        return NFD_ERROR;
    }

    Release_Guard<::IFileOpenDialog> fileOpenDialogGuard(fileOpenDialog);

    // Set the default path
    if (!SetDefaultPath(fileOpenDialog, defaultPath))
    {
        return NFD_ERROR;
    }

    // Get the dialogs options
    DWORD dwOptions = 0;
    if (!SUCCEEDED(fileOpenDialog->GetOptions(&dwOptions)))
    {
        NFDi_SetError("GetOptions for IFileDialog failed.");
        return NFD_ERROR;
    }

    // Add in FOS_PICKFOLDERS which hides files and only allows selection of folders
    if (!SUCCEEDED(fileOpenDialog->SetOptions(dwOptions | FOS_PICKFOLDERS)))
    {
        NFDi_SetError("SetOptions for IFileDialog failed.");
        return NFD_ERROR;
    }

    // Show the dialog to the user
    const HRESULT result = fileOpenDialog->Show(NULL);
    if (result == HRESULT_FROM_WIN32(ERROR_CANCELLED))
    {
        return NFD_CANCEL;
    }
    else if (!SUCCEEDED(result))
    {
        NFDi_SetError("File dialog box show failed.");
        return NFD_ERROR;
    }

    // Get the shell item result
    ::IShellItem *psiResult;
    if (!SUCCEEDED(fileOpenDialog->GetResult(&psiResult)))
    {
        return NFD_ERROR;
    }

    Release_Guard<::IShellItem> psiResultGuard(psiResult);

    // Finally get the path
    nfdchar_t *filePath;
    // Why are we not using SIGDN_FILESYSPATH?
    if (!SUCCEEDED(psiResult->GetDisplayName(::SIGDN_DESKTOPABSOLUTEPARSING, &filePath)))
    {
        NFDi_SetError("Could not get file path for selected.");
        return NFD_ERROR;
    }

    *outPath = filePath;

    return NFD_OKAY;
}


nfdresult_t NFD_PathSet_GetCount(const nfdpathset_t *pathSet, nfd_pathsetsize_t* count)
{
    assert(pathSet);
    // const_cast because methods on IShellItemArray aren't const, but it should act like const to the caller
    ::IShellItemArray *psiaPathSet = const_cast<::IShellItemArray*>(static_cast<const ::IShellItemArray*>(pathSet));

    DWORD numPaths;
    HRESULT result = psiaPathSet->GetCount(&numPaths);
    if (!SUCCEEDED(result))
    {
        NFDi_SetError("Could not get path count");
        return NFD_ERROR;
    }
    *count = numPaths;
    return NFD_OKAY;
}

nfdresult_t NFD_PathSet_GetPath(const nfdpathset_t *pathSet, nfd_pathsetsize_t index, nfdchar_t **outPath)
{
    assert(pathSet);
    // const_cast because methods on IShellItemArray aren't const, but it should act like const to the caller
    ::IShellItemArray *psiaPathSet = const_cast<::IShellItemArray*>(static_cast<const ::IShellItemArray*>(pathSet));

    ::IShellItem *psiPath;
    if (!SUCCEEDED(psiaPathSet->GetItemAt(index, &psiPath))) {
        NFDi_SetError("Could not get shell item");
        return NFD_ERROR;
    }

    nfdchar_t* name;
    if (!SUCCEEDED(psiPath->GetDisplayName(::SIGDN_FILESYSPATH, &name))) {
        NFDi_SetError("Could not get file path for selected.");
        return NFD_ERROR;
    }

    *outPath = name;
    return NFD_OKAY;
}

void NFD_PathSet_Free(nfdpathset_t *pathSet)
{
    assert(pathSet);
    // const_cast because methods on IShellItemArray aren't const, but it should act like const to the caller
    ::IShellItemArray *psiaPathSet = const_cast<::IShellItemArray*>(static_cast<const ::IShellItemArray*>(pathSet));
    
    // free the path set memory
    psiaPathSet->Release();
}
