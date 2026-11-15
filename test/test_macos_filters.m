// Exercise the real AppKit accessory and its target/action without a modal loop.
// Include the backend so this test can configure panels through its private helper.
#include "../src/nfd_cocoa.m"

#define CHECK(...) do { \
    if (!(__VA_ARGS__)) { \
        fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #__VA_ARGS__); \
        exit(1); \
    } \
} while (0)

static NSPopUpButton* FindPopup(NSView* view) {
    if ([view isKindOfClass:[NSPopUpButton class]]) return (NSPopUpButton*)view;
    for (NSView* child in [view subviews]) {
        NSPopUpButton* popup = FindPopup(child);
        if (popup) return popup;
    }
    return nil;
}

static void SelectFilter(NSPopUpButton* popup, NSInteger index) {
    [popup selectItemAtIndex:index];
    CHECK([popup sendAction:[popup action] to:[popup target]]);
}

static NSArray* Extensions(NSSavePanel* panel) {
#if NFD_MACOS_ALLOWEDCONTENTTYPES
    NSMutableArray* extensions = [NSMutableArray array];
    for (UTType* type in [panel allowedContentTypes]) {
        [extensions addObject:[type preferredFilenameExtension]];
    }
    return extensions;
#else
    return [panel allowedFileTypes];
#endif
}

static void CheckPanel(NSSavePanel* panel) {
    const nfdnfilteritem_t filters[] = {
        {"Geometry files", "stl,xml,zip"},
        {"Données", "bxy,syn"},
    };
    AddFilterListToDialog(panel, filters, 2);
    NSPopUpButton* popup = FindPopup([panel accessoryView]);
    CHECK(popup != nil);
    CHECK([popup numberOfItems] == 3);
    CHECK([[popup itemTitleAtIndex:0] isEqualToString:@"Geometry files"]);
    CHECK([[popup itemTitleAtIndex:1] isEqualToString:@"Données"]);
    CHECK([[popup itemTitleAtIndex:2] isEqualToString:@"All files"]);
    CHECK([popup indexOfSelectedItem] == 0);
    CHECK([Extensions(panel) isEqualToArray:@[@"stl", @"xml", @"zip"]]);
    CHECK(![panel allowsOtherFileTypes]);
    if ([panel isKindOfClass:[NSOpenPanel class]]) {
        CHECK([(NSOpenPanel*)panel isAccessoryViewDisclosed]);
    }

    SelectFilter(popup, 1);
    CHECK([Extensions(panel) isEqualToArray:@[@"bxy", @"syn"]]);
    SelectFilter(popup, 2);
    CHECK([Extensions(panel) count] == 0);
    CHECK([panel allowsOtherFileTypes]);
    SelectFilter(popup, 0);
    CHECK([Extensions(panel) isEqualToArray:@[@"stl", @"xml", @"zip"]]);
    CHECK(![panel allowsOtherFileTypes]);

    // Reusing a panel must discard the previous selector and restrictions.
    AddFilterListToDialog(panel, NULL, 0);
    CHECK([panel accessoryView] == nil);
    CHECK([Extensions(panel) count] == 0);
    CHECK([panel allowsOtherFileTypes]);

    // Direct NFD callers may explicitly include a wildcard; do not duplicate it.
    const nfdnfilteritem_t explicitAll[] = {
        {"All files", "*"}, {"Text files", "txt"},
    };
    AddFilterListToDialog(panel, explicitAll, 2);
    popup = FindPopup([panel accessoryView]);
    CHECK([popup numberOfItems] == 2);
    CHECK([Extensions(panel) count] == 0);
    SelectFilter(popup, 1);
    CHECK([Extensions(panel) isEqualToArray:@[@"txt"]]);

    const nfdnfilteritem_t allOnly[] = {{"All files", "*"}};
    AddFilterListToDialog(panel, allOnly, 1);
    CHECK([FindPopup([panel accessoryView]) numberOfItems] == 1);
    CHECK([Extensions(panel) count] == 0);

    const nfdnfilteritem_t duplicateNames[] = {
        {"Data", "txt"}, {"Data", "csv"},
    };
    AddFilterListToDialog(panel, duplicateNames, 2);
    popup = FindPopup([panel accessoryView]);
    CHECK([popup numberOfItems] == 3);
    CHECK([Extensions(panel) isEqualToArray:@[@"txt"]]);
    SelectFilter(popup, 1);
    CHECK([Extensions(panel) isEqualToArray:@[@"csv"]]);

    AddFilterListToDialog(panel, filters, 2);
    CHECK([Extensions(panel) isEqualToArray:@[@"stl", @"xml", @"zip"]]);
    AddFilterListToDialog(panel, NULL, 0);
}

int main(void) {
    @autoreleasepool {
        CHECK(NFD_Init() == NFD_OKAY);
        NSOpenPanel* open = [NSOpenPanel openPanel];
        CheckPanel(open);
        [open setAllowsMultipleSelection:YES];
        CheckPanel(open);
        CHECK([open allowsMultipleSelection]);
        CheckPanel([NSSavePanel savePanel]);
        NFD_Quit();
    }
    puts("PASS: macOS native filter selection (open, multiple-open, save)");
    return 0;
}
