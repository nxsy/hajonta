/*
 * Lightly modified from https://github.com/gamedevtech/CocoaOpenGLWindow
 *
 * The above-mentioned project was released under public domain.
 */
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CVDisplayLink.h>
#import <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>

@interface OpenFileDialogReturn : NSObject {
@public
    NSURL *filename;
};
@end

@implementation OpenFileDialogReturn
@end

#include "hajonta/platform/common.h"

bool
openFileDialog(void *void_view, char *filename, uint32_t file_max_length, char **contents, uint32_t *size, char *file_path);

bool
openFileNearby(char *base, char *path, char **contents, uint32_t *size, char *file_path);

#include "hajonta/platform/osx.cpp"

@class View;
static CVReturn GlobalDisplayLinkCallback(CVDisplayLinkRef, const CVTimeStamp*, const CVTimeStamp*, CVOptionFlags, CVOptionFlags*, void*);

@interface View : NSOpenGLView <NSWindowDelegate> {
@public
    CVDisplayLinkRef displayLink;
    NSRect windowRect;
    NSRecursiveLock* appLock;

    bool running;
    osx_state state;
}
@end

@implementation View
// Initialize
- (id) initWithFrame: (NSRect) frame {

    osx_init(&state, frame.size.width, frame.size.height, (void *)self);
    running = true;

    // No multisampling
    uint32_t samples = 0;

    // Keep multisampling attributes at the start of the attribute lists since code below assumes they are array elements 0 through 4.
    NSOpenGLPixelFormatAttribute windowedAttrs[] = 
    {
        NSOpenGLPFAMultisample,
        NSOpenGLPFASampleBuffers, (uint32_t)(samples ? 1 : 0),
        NSOpenGLPFASamples, samples,
        NSOpenGLPFAAccelerated,
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFAColorSize, 32,
        NSOpenGLPFADepthSize, 24,
        NSOpenGLPFAAlphaSize, 8,
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
        0
    };

    // Try to choose a supported pixel format
    NSOpenGLPixelFormat* pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:windowedAttrs];

    if (!pf) {
        bool valid = false;
        while (!pf && samples > 0) {
            samples /= 2;
            windowedAttrs[2] = samples ? 1 : 0;
            windowedAttrs[4] = samples;
            pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:windowedAttrs];
            if (pf) {
                valid = true;
                break;
            }
        }

        if (!valid) {
            NSLog(@"OpenGL pixel format not supported.");
            return nil;
        }
    }

    self = [super initWithFrame:frame pixelFormat:[pf autorelease]];
    appLock = [[NSRecursiveLock alloc] init];

    return self;
}

- (void) prepareOpenGL {
    [super prepareOpenGL];

    [[self window] setLevel: NSNormalWindowLevel];
    [[self window] makeKeyAndOrderFront: self];

    // Make all the OpenGL calls to setup rendering and build the necessary rendering objects
    [[self openGLContext] makeCurrentContext];
    // Synchronize buffer swaps with vertical refresh rate
    GLint swapInt = 1; // Vsynch on!
    [[self openGLContext] setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];

    // Create a display link capable of being used with all active displays
    CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);

    // Set the renderer output callback function
    CVDisplayLinkSetOutputCallback(displayLink, &GlobalDisplayLinkCallback, self);

    CGLContextObj cglContext = (CGLContextObj)[[self openGLContext] CGLContextObj];
    CGLPixelFormatObj cglPixelFormat = (CGLPixelFormatObj)[[self pixelFormat] CGLPixelFormatObj];
    CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(displayLink, cglContext, cglPixelFormat);

    GLint dim[2] = {(int32_t)windowRect.size.width, (int32_t)windowRect.size.height};
    CGLSetParameter(cglContext, kCGLCPSurfaceBackingSize, dim);
    CGLEnable(cglContext, kCGLCESurfaceBackingSize);

    [appLock lock];
    CGLLockContext((CGLContextObj)[[self openGLContext] CGLContextObj]);
    NSLog(@"Initialize");

    NSLog(@"GL version:   %s", glGetString(GL_VERSION));
    NSLog(@"GLSL version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

    glViewport(0, 0, windowRect.size.width, windowRect.size.height);

    CGLUnlockContext((CGLContextObj)[[self openGLContext] CGLContextObj]); 
    [appLock unlock];

    // Activate the display link
    CVDisplayLinkStart(displayLink);
}

// Tell the window to accept input events
- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)mouseMoved:(NSEvent*) event {
    [appLock lock];
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    // NSLog(@"Mouse pos: %lf, %lf", point.x, point.y);
    if (point.x < 0)
    {
        point.x = 0;
    }
    state.new_input->mouse.x = point.x;
    state.new_input->mouse.y = state.window_height - point.y;
    [appLock unlock];
}

- (void) mouseDragged: (NSEvent*) event {
    [appLock lock];
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    NSLog(@"Mouse pos: %lf, %lf", point.x, point.y);
    [appLock unlock];
}

- (void)scrollWheel: (NSEvent*) event  {
    [appLock lock];
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    NSLog(@"Mouse wheel at: %lf, %lf. Delta: %lf", point.x, point.y, [event deltaY]);
    [appLock unlock];
}

- (void) mouseDown: (NSEvent*) event {
    [appLock lock];
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    NSLog(@"Left mouse down: %lf, %lf", point.x, point.y);
    osx_process_mouse_press(&state, true);
    [appLock unlock];
}

- (void) mouseUp: (NSEvent*) event {
    [appLock lock];
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    NSLog(@"Left mouse up: %lf, %lf", point.x, point.y);
    osx_process_mouse_press(&state, false);
    [appLock unlock];
}

- (void) rightMouseDown: (NSEvent*) event {
    [appLock lock];
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    NSLog(@"Right mouse down: %lf, %lf", point.x, point.y);
    [appLock unlock];
}

- (void) rightMouseUp: (NSEvent*) event {
    [appLock lock];
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    NSLog(@"Right mouse up: %lf, %lf", point.x, point.y);
    [appLock unlock];
}

- (void)otherMouseDown: (NSEvent*) event {
    [appLock lock];
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    NSLog(@"Middle mouse down: %lf, %lf", point.x, point.y);
    [appLock unlock];
}

- (void)otherMouseUp: (NSEvent*) event {
    [appLock lock];
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    NSLog(@"Middle mouse up: %lf, %lf", point.x, point.y);
    [appLock unlock];
}

- (void) mouseEntered: (NSEvent*)event {
    [appLock lock];
    NSLog(@"Mouse entered");
    [appLock unlock];
}

- (void) mouseExited: (NSEvent*)event {
    [appLock lock];
    NSLog(@"Mouse left");
    [appLock unlock];
}

- (void) keyDown: (NSEvent*) event {
    [appLock lock];
    if ([event isARepeat] == NO) {
        //NSLog(@"Key down: %d", [event keyCode]);
        if (!state.keyboard_mode) {
            osx_process_key_event(&state, [event keyCode], true);
        }
    }
    [appLock unlock];
}

- (void) keyUp: (NSEvent*) event {
    [appLock lock];
    //NSLog(@"Key up: %d", [event keyCode]);
    if (!state.keyboard_mode) {
        osx_process_key_event(&state, [event keyCode], false);
    }
    else
    {
        NSString *characters = [event characters];
        unichar c = [characters characterAtIndex:0];
        osx_process_character(&state, c);
    }
    [appLock unlock];
}

// Update
- (CVReturn) getFrameForTime:(const CVTimeStamp*)outputTime {
    [appLock lock];

    [[self openGLContext] makeCurrentContext];
    CGLLockContext((CGLContextObj)[[self openGLContext] CGLContextObj]);

    //NSLog(@"Update");
    // Temp
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    [appLock unlock];
    loop_cycle(&state);
    [appLock lock];

    // EndTemp

    CGLFlushDrawable((CGLContextObj)[[self openGLContext] CGLContextObj]);
    CGLUnlockContext((CGLContextObj)[[self openGLContext] CGLContextObj]); 

    if (state.stopping) {
        [NSApp terminate:self];
    }

    [appLock unlock];

    return kCVReturnSuccess;
}

// Resize
- (void)windowDidResize:(NSNotification*)notification {
    [appLock lock];
    NSSize size = [ [ _window contentView ] frame ].size;
    NSLog(@"Window resize: %lf, %lf", size.width, size.height);
    state.window_width = size.width;
    state.window_height = size.height;
    [appLock unlock];
}

- (void)resumeDisplayRenderer  {
    [appLock lock];
    CVDisplayLinkStop(displayLink);
    [appLock unlock]; 
}

- (void)haltDisplayRenderer  {
    [appLock lock];
    CVDisplayLinkStop(displayLink);
    [appLock unlock];
}

// Terminate window when the red X is pressed
-(void)windowWillClose:(NSNotification *)notification {
    if (running) {
        running = false;

        [appLock lock];
        NSLog(@"Cleanup");

        CVDisplayLinkStop(displayLink);
        CVDisplayLinkRelease(displayLink);

        [appLock unlock];
    }

    [NSApp terminate:self];
}

// Cleanup
- (void) dealloc {  
    [appLock release]; 
    [super dealloc];
}

- (bool) openFileDialog:(OpenFileDialogReturn *)ret {
    [appLock lock];
    bool result = false;
    NSOpenPanel *panel = [NSOpenPanel openPanel];
    [panel makeKeyAndOrderFront:self];
    int panel_run_result = [panel runModal];
    if (panel_run_result == NSFileHandlingPanelOKButton)
    {
        NSLog(@"Got NSFileHandlingPanelOKButton");
        NSArray* urls = [panel URLs];
        for(int i = 0; i < [urls count]; i++ )
        {
            NSURL *url = [urls objectAtIndex:i];
            ret->filename = url;
        }
    }
    [appLock unlock];
    return true;
}
@end

