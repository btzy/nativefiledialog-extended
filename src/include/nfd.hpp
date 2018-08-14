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
#include <cstddef> // for std::size_t
#include <memory> // for std::unique_ptr
#ifdef NFD_GUARD_THROWS_EXCEPTION
#include <stdexcept>
#endif

namespace NFD {

    inline nfdresult_t Init() noexcept {
        return ::NFD_Init();
    }

    inline void Quit() noexcept {
        ::NFD_Quit();
    }

    inline void FreePath(nfdnchar_t *outPath) noexcept {
        ::NFD_FreePathN(outPath);
    }

    inline nfdresult_t OpenDialog(const nfdnfilteritem_t *filterList, nfdfiltersize_t filterCount, const nfdnchar_t *defaultPath, nfdnchar_t *&outPath) noexcept {
        return ::NFD_OpenDialogN(filterList, filterCount, defaultPath, &outPath);
    }

    inline nfdresult_t OpenDialogMultiple(const nfdnfilteritem_t *filterList, nfdfiltersize_t filterCount, const nfdnchar_t *defaultPath, const nfdpathset_t *&outPaths) noexcept {
        return ::NFD_OpenDialogMultipleN(filterList, filterCount, defaultPath, &outPaths);
    }

    inline nfdresult_t SaveDialog(const nfdnfilteritem_t *filterList, nfdfiltersize_t filterCount, const nfdnchar_t *defaultPath, nfdnchar_t *&outPath) noexcept {
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

    namespace PathSet {
        inline nfdresult_t Count(const nfdpathset_t* pathSet, nfdpathsetsize_t& count) noexcept {
            return ::NFD_PathSet_GetCount(pathSet, &count);
        }

        inline nfdresult_t GetPath(const nfdpathset_t *pathSet, nfdpathsetsize_t index, nfdnchar_t *&outPath) noexcept {
            return ::NFD_PathSet_GetPathN(pathSet, index, &outPath);
        }

        inline void FreePath(nfdnchar_t *filePath) noexcept {
            ::NFD_PathSet_FreePathN(filePath);
        }

        inline void Free(const nfdpathset_t *pathSet) noexcept {
            ::NFD_PathSet_Free(pathSet);
        }
    }

#ifdef NFD_DIFFERENT_NATIVE_FUNCTIONS
    /* we need the C++ bindings for the UTF-8 functions as well, because there are different functions for them */

    inline void FreePath(nfdu8char_t *outPath) noexcept {
        ::NFD_FreePathU8(outPath);
    }

    inline nfdresult_t OpenDialog(const nfdu8filteritem_t *filterList, nfdfiltersize_t count, const nfdu8char_t *defaultPath, nfdu8char_t *&outPath) noexcept {
        return ::NFD_OpenDialogU8(filterList, count, defaultPath, &outPath);
    }

    inline nfdresult_t OpenDialogMultiple(const nfdu8filteritem_t *filterList, nfdfiltersize_t count, const nfdu8char_t *defaultPath, const nfdpathset_t *&outPaths) noexcept {
        return ::NFD_OpenDialogMultipleU8(filterList, count, defaultPath, &outPaths);
    }

    inline nfdresult_t SaveDialog(const nfdu8filteritem_t *filterList, nfdfiltersize_t count, const nfdu8char_t *defaultPath, nfdu8char_t *&outPath) noexcept {
        return ::NFD_SaveDialogU8(filterList, count, defaultPath, &outPath);
    }

    inline nfdresult_t PickFolder(const nfdu8char_t *defaultPath, nfdu8char_t *&outPath) noexcept {
        ::NFD_PickFolderU8(defaultPath, &outPath);
    }

    namespace PathSet {
        inline nfdresult_t GetPath(const nfdpathset_t *pathSet, nfdpathsetsize_t index, nfdu8char_t *&outPath) noexcept {
            ::NFD_PathSet_GetPathU8(pathSet, index, &outPath);
        }
        inline void FreePath(nfdu8char_t *filePath) noexcept {
            ::NFD_PathSet_FreePathU8(filePath);
        }
    }
#endif


    // smart objects

    class Guard {
    public:
#ifndef NFD_CPP_EXCEPTION
        inline Guard() noexcept {
            Init(); // always assume that initialization succeeds
        }
#else
        inline Guard() {
            if (!Init()) {
                throw std::runtime_error(GetError());
            }
        }
#endif
        inline ~Guard() noexcept {
            Quit();
        }

        // Not allowed to copy or move this class
        Guard(const Guard&) = delete;
        Guard& operator=(const Guard&) = delete;
    };

    template <typename T>
    struct PathDeleter {
        inline void operator()(T* ptr) const noexcept {
            FreePath(ptr);
        }
    };

    typedef std::unique_ptr<nfdchar_t, PathDeleter<nfdchar_t>> UniquePath;
    typedef std::unique_ptr<nfdnchar_t, PathDeleter<nfdnchar_t>> UniquePathN;
    typedef std::unique_ptr<nfdu8char_t, PathDeleter<nfdu8char_t>> UniquePathU8;

    struct PathSetDeleter {
        inline void operator()(const nfdpathset_t* ptr) const noexcept {
            PathSet::Free(ptr);
        }
    };

    typedef std::unique_ptr<const nfdpathset_t, PathSetDeleter> UniquePathSet;

    template <typename T>
    struct PathSetPathDeleter {
        inline void operator()(T* ptr) const noexcept {
            PathSet::FreePath(ptr);
        }
    };

    typedef std::unique_ptr<nfdchar_t, PathSetPathDeleter<nfdchar_t>> UniquePathSetPath;
    typedef std::unique_ptr<nfdnchar_t, PathSetPathDeleter<nfdnchar_t>> UniquePathSetPathN;
    typedef std::unique_ptr<nfdu8char_t, PathSetPathDeleter<nfdu8char_t>> UniquePathSetPathU8;

    
    inline nfdresult_t OpenDialog(const nfdnfilteritem_t* filterList, nfdfiltersize_t filterCount, const nfdnchar_t* defaultPath, UniquePathN& outPath) noexcept {
        nfdnchar_t* out;
        nfdresult_t res = OpenDialog(filterList, filterCount, defaultPath, out);
        if (res) {
            outPath.reset(out);
        }
        return res;
    }

    inline nfdresult_t OpenDialogMultiple(const nfdnfilteritem_t* filterList, nfdfiltersize_t filterCount, const nfdnchar_t* defaultPath, UniquePathSet& outPaths) noexcept {
        const nfdpathset_t* out;
        nfdresult_t res = OpenDialogMultiple(filterList, filterCount, defaultPath, out);
        if (res) {
            outPaths.reset(out);
        }
        return res;
    }

    inline nfdresult_t SaveDialog(const nfdnfilteritem_t* filterList, nfdfiltersize_t filterCount, const nfdnchar_t* defaultPath, UniquePathN& outPath) noexcept {
        nfdnchar_t* out;
        nfdresult_t res = SaveDialog(filterList, filterCount, defaultPath, out);
        if (res) {
            outPath.reset(out);
        }
        return res;
    }

    inline nfdresult_t PickFolder(const nfdnchar_t* defaultPath, UniquePathN outPath) noexcept {
        nfdnchar_t* out;
        nfdresult_t res = PickFolder(defaultPath, out);
        if (res) {
            outPath.reset(out);
        }
        return res;
    }

#ifdef NFD_DIFFERENT_NATIVE_FUNCTIONS
    inline nfdresult_t OpenDialog(const nfdu8filteritem_t* filterList, nfdfiltersize_t filterCount, const nfdu8char_t* defaultPath, UniquePathU8& outPath) noexcept {
        nfdu8char_t* out;
        nfdresult_t res = OpenDialog(filterList, filterCount, defaultPath, out);
        if (res) {
            outPath.reset(out);
        }
        return res;
    }

    inline nfdresult_t OpenDialogMultiple(const nfdu8filteritem_t* filterList, nfdfiltersize_t filterCount, const nfdu8char_t* defaultPath, UniquePathSet& outPaths) noexcept {
        const nfdpathset_t* out;
        nfdresult_t res = OpenDialogMultiple(filterList, filterCount, defaultPath, out);
        if (res) {
            outPaths.reset(out);
        }
        return res;
    }

    inline nfdresult_t SaveDialog(const nfdu8filteritem_t* filterList, nfdfiltersize_t filterCount, const nfdu8char_t* defaultPath, UniquePathU8& outPath) noexcept {
        nfdu8char_t* out;
        nfdresult_t res = SaveDialog(filterList, filterCount, defaultPath, out);
        if (res) {
            outPath.reset(out);
        }
        return res;
    }

    inline nfdresult_t PickFolder(const nfdu8char_t* defaultPath, UniquePathU8 outPath) noexcept {
        nfdu8char_t* out;
        nfdresult_t res = PickFolder(defaultPath, out);
        if (res) {
            outPath.reset(out);
        }
        return res;
    }
#endif

    namespace PathSet {
        inline nfdresult_t Count(const UniquePathSet& uniquePathSet, nfdpathsetsize_t& count) noexcept {
            return Count(uniquePathSet.get(), count);
        }
        inline nfdresult_t GetPath(const UniquePathSet& uniquePathSet, nfdpathsetsize_t index, UniquePathSetPathN& outPath) noexcept {
            nfdnchar_t* out;
            nfdresult_t res = GetPath(uniquePathSet.get(), index, out);
            if (res) {
                outPath.reset(out);
            }
            return res;
        }
#ifdef NFD_DIFFERENT_NATIVE_FUNCTIONS
        inline nfdresult_t GetPath(const UniquePathSet& uniquePathSet, nfdpathsetsize_t index, UniquePathSetPathU8& outPath) noexcept {
            nfdu8char_t* out;
            nfdresult_t res = GetPath(uniquePathSet.get(), index, out);
            if (res) {
                outPath.reset(out);
            }
            return res;
        }
#endif
    }

}

#endif