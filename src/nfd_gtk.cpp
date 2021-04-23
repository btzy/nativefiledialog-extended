/*
  Native File Dialog Extended
  Repository: https://github.com/btzy/nativefiledialog-extended
  License: Zlib
  Authors: Bernard Teo, Michael Labbe

  Note: We do not check for malloc failure on Linux - Linux overcommits memory!
  Note: The GTK4 implementation does not distinguish between local and network files,
  so it is possible for the file picker to return a network URI instead of a local filename.
*/

#include <assert.h>
#if NFD_GTK_VERSION == 4
#include <errno.h>
#endif
#include <gtk/gtk.h>
#if defined(GDK_WINDOWING_X11)
#if NFD_GTK_VERSION == 3
#include <gdk/gdkx.h>
#elif NFD_GTK_VERSION == 4
#include <gdk/x11/gdkx.h>
#endif
#endif
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nfd.h"

#define UNSUPPORTED_ERROR() static_assert(false, "Unsupported GTK version, this is an NFD bug.")

namespace {

template <typename T>
struct Free_Guard {
    T* data;
    Free_Guard(T* freeable) noexcept : data(freeable) {}
    ~Free_Guard() { NFDi_Free(data); }
};

template <typename T>
struct FreeCheck_Guard {
    T* data;
    FreeCheck_Guard(T* freeable = nullptr) noexcept : data(freeable) {}
    ~FreeCheck_Guard() {
        if (data) NFDi_Free(data);
    }
};

template <typename T>
struct GUnref_Guard {
    T* data;
    GUnref_Guard(T* object) noexcept : data(object) {}
    ~GUnref_Guard() { g_object_unref(data); }
    T* get() const noexcept { return data; }
};

/* current error */
const char* g_errorstr = nullptr;

void NFDi_SetError(const char* msg) {
    g_errorstr = msg;
}

template <typename T = void>
T* NFDi_Malloc(size_t bytes) {
    void* ptr = malloc(bytes);
    if (!ptr) NFDi_SetError("NFDi_Malloc failed.");

    return static_cast<T*>(ptr);
}

template <typename T>
void NFDi_Free(T* ptr) {
    assert(ptr);
    free(static_cast<void*>(ptr));
}

template <typename T>
T* copy(const T* begin, const T* end, T* out) {
    for (; begin != end; ++begin) {
        *out++ = *begin;
    }
    return out;
}

// Does not own the filter and extension.
struct Pair_GtkFileFilter_FileExtension {
    GtkFileFilter* filter;
    const nfdnchar_t* extensionBegin;
    const nfdnchar_t* extensionEnd;
};

struct ButtonClickedArgs {
    Pair_GtkFileFilter_FileExtension* map;
    GtkFileChooser* chooser;
};

void AddFiltersToDialog(GtkFileChooser* chooser,
                        const nfdnfilteritem_t* filterList,
                        nfdfiltersize_t filterCount) {
    if (filterCount) {
        assert(filterList);

        // we have filters to add ... format and add them

        for (nfdfiltersize_t index = 0; index != filterCount; ++index) {
            GtkFileFilter* filter = gtk_file_filter_new();

            // count number of file extensions
            size_t sep = 1;
            for (const nfdnchar_t* p_spec = filterList[index].spec; *p_spec; ++p_spec) {
                if (*p_spec == L',') {
                    ++sep;
                }
            }

            // friendly name conversions: "png,jpg" -> "Image files
            // (png, jpg)"

            // calculate space needed (including the trailing '\0')
            size_t nameSize =
                sep + strlen(filterList[index].spec) + 3 + strlen(filterList[index].name);

            // malloc the required memory
            nfdnchar_t* nameBuf = NFDi_Malloc<nfdnchar_t>(sizeof(nfdnchar_t) * nameSize);

            nfdnchar_t* p_nameBuf = nameBuf;
            for (const nfdnchar_t* p_filterName = filterList[index].name; *p_filterName;
                 ++p_filterName) {
                *p_nameBuf++ = *p_filterName;
            }
            *p_nameBuf++ = ' ';
            *p_nameBuf++ = '(';
            const nfdnchar_t* p_extensionStart = filterList[index].spec;
            for (const nfdnchar_t* p_spec = filterList[index].spec; true; ++p_spec) {
                if (*p_spec == ',' || !*p_spec) {
                    if (*p_spec == ',') {
                        *p_nameBuf++ = ',';
                        *p_nameBuf++ = ' ';
                    }

                    // +1 for the trailing '\0'
                    nfdnchar_t* extnBuf = NFDi_Malloc<nfdnchar_t>(sizeof(nfdnchar_t) *
                                                                  (p_spec - p_extensionStart + 3));
                    nfdnchar_t* p_extnBufEnd = extnBuf;
                    *p_extnBufEnd++ = '*';
                    *p_extnBufEnd++ = '.';
                    p_extnBufEnd = copy(p_extensionStart, p_spec, p_extnBufEnd);
                    *p_extnBufEnd++ = '\0';
                    assert((size_t)(p_extnBufEnd - extnBuf) ==
                           sizeof(nfdnchar_t) * (p_spec - p_extensionStart + 3));
                    gtk_file_filter_add_pattern(filter, extnBuf);
                    NFDi_Free(extnBuf);

                    if (*p_spec) {
                        // update the extension start point
                        p_extensionStart = p_spec + 1;
                    } else {
                        // reached the '\0' character
                        break;
                    }
                } else {
                    *p_nameBuf++ = *p_spec;
                }
            }
            *p_nameBuf++ = ')';
            *p_nameBuf++ = '\0';
            assert((size_t)(p_nameBuf - nameBuf) == sizeof(nfdnchar_t) * nameSize);

            // add to the filter
            gtk_file_filter_set_name(filter, nameBuf);

            // free the memory
            NFDi_Free(nameBuf);

            // add filter to chooser
            gtk_file_chooser_add_filter(chooser, filter);
        }
    }

    /* always append a wildcard option to the end*/

    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "All files");
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(chooser, filter);
}

// returns null-terminated map (trailing .filter is null)
Pair_GtkFileFilter_FileExtension* AddFiltersToDialogWithMap(GtkFileChooser* chooser,
                                                            const nfdnfilteritem_t* filterList,
                                                            nfdfiltersize_t filterCount) {
    Pair_GtkFileFilter_FileExtension* map = NFDi_Malloc<Pair_GtkFileFilter_FileExtension>(
        sizeof(Pair_GtkFileFilter_FileExtension) * (filterCount + 1));

    if (filterCount) {
        assert(filterList);

        // we have filters to add ... format and add them

        for (nfdfiltersize_t index = 0; index != filterCount; ++index) {
            GtkFileFilter* filter = gtk_file_filter_new();

            // store filter in map
            map[index].filter = filter;
            map[index].extensionBegin = filterList[index].spec;
            map[index].extensionEnd = nullptr;

            // count number of file extensions
            size_t sep = 1;
            for (const nfdnchar_t* p_spec = filterList[index].spec; *p_spec; ++p_spec) {
                if (*p_spec == L',') {
                    ++sep;
                }
            }

            // friendly name conversions: "png,jpg" -> "Image files
            // (png, jpg)"

            // calculate space needed (including the trailing '\0')
            size_t nameSize =
                sep + strlen(filterList[index].spec) + 3 + strlen(filterList[index].name);

            // malloc the required memory
            nfdnchar_t* nameBuf = NFDi_Malloc<nfdnchar_t>(sizeof(nfdnchar_t) * nameSize);

            nfdnchar_t* p_nameBuf = nameBuf;
            for (const nfdnchar_t* p_filterName = filterList[index].name; *p_filterName;
                 ++p_filterName) {
                *p_nameBuf++ = *p_filterName;
            }
            *p_nameBuf++ = ' ';
            *p_nameBuf++ = '(';
            const nfdnchar_t* p_extensionStart = filterList[index].spec;
            for (const nfdnchar_t* p_spec = filterList[index].spec; true; ++p_spec) {
                if (*p_spec == ',' || !*p_spec) {
                    if (*p_spec == ',') {
                        *p_nameBuf++ = ',';
                        *p_nameBuf++ = ' ';
                    }

                    // +1 for the trailing '\0'
                    nfdnchar_t* extnBuf = NFDi_Malloc<nfdnchar_t>(sizeof(nfdnchar_t) *
                                                                  (p_spec - p_extensionStart + 3));
                    nfdnchar_t* p_extnBufEnd = extnBuf;
                    *p_extnBufEnd++ = '*';
                    *p_extnBufEnd++ = '.';
                    p_extnBufEnd = copy(p_extensionStart, p_spec, p_extnBufEnd);
                    *p_extnBufEnd++ = '\0';
                    assert((size_t)(p_extnBufEnd - extnBuf) ==
                           sizeof(nfdnchar_t) * (p_spec - p_extensionStart + 3));
                    gtk_file_filter_add_pattern(filter, extnBuf);
                    NFDi_Free(extnBuf);

                    // store current pointer in map (if it's
                    // the first one)
                    if (map[index].extensionEnd == nullptr) {
                        map[index].extensionEnd = p_spec;
                    }

                    if (*p_spec) {
                        // update the extension start point
                        p_extensionStart = p_spec + 1;
                    } else {
                        // reached the '\0' character
                        break;
                    }
                } else {
                    *p_nameBuf++ = *p_spec;
                }
            }
            *p_nameBuf++ = ')';
            *p_nameBuf++ = '\0';
            assert((size_t)(p_nameBuf - nameBuf) == sizeof(nfdnchar_t) * nameSize);

            // add to the filter
            gtk_file_filter_set_name(filter, nameBuf);

            // free the memory
            NFDi_Free(nameBuf);

            // add filter to chooser
            gtk_file_chooser_add_filter(chooser, filter);
        }
    }
    // set trailing map index to null
    map[filterCount].filter = nullptr;

    /* always append a wildcard option to the end*/
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "All files");
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(chooser, filter);

    return map;
}

/*
Note: GTK+ manual recommends not specifically setting the default path.
We do it anyway in order to be consistent across platforms.

If consistency with the native OS is preferred,
then this function should be made a no-op.
*/
#if NFD_GTK_VERSION == 3
nfdresult_t SetDefaultPath(GtkFileChooser* chooser, const char* defaultPath) {
    if (!defaultPath || !*defaultPath) return NFD_OKAY;

    gtk_file_chooser_set_current_folder(chooser, defaultPath);
    return NFD_OKAY;
}
#elif NFD_GTK_VERSION == 4
nfdresult_t SetDefaultPath(GtkFileChooser* chooser, const char* defaultPath) {
    if (!defaultPath || !*defaultPath) return NFD_OKAY;

    GUnref_Guard<GFile> file(g_file_new_for_path(defaultPath));

    if (!gtk_file_chooser_set_current_folder(chooser, file.get(), NULL)) {
        NFDi_SetError("Failed to set default path.");
        return NFD_ERROR;
    }
    return NFD_OKAY;
}
#endif

void SetDefaultName(GtkFileChooser* chooser, const char* defaultName) {
    if (!defaultName || !*defaultName) return;

    gtk_file_chooser_set_current_name(chooser, defaultName);
}

#if NFD_GTK_VERSION == 3
nfdresult_t GetSingleFileNameForOpen(GtkWidget* widget, char** outPath) {
    char* tmp_outPath = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget));
    if (tmp_outPath) {
        *outPath = tmp_outPath;
        return NFD_OKAY;
    }
    return NFD_ERROR;
}
nfdresult_t GetSingleFileNameForSave(GtkWidget* widget, char** outPath) {
    return GetSingleFileNameForOpen(widget, outPath);
}
nfdresult_t GetMultipleFileNamesForOpen(GtkWidget* widget, const nfdpathset_t** outPaths) {
    GSList* fileList = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(widget));
    if (fileList) {
        *outPaths = static_cast<void*>(fileList);
        return NFD_OKAY;
    }
    return NFD_ERROR;
}
#elif NFD_GTK_VERSION == 4
// Helper method to get a local file path from a GFile,
// potentially downloading the file if it is a non-local file.
// This function takes ownership of the given gfile, and will call g_object_unref() to release it.
// Note: downloads may take long, but there is no way for the user to cancel it
nfdresult_t GetFileNameFromGFile(GFile* gfile, char** outPath) {
    GUnref_Guard<GFile> file(gfile);
    char* tmp_outPath = g_file_get_path(file.get());
    if (tmp_outPath) {
        *outPath = tmp_outPath;
        return NFD_OKAY;
    }
    // it's not a local file... we should copy it
    GFileIOStream* localFileIOStream;
    GFile* localFile = g_file_new_tmp(NULL, &localFileIOStream, NULL);
    if (!localFile) return NFD_ERROR;
    GUnref_Guard<GFile> localFileGuard(localFile);
    GUnref_Guard<GFileIOStream> localFileIOStreamGuard(localFileIOStream);
    g_io_stream_close(G_IO_STREAM(localFileIOStream), NULL, NULL);
    if (!g_file_copy(file.get(), localFile, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, NULL))
        return NFD_ERROR;
    *outPath = g_file_get_path(localFile);
    return NFD_OKAY;
}

nfdresult_t GetSingleFileNameForOpen(GtkWidget* widget, char** outPath) {
    return GetFileNameFromGFile(gtk_file_chooser_get_file(GTK_FILE_CHOOSER(widget)), outPath);
}
nfdresult_t GetSingleFileNameForSave(GtkWidget* widget, char** outPath) {
    GUnref_Guard<GFile> file(gtk_file_chooser_get_file(GTK_FILE_CHOOSER(widget)));
    char* tmp_outPath = g_file_get_path(file.get());
    if (tmp_outPath) {
        *outPath = tmp_outPath;
        return NFD_OKAY;
    }
    // it's not a local file... we balk and say the user cancelled the dialog
    return NFD_CANCEL;
}
nfdresult_t GetMultipleFileNamesForOpen(GtkWidget* widget, const nfdpathset_t** outPaths) {
    GListModel* fileList = gtk_file_chooser_get_files(GTK_FILE_CHOOSER(widget));
    if (fileList) {
        *outPaths = static_cast<void*>(fileList);
        return NFD_OKAY;
    }
    return NFD_ERROR;
}
#endif

void WaitForCleanup() {
#if NFD_GTK_VERSION == 3
    while (gtk_events_pending()) gtk_main_iteration();
#elif NFD_GTK_VERSION == 4
    while (g_main_context_iteration(NULL, FALSE))
        ;
#else
    UNSUPPORTED_ERROR();
#endif
}

struct Widget_Guard {
    GtkWidget* data;
    Widget_Guard(GtkWidget* widget) : data(widget) {}
    ~Widget_Guard() {
        WaitForCleanup();
#if NFD_GTK_VERSION == 3
        gtk_widget_destroy(data);
#elif NFD_GTK_VERSION == 4
        gtk_window_destroy(GTK_WINDOW(data));
#else
        UNSUPPORTED_ERROR();
#endif
        WaitForCleanup();
    }
};

