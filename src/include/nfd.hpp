/*
Native File Dialog Extended
Author: Bernard Teo
License : Zlib
Repository: https://github.com/btzy/nativefiledialog-extended
Based on https://github.com/mlabbe/nativefiledialog

This header is a thin C++ wrapper for nfd.h.
C++ projects can choose to use this header instead of nfd.h directly.

Refer to documentation on nfd.h for instructions on how to use these functions.
*/

#ifndef _NFD_HPP
#define _NFD_HPP

#include "nfd.h"
#ifdef NFD_GUARD_THROWS_EXCEPTION
#include <stdexcept>
#endif

namespace NFD {
    class Guard {
    public:
#ifndef NFD_GUARD_THROWS_EXCEPTION
        inline Guard() noexcept {
            ::NFD_Init(); // always assume that initialization succeeds
        }
#else
        inline Guard() {
            if (!::NFD_Init()) {
                throw std::runtime_error(NFD_GetError());
            }
        }
#endif
        inline ~Guard() noexcept {
            ::NFD_Quit();
        }

        // Not allowed to copy or move this class
        Guard(const Guard&) = delete;
        Guard& operator=(const Guard&) = delete;
    };

    inline nfdresult_t Init() noexcept {
        return ::NFD_Init();
    }

    inline void Quit() noexcept {
        ::NFD_Quit();
    }

    inline void FreePath(nfdnchar_t *outPath) noexcept {
        ::NFD_FreePathN(outPath);
    }

    inline nfdresult_t OpenDialog(const nfdnfilteritem_t *filterList, nfd_filtersize_t filterCount, const nfdnchar_t *defaultPath, nfdnchar_t *&outPath) noexcept {
        return ::NFD_OpenDialogN(filterList, filterCount, defaultPath, &outPath);
    }

    inline nfdresult_t OpenDialogMultiple(const nfdnfilteritem_t *filterList, nfd_filtersize_t filterCount, const nfdnchar_t *defaultPath, const nfdpathset_t *&outPaths) noexcept {
        return ::NFD_OpenDialogMultipleN(filterList, filterCount, defaultPath, &outPaths);
    }

    inline nfdresult_t SaveDialog(const nfdnfilteritem_t *filterList, nfd_filtersize_t filterCount, const nfdnchar_t *defaultPath, nfdnchar_t *&outPath) noexcept {
        return ::NFD_SaveDialogN(filterList, filterCount, defaultPath, &outPath);
    }

    inline nfdresult_t PickFolder(const nfdnchar_t *defaultPath, nfdnchar_t *&outPath) noexcept {
        ::NFD_PickFolderN(defaultPath, &outPath);
    }

    inline const char *GetError() noexcept {
        return ::NFD_GetError();
    }

    inline void ClearError() noexcept {
        ::NFD_ClearError();
    }

    inline nfdresult_t CountPaths(const nfdpathset_t* pathSet, nfd_pathsetsize_t& count) noexcept {
        return ::NFD_PathSet_GetCount(pathSet, &count);
    }

    inline nfdresult_t GetPath(const nfdpathset_t *pathSet, nfd_pathsetsize_t index, nfdnchar_t *&outPath) noexcept {
        return ::NFD_PathSet_GetPathN(pathSet, index, &outPath);
    }

    inline void NFD_FreePathSet(const nfdpathset_t *pathSet) noexcept {
        ::NFD_PathSet_Free(pathSet);
    }

#ifdef NFD_DIFFERENT_NATIVE_FUNCTIONS
    /* we need the C++ bindings for the UTF-8 functions as well, because there are different functions for them */

    inline void FreePath(nfdu8char_t *outPath) noexcept {
        ::NFD_FreePathU8(outPath);
    }

    inline nfdresult_t OpenDialog(const nfdu8filteritem_t *filterList, nfd_filtersize_t count, const nfdu8char_t *defaultPath, nfdu8char_t *&outPath) noexcept {
        return ::NFD_OpenDialogU8(filterList, count, defaultPath, &outPath);
    }

    inline nfdresult_t OpenDialogMultiple(const nfdu8filteritem_t *filterList, nfd_filtersize_t count, const nfdu8char_t *defaultPath, const nfdpathset_t *&outPaths) noexcept {
        return ::NFD_OpenDialogMultipleU8(filterList, count, defaultPath, &outPaths);
    }

    inline nfdresult_t SaveDialog(const nfdu8filteritem_t *filterList, nfd_filtersize_t count, const nfdu8char_t *defaultPath, nfdu8char_t *&outPath) noexcept {
        return ::NFD_SaveDialogU8(filterList, count, defaultPath, &outPath);
    }

    inline nfdresult_t PickFolder(const nfdu8char_t *defaultPath, nfdu8char_t *&outPath) noexcept {
        ::NFD_PickFolderU8(defaultPath, &outPath);
    }

    inline nfdresult_t GetPath(const nfdpathset_t *pathSet, nfd_pathsetsize_t index, nfdu8char_t *&outPath) noexcept {
        ::NFD_PathSet_GetPathU8(pathSet, index, &outPath);
    }
#endif
}

#endif