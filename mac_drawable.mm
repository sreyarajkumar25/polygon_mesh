#if defined(__APPLE__)

#import <AppKit/AppKit.h>
#import <OpenGL/OpenGL.h>

void macDrawablePixels(int* width, int* height) {
    *width = 0;
    *height = 0;
    NSOpenGLContext* ctx = [NSOpenGLContext currentContext];
    if (!ctx)
        return;
    NSView* view = [ctx view];
    if (!view)
        return;
    NSRect backing = [view convertRectToBacking:[view bounds]];
    *width = (int)backing.size.width;
    *height = (int)backing.size.height;
}

#endif