void ButtonPressedSignalHandler(
#if NFD_GTK_VERSION == 3
    GtkWidget*, GdkEvent*,
#elif NFD_GTK_VERSION == 4
    GtkGestureClick*, int, double, double,
#endif
    void* userdata) {
    ButtonClickedArgs* args = static_cast<ButtonClickedArgs*>(userdata);
    GtkFileChooser* chooser = args->chooser;
    char* currentFileName = gtk_file_chooser_get_current_name(chooser);
    if (*currentFileName) {  // string is not empty

        // find a '.' in the file name
        const char* p_period = currentFileName;
        for (; *p_period; ++p_period) {
            if (*p_period == '.') {
                break;
            }
        }

        if (!*p_period) {  // there is no '.', so append the default extension
            Pair_GtkFileFilter_FileExtension* filterMap =
                static_cast<Pair_GtkFileFilter_FileExtension*>(args->map);
            GtkFileFilter* currentFilter = gtk_file_chooser_get_filter(chooser);
            if (currentFilter) {
                for (; filterMap->filter; ++filterMap) {
                    if (filterMap->filter == currentFilter) break;
                }
            }
            if (filterMap->filter) {
                // memory for appended string (including '.' and
                // trailing '\0')
                char* appendedFileName = NFDi_Malloc<char>(
                    sizeof(char) * ((p_period - currentFileName) +
                                    (filterMap->extensionEnd - filterMap->extensionBegin) + 2));
                char* p_fileName = copy(currentFileName, p_period, appendedFileName);
                *p_fileName++ = '.';
                p_fileName = copy(filterMap->extensionBegin, filterMap->extensionEnd, p_fileName);
                *p_fileName++ = '\0';

                assert(p_fileName - appendedFileName ==
                       (p_period - currentFileName) +
                           (filterMap->extensionEnd - filterMap->extensionBegin) + 2);

                // set the appended file name
                gtk_file_chooser_set_current_name(chooser, appendedFileName);

                // free the memory
                NFDi_Free(appendedFileName);
            }
        }
    }

    // free the memory
    g_free(currentFileName);
}

#if NFD_GTK_VERSION == 4
void DialogResponseHandler(GtkDialog*, gint resp, gpointer out_resp_gp) {
    gint* out_resp = static_cast<gint*>(out_resp_gp);
    *out_resp = resp;
}
#endif

// wrapper for gtk_dialog_run() that brings the dialog to the front
// see issues at:
// https://github.com/btzy/nativefiledialog-extended/issues/31
// https://github.com/mlabbe/nativefiledialog/pull/92
// https://github.com/guillaumechereau/noc/pull/11
gint RunDialogWithFocus(GtkDialog* dialog) {
#if NFD_GTK_VERSION == 3
#if defined(GDK_WINDOWING_X11)
    gtk_widget_show_all(GTK_WIDGET(dialog));  // show the dialog so that it gets a display
    if (GDK_IS_X11_DISPLAY(gtk_widget_get_display(GTK_WIDGET(dialog)))) {
        GdkWindow* window = gtk_widget_get_window(GTK_WIDGET(dialog));
        gdk_window_set_events(
            window,
            static_cast<GdkEventMask>(gdk_window_get_events(window) | GDK_PROPERTY_CHANGE_MASK));
        gtk_window_present_with_time(GTK_WINDOW(dialog), gdk_x11_get_server_time(window));
    }
#endif
    return gtk_dialog_run(dialog);
#elif NFD_GTK_VERSION == 4
    // TODO: the X11 popup issues

    // technically we should do this, but it's no use because we don't have a GTK window to set as
    // the transient parent:
    // gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

    gint resp = 0;
    g_signal_connect(G_OBJECT(dialog),
                     "response",
                     G_CALLBACK(DialogResponseHandler),
                     static_cast<gpointer>(&resp));
    gtk_widget_show(GTK_WIDGET(dialog));
    while (resp == 0) g_main_context_iteration(NULL, TRUE);
    return resp;
#else
    UNSUPPORTED_ERROR();
#endif
}

}  // namespace

const char* NFD_GetError(void) {
    return g_errorstr;
}

void NFD_ClearError(void) {
    NFDi_SetError(nullptr);
}

/* public */

nfdresult_t NFD_Init(void) {
    // Init GTK
#if NFD_GTK_VERSION == 3
    if (!gtk_init_check(NULL, NULL))
#elif NFD_GTK_VERSION == 4
    if (!gtk_init_check())
#else
    UNSUPPORTED_ERROR();
#endif
    {
        NFDi_SetError("Failed to initialize GTK+ with gtk_init_check.");
        return NFD_ERROR;
    }
#if NFD_GTK_VERSION == 4
    // we need to give the program a name, so that GTK won't keep giving us warnings about recently
    // used resources
    if (!g_get_prgname()) {
        g_set_prgname(program_invocation_short_name);
    }
#endif
    return NFD_OKAY;
}
void NFD_Quit(void) {
    // do nothing, GTK cannot be de-initialized
}