bool
openFileDialog(void *void_view, char *filename, uint32_t file_max_length, char **contents, uint32_t *size, char *file_path)
{
    View *view = (View *)void_view;
    OpenFileDialogReturn *fdr = [OpenFileDialogReturn alloc];
    NSLog(@"%@", fdr->filename);

    [view
        performSelectorOnMainThread:@selector(openFileDialog:) withObject:fdr waitUntilDone:YES];

    if (!fdr->filename)
    {
        return false;
    }

    NSLog(@"%@", fdr->filename);

    NSString *filename_s = [fdr->filename absoluteString];

    strcpy(file_path, [filename_s UTF8String]);
    NSData *data = [[NSData alloc] initWithContentsOfURL:fdr->filename];

    *size = [data length];
    char *file_contents = (char *)malloc(*size);
    [data getBytes:file_contents length:*size];

    *contents = file_contents;

    return true;
}

bool
openFileNearby(char *base, char *path, char **contents, uint32_t *size, char *file_path)
{
    NSString *base_string = [NSString stringWithCString:base encoding:NSUTF8StringEncoding];
    NSURL *base_url = [NSURL URLWithString:base_string];
    NSString *path_string = [NSString stringWithCString:path encoding:NSUTF8StringEncoding];
    NSURL *url = [NSURL URLWithString:path_string relativeToURL:base_url];

    NSString *url_s = [url absoluteString];
    strcpy(file_path, [url_s UTF8String]);

    NSData *data = [[NSData alloc] initWithContentsOfURL:url];

    *size = [data length];
    char *file_contents = (char *)malloc(*size);
    [data getBytes:file_contents length:*size];

    *contents = file_contents;
    return true;
}

static CVReturn GlobalDisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now, const CVTimeStamp* outputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext) {
    CVReturn result = [(View*)displayLinkContext getFrameForTime:outputTime];
    return result;
}

int main(int argc, const char * argv[])  { 
    // Autorelease Pool: 
    // Objects declared in this scope will be automatically 
    // released at the end of it, when the pool is "drained". 
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init]; 

    // Create a shared app instance. 
    // This will initialize the global variable 
    // 'NSApp' with the application instance. 
    [NSApplication sharedApplication]; 

    // Create a window: 

    // Style flags 
    NSUInteger windowStyle = NSTitledWindowMask  | NSClosableWindowMask | NSResizableWindowMask | NSMiniaturizableWindowMask; 

    // Window bounds (x, y, width, height) 
    NSRect screenRect = [[NSScreen mainScreen] frame];
    NSRect viewRect = NSMakeRect(0, 0, 960, 540);
    NSRect windowRect = NSMakeRect(NSMidX(screenRect) - NSMidX(viewRect),
                                 NSMidY(screenRect) - NSMidY(viewRect),
                                 viewRect.size.width, 
                                 viewRect.size.height);

    NSWindow * window = [[NSWindow alloc] initWithContentRect:windowRect 
                        styleMask:windowStyle 
                        backing:NSBackingStoreBuffered 
                        defer:NO]; 
    [window autorelease]; 

    // Window controller 
    NSWindowController * windowController = [[NSWindowController alloc] initWithWindow:window]; 
    [windowController autorelease]; 

    // Since Snow Leopard, programs without application bundles and Info.plist files don't get a menubar 
    // and can't be brought to the front unless the presentation option is changed
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    // Next, we need to create the menu bar. You don't need to give the first item in the menubar a name 
    // (it will get the application's name automatically)
    id menubar = [[NSMenu new] autorelease];
    id appMenuItem = [[NSMenuItem new] autorelease];
    [menubar addItem:appMenuItem];
    [NSApp setMainMenu:menubar];

    // Then we add the quit item to the menu. Fortunately the action is simple since terminate: is 
    // already implemented in NSApplication and the NSApplication is always in the responder chain.
    id appMenu = [[NSMenu new] autorelease];
    id appName = [[NSProcessInfo processInfo] processName];
    id quitTitle = [@"Quit " stringByAppendingString:appName];
    id quitMenuItem = [[[NSMenuItem alloc] initWithTitle:quitTitle
        action:@selector(terminate:) keyEquivalent:@"q"] autorelease];
    [appMenu addItem:quitMenuItem];
    [appMenuItem setSubmenu:appMenu];

    // Create app delegate to handle system events
    View* view = [[[View alloc] initWithFrame:windowRect] autorelease];
    view->windowRect = windowRect;
    [window setAcceptsMouseMovedEvents:YES];
    [window setContentView:view];
    [window setDelegate:view];

    // Set app title
    [window setTitle:appName];

    // Add fullscreen button
    [window setCollectionBehavior: NSWindowCollectionBehaviorFullScreenPrimary];

    // Show window and run event loop 
    [window orderFrontRegardless]; 
    [NSApp run]; 

    [pool drain]; 

    return (0); 
}