void NFD_FreePathN(nfdnchar_t* filePath) {
    assert(filePath);
    g_free(filePath);
}

nfdresult_t NFD_OpenDialogN(nfdnchar_t** outPath,
                            const nfdnfilteritem_t* filterList,
                            nfdfiltersize_t filterCount,
                            const nfdnchar_t* defaultPath) {
    GtkWidget* widget = gtk_file_chooser_dialog_new("Open File",
                                                    nullptr,
                                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    "_Cancel",
                                                    GTK_RESPONSE_CANCEL,
                                                    "_Open",
                                                    GTK_RESPONSE_ACCEPT,
                                                    nullptr);

    // guard to destroy the widget when returning from this function
    Widget_Guard widgetGuard(widget);

    /* Build the filter list */
    AddFiltersToDialog(GTK_FILE_CHOOSER(widget), filterList, filterCount);

    /* Set the default path */
    nfdresult_t res = SetDefaultPath(GTK_FILE_CHOOSER(widget), defaultPath);
    if (res != NFD_OKAY) return res;

    if (RunDialogWithFocus(GTK_DIALOG(widget)) == GTK_RESPONSE_ACCEPT) {
        // write out the file name
        return GetSingleFileNameForOpen(widget, outPath);
    } else {
        return NFD_CANCEL;
    }
}

nfdresult_t NFD_OpenDialogMultipleN(const nfdpathset_t** outPaths,
                                    const nfdnfilteritem_t* filterList,
                                    nfdfiltersize_t filterCount,
                                    const nfdnchar_t* defaultPath) {
    GtkWidget* widget = gtk_file_chooser_dialog_new("Open Files",
                                                    nullptr,
                                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    "_Cancel",
                                                    GTK_RESPONSE_CANCEL,
                                                    "_Open",
                                                    GTK_RESPONSE_ACCEPT,
                                                    nullptr);

    // guard to destroy the widget when returning from this function
    Widget_Guard widgetGuard(widget);

    // set select multiple
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(widget), TRUE);

    /* Build the filter list */
    AddFiltersToDialog(GTK_FILE_CHOOSER(widget), filterList, filterCount);

    /* Set the default path */
    nfdresult_t res = SetDefaultPath(GTK_FILE_CHOOSER(widget), defaultPath);
    if (res != NFD_OKAY) return res;

    if (RunDialogWithFocus(GTK_DIALOG(widget)) == GTK_RESPONSE_ACCEPT) {
        // write out the file name
        return GetMultipleFileNamesForOpen(widget, outPaths);
    } else {
        return NFD_CANCEL;
    }
}

nfdresult_t NFD_SaveDialogN(nfdnchar_t** outPath,
                            const nfdnfilteritem_t* filterList,
                            nfdfiltersize_t filterCount,
                            const nfdnchar_t* defaultPath,
                            const nfdnchar_t* defaultName) {
    GtkWidget* widget = gtk_file_chooser_dialog_new("Save File",
                                                    nullptr,
                                                    GTK_FILE_CHOOSER_ACTION_SAVE,
                                                    "_Cancel",
                                                    GTK_RESPONSE_CANCEL,
                                                    nullptr);

    // guard to destroy the widget when returning from this function
    Widget_Guard widgetGuard(widget);

    GtkWidget* saveButton = gtk_dialog_add_button(GTK_DIALOG(widget), "_Save", GTK_RESPONSE_ACCEPT);

    // Prompt on overwrite (GTK3 only, because GTK4 automatically prompts)
#if NFD_GTK_VERSION == 3
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(widget), TRUE);
#endif

    /* Build the filter list */
    ButtonClickedArgs buttonClickedArgs;
    buttonClickedArgs.chooser = GTK_FILE_CHOOSER(widget);
    buttonClickedArgs.map =
        AddFiltersToDialogWithMap(GTK_FILE_CHOOSER(widget), filterList, filterCount);

    /* Set the default path */
    nfdresult_t res = SetDefaultPath(GTK_FILE_CHOOSER(widget), defaultPath);
    if (res != NFD_OKAY) return res;

    /* Set the default file name */
    SetDefaultName(GTK_FILE_CHOOSER(widget), defaultName);

#if NFD_GTK_VERSION == 3
    /* set the handler to add file extension */
    gulong handlerID = g_signal_connect(G_OBJECT(saveButton),
                                        "button-press-event",
                                        G_CALLBACK(ButtonPressedSignalHandler),
                                        static_cast<void*>(&buttonClickedArgs));
#elif NFD_GTK_VERSION == 4
    GtkGesture* gesture = gtk_gesture_click_new();
    gulong handlerID = g_signal_connect(G_OBJECT(gesture),
                                        "pressed",
                                        G_CALLBACK(ButtonPressedSignalHandler),
                                        static_cast<void*>(&buttonClickedArgs));
    gtk_widget_add_controller(saveButton,GTK_EVENT_CONTROLLER(gesture));
#endif

    /* invoke the dialog (blocks until dialog is closed) */
    gint result = RunDialogWithFocus(GTK_DIALOG(widget));

#if NFD_GTK_VERSION == 3
    /* unset the handler */
    g_signal_handler_disconnect(G_OBJECT(saveButton), handlerID);
#elif NFD_GTK_VERSION == 4
    g_signal_handler_disconnect(G_OBJECT(gesture), handlerID);
#endif

    /* free the filter map */
    NFDi_Free(buttonClickedArgs.map);

    if (result == GTK_RESPONSE_ACCEPT) {
        // write out the file name
        return GetSingleFileNameForSave(widget, outPath);
    } else {
        return NFD_CANCEL;
    }
}

nfdresult_t NFD_PickFolderN(nfdnchar_t** outPath, const nfdnchar_t* defaultPath) {
    GtkWidget* widget = gtk_file_chooser_dialog_new("Select folder",
                                                    nullptr,
                                                    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                                    "_Cancel",
                                                    GTK_RESPONSE_CANCEL,
                                                    "_Select",
                                                    GTK_RESPONSE_ACCEPT,
                                                    nullptr);

    // guard to destroy the widget when returning from this function
    Widget_Guard widgetGuard(widget);

    /* Set the default path */
    nfdresult_t res = SetDefaultPath(GTK_FILE_CHOOSER(widget), defaultPath);
    if (res != NFD_OKAY) return res;

    if (RunDialogWithFocus(GTK_DIALOG(widget)) == GTK_RESPONSE_ACCEPT) {
        // write out the file name
        // we don't support non-local files, so the behaviour is the same as the save dialog
        return GetSingleFileNameForSave(widget, outPath);
    } else {
        return NFD_CANCEL;
    }
}

#if NFD_GTK_VERSION == 3

nfdresult_t NFD_PathSet_GetCount(const nfdpathset_t* pathSet, nfdpathsetsize_t* count) {
    assert(pathSet);
    // const_cast because methods on GSList aren't const, but it should act
    // like const to the caller
    GSList* fileList = const_cast<GSList*>(static_cast<const GSList*>(pathSet));

    *count = g_slist_length(fileList);
    return NFD_OKAY;
}

nfdresult_t NFD_PathSet_GetPathN(const nfdpathset_t* pathSet,
                                 nfdpathsetsize_t index,
                                 nfdnchar_t** outPath) {
    assert(pathSet);
    // const_cast because methods on GSList aren't const, but it should act
    // like const to the caller
    GSList* fileList = const_cast<GSList*>(static_cast<const GSList*>(pathSet));

    // Note: this takes linear time... but should be good enough
    *outPath = static_cast<nfdnchar_t*>(g_slist_nth_data(fileList, index));

    return NFD_OKAY;
}

void NFD_PathSet_FreePathN(const nfdnchar_t* filePath) {
    assert(filePath);
    // no-op, because NFD_PathSet_Free does the freeing for us
}

void NFD_PathSet_Free(const nfdpathset_t* pathSet) {
    assert(pathSet);
    // const_cast because methods on GSList aren't const, but it should act
    // like const to the caller
    GSList* fileList = const_cast<GSList*>(static_cast<const GSList*>(pathSet));

    // free all the nodes
    for (GSList* node = fileList; node; node = node->next) {
        assert(node->data);
        g_free(node->data);
    }

    // free the path set memory
    g_slist_free(fileList);
}

nfdresult_t NFD_PathSet_GetEnum(const nfdpathset_t* pathSet, nfdpathsetenum_t* outEnumerator) {
    // The pathset (GSList) is already a linked list, so the enumeration is itself
    outEnumerator->ptr = const_cast<void*>(pathSet);

    return NFD_OKAY;
}

void NFD_PathSet_FreeEnum(nfdpathsetenum_t*) {
    // Do nothing, because the enumeration is the pathset itself
}

nfdresult_t NFD_PathSet_EnumNextN(nfdpathsetenum_t* enumerator, nfdnchar_t** outPath) {
    const GSList* fileList = static_cast<const GSList*>(enumerator->ptr);

    if (fileList) {
        *outPath = static_cast<nfdnchar_t*>(fileList->data);
        enumerator->ptr = static_cast<void*>(fileList->next);
    } else {
        *outPath = nullptr;
    }

    return NFD_OKAY;
}

#elif NFD_GTK_VERSION == 4

nfdresult_t NFD_PathSet_GetCount(const nfdpathset_t* pathSet, nfdpathsetsize_t* count) {
    assert(pathSet);
    // const_cast because methods on GSList aren't const, but it should act
    // like const to the caller
    GListModel* fileList = const_cast<GListModel*>(static_cast<const GListModel*>(pathSet));

    *count = g_list_model_get_n_items(fileList);
    return NFD_OKAY;
}

nfdresult_t NFD_PathSet_GetPathN(const nfdpathset_t* pathSet,
                                 nfdpathsetsize_t index,
                                 nfdnchar_t** outPath) {
    assert(pathSet);
    // const_cast because methods on GSList aren't const, but it should act
    // like const to the caller
    GListModel* fileList = const_cast<GListModel*>(static_cast<const GListModel*>(pathSet));

    return GetFileNameFromGFile(static_cast<GFile*>(g_list_model_get_item(fileList, index)),
                                outPath);
}

void NFD_PathSet_FreePathN(const nfdnchar_t* filePath) {
    assert(filePath);
    // no-op, because GetFileNameFromGFile already frees it
}

void NFD_PathSet_Free(const nfdpathset_t* pathSet) {
    assert(pathSet);
    // const_cast because methods on GSList aren't const, but it should act
    // like const to the caller
    GListModel* fileList = const_cast<GListModel*>(static_cast<const GListModel*>(pathSet));

    // free the path set memory
    g_object_unref(fileList);
}

nfdresult_t NFD_PathSet_GetEnum(const nfdpathset_t* pathSet, nfdpathsetenum_t* outEnumerator) {
    // The enumeration is the GListModel with the current index
    outEnumerator->ptr = const_cast<void*>(pathSet);
    outEnumerator->next_index = 0;

    return NFD_OKAY;
}

void NFD_PathSet_FreeEnum(nfdpathsetenum_t*) {
    // Do nothing, because we didn't allocate any memory in the enum
}

nfdresult_t NFD_PathSet_EnumNextN(nfdpathsetenum_t* enumerator, nfdnchar_t** outPath) {
    GListModel* fileList = static_cast<GListModel*>(enumerator->ptr);

    GFile* file = static_cast<GFile*>(g_list_model_get_item(fileList, enumerator->next_index));
    if (file) {
        ++enumerator->next_index;
        return GetFileNameFromGFile(file, outPath);
    } else {
        *outPath = nullptr;
        return NFD_OKAY;
    }
}

#endif