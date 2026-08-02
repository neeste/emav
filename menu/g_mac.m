#import <Cocoa/Cocoa.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "menu.h"
#include "fk.h"

int bx, by, bw, bh, xpix, ypix, ytop, ybot, xmax, ymax;
int mxpos, mypos;
static char pgm_title[80] = "EMAV";

static struct {
    short int r, g, b;
} ct[16] = {
    {0, 0, 0},           /* black      */
    {0, 0, 170},         /* blue       */
    {0, 170, 0},         /* green      */
    {0, 170, 170},       /* cyan       */
    {170, 0, 0},         /* red        */
    {170, 0, 170},       /* magenta    */
    {170, 85, 0},        /* brown      */
    {170, 170, 170},     /* lt grey    */
    {85, 85, 85},        /* dk grey    */
    {85, 85, 255},       /* lt blue    */
    {85, 255, 85},       /* lt green   */
    {85, 255, 255},      /* lt cyan    */
    {255, 85, 85},       /* lt red     */
    {255, 85, 255},      /* lt magenta */
    {255, 255, 85},      /* yellow     */
    {255, 255, 255},     /* white      */
};

static int txtbgc = 0; 
static int txtfgc = 15;
static unsigned int xw_width = 1024;
static unsigned int xw_height = 768;
static int font_size = 18;

// Thread-safe event queue
#define MAX_EVENTS 1024
static int event_queue[MAX_EVENTS];
static int event_head = 0;
static int event_tail = 0;
static pthread_mutex_t event_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t event_cond = PTHREAD_COND_INITIALIZER;

static void push_event(int evt) {
    pthread_mutex_lock(&event_mutex);
    int next = (event_tail + 1) % MAX_EVENTS;
    if (next != event_head) {
        event_queue[event_tail] = evt;
        event_tail = next;
        pthread_cond_signal(&event_cond);
    }
    pthread_mutex_unlock(&event_mutex);
}

static int pop_event(int block) {
    int evt = 0;
    pthread_mutex_lock(&event_mutex);
    while (event_head == event_tail) {
        if (!block) {
            pthread_mutex_unlock(&event_mutex);
            return 0;
        }
        pthread_cond_wait(&event_cond, &event_mutex);
    }
    evt = event_queue[event_head];
    event_head = (event_head + 1) % MAX_EVENTS;
    pthread_mutex_unlock(&event_mutex);
    return evt;
}

static int has_event() {
    int has = 0;
    pthread_mutex_lock(&event_mutex);
    has = (event_head != event_tail);
    pthread_mutex_unlock(&event_mutex);
    return has;
}

// Emulate graphics via offscreen bitmap
static CGContextRef offscreenContext = NULL;
static NSWindow *mainWindow = nil;

@interface EMAVView : NSView
@end

@implementation EMAVView

- (BOOL)acceptsFirstResponder { return YES; }

- (void)drawRect:(NSRect)dirtyRect {
    if (offscreenContext) {
        CGImageRef image = CGBitmapContextCreateImage(offscreenContext);
        if (image) {
            CGContextRef ctx = [[NSGraphicsContext currentContext] CGContext];
            CGContextDrawImage(ctx, NSRectToCGRect(self.bounds), image);
            CGImageRelease(image);
        }
    }
}

- (void)keyDown:(NSEvent *)event {
    NSString *chars = [event charactersIgnoringModifiers];
    if ([chars length] > 0) {
        unichar c = [chars characterAtIndex:0];
        // Handle special keys mapping to DOS keys
        if (c == NSUpArrowFunctionKey) push_event(FK_Up_Arrow);
        else if (c == NSDownArrowFunctionKey) push_event(FK_Down_Arrow);
        else if (c == NSLeftArrowFunctionKey) push_event(FK_Left_Arrow);
        else if (c == NSRightArrowFunctionKey) push_event(FK_Right_Arrow);
        else if (c == NSPageUpFunctionKey) push_event(FK_PgUp);
        else if (c == NSPageDownFunctionKey) push_event(FK_PgDn);
        else if (c == NSHomeFunctionKey) push_event(FK_Home);
        else if (c == NSEndFunctionKey) push_event(FK_End);
        else if (c == NSDeleteFunctionKey || c == NSBackspaceCharacter) push_event(8);
        else if (c == NSCarriageReturnCharacter || c == NSEnterCharacter) push_event(13);
        else if (c == 27) push_event(Esc);
        else if (c < 128) push_event(c);
    }
}

- (void)mouseDown:(NSEvent *)event {
    NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil];
    mxpos = (int)p.x;
    mypos = xw_height - (int)p.y;
    push_event(LEFT_CLICK);
}

@end

// ----------------------------------------------------
// Public C API
// ----------------------------------------------------

void set_title(char *s) {
    strncpy(pgm_title, s, 79);
    dispatch_async(dispatch_get_main_queue(), ^{
        if (mainWindow) {
            [mainWindow setTitle:[NSString stringWithUTF8String:pgm_title]];
        }
    });
}

int xw_kbhit(void) {
    return has_event();
}

int xw_getch(void) {
    return pop_event(1);
}

static void set_cg_color(CGContextRef ctx, int c) {
    if (c < 0 || c > 15) c = 0;
    CGFloat r = ct[c].r / 255.0;
    CGFloat g = ct[c].g / 255.0;
    CGFloat b = ct[c].b / 255.0;
    CGContextSetRGBFillColor(ctx, r, g, b, 1.0);
    CGContextSetRGBStrokeColor(ctx, r, g, b, 1.0);
}

void init_gr() {
    dispatch_sync(dispatch_get_main_queue(), ^{
        if (mainWindow) return;

        NSRect frame = NSMakeRect(0, 0, xw_width, xw_height);
        mainWindow = [[NSWindow alloc] initWithContentRect:frame
                                                 styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable)
                                                   backing:NSBackingStoreBuffered
                                                     defer:NO];
        [mainWindow setTitle:[NSString stringWithUTF8String:pgm_title]];
        [mainWindow center];
        
        EMAVView *view = [[EMAVView alloc] initWithFrame:frame];
        [mainWindow setContentView:view];
        [mainWindow makeKeyAndOrderFront:nil];
        [mainWindow makeFirstResponder:view];
        
        // Setup offscreen context
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        offscreenContext = CGBitmapContextCreate(NULL, xw_width, xw_height, 8, xw_width * 4, colorSpace, (CGBitmapInfo)kCGImageAlphaPremultipliedLast);
        CGColorSpaceRelease(colorSpace);
        
        // Clear to black
        CGContextSetRGBFillColor(offscreenContext, 0, 0, 0, 1);
        CGContextFillRect(offscreenContext, CGRectMake(0, 0, xw_width, xw_height));
        
        xmax = xw_width;
        ymax = xw_height;
        xpix = xw_width;
        ypix = xw_height;

        txtpar.font_width = 11;
        txtpar.font_height = 20;
        txtpar.menu_height = 24;
    });
}

void end_gr() {
    dispatch_sync(dispatch_get_main_queue(), ^{
        if (offscreenContext) {
            CGContextRelease(offscreenContext);
            offscreenContext = NULL;
        }
        if (mainWindow) {
            [mainWindow close];
            mainWindow = nil;
        }
    });
}

static void redraw_view() {
    dispatch_async(dispatch_get_main_queue(), ^{
        [[mainWindow contentView] setNeedsDisplay:YES];
    });
}

void gr_clear(int c) {
    dispatch_sync(dispatch_get_main_queue(), ^{
        if (offscreenContext) {
            set_cg_color(offscreenContext, c);
            CGContextFillRect(offscreenContext, CGRectMake(0, 0, xw_width, xw_height));
        }
    });
    redraw_view();
}

void gr_recto(int x1, int y1, int x2, int y2, int c) {
    dispatch_sync(dispatch_get_main_queue(), ^{
        if (offscreenContext) {
            set_cg_color(offscreenContext, c);
            int minx = x1 < x2 ? x1 : x2;
            int maxx = x1 > x2 ? x1 : x2;
            int miny = y1 < y2 ? y1 : y2;
            int maxy = y1 > y2 ? y1 : y2;
            CGRect rect = CGRectMake(minx, ymax - maxy, maxx - minx, maxy - miny);
            CGContextStrokeRect(offscreenContext, rect);
        }
    });
    redraw_view();
}

void gr_rectf(int x1, int y1, int x2, int y2, int c) {
    dispatch_sync(dispatch_get_main_queue(), ^{
        if (offscreenContext) {
            set_cg_color(offscreenContext, c);
            int minx = x1 < x2 ? x1 : x2;
            int maxx = x1 > x2 ? x1 : x2;
            int miny = y1 < y2 ? y1 : y2;
            int maxy = y1 > y2 ? y1 : y2;
            CGRect rect = CGRectMake(minx, ymax - maxy, maxx - minx, maxy - miny);
            CGContextFillRect(offscreenContext, rect);
        }
    });
    redraw_view();
}

void gr_line(int x1, int y1, int x2, int y2, int c) {
    dispatch_sync(dispatch_get_main_queue(), ^{
        if (offscreenContext) {
            set_cg_color(offscreenContext, c);
            CGContextBeginPath(offscreenContext);
            CGContextMoveToPoint(offscreenContext, x1, ymax - y1);
            CGContextAddLineToPoint(offscreenContext, x2, ymax - y2);
            CGContextStrokePath(offscreenContext);
        }
    });
    redraw_view();
}

void gr_setpix(int x, int y, int c) {
    gr_rectf(x, y, x+1, y+1, c);
}

void gr_text(int x, int y, char *s) {
    if (!s || strlen(s) == 0) return;
    gr_rectf(x, y, x + (int)strlen(s) * txtpar.font_width + 4, y - txtpar.font_height + 1, txtbgc);

    dispatch_sync(dispatch_get_main_queue(), ^{
        if (offscreenContext) {
            // Very basic text rendering
            // CoreText should be used here, but for simplicity:
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
            CGContextSelectFont(offscreenContext, "Monaco", font_size, kCGEncodingMacRoman);
            set_cg_color(offscreenContext, txtfgc);
            CGContextSetTextDrawingMode(offscreenContext, kCGTextFill);
            // y is the bottom in original coords. y - 3 is the text baseline.
            CGContextShowTextAtPoint(offscreenContext, x + 1, ymax - (y - 4), s, strlen(s));
#pragma clang diagnostic pop
        }
    });
    redraw_view();
}

void gr_scrsiz(int *x, int *y) {
    *x = xw_width;
    *y = xw_height;
}

void gr_settc(int fgc, int bgc) {
    txtfgc = fgc;
    txtbgc = bgc;
}

void gr_beep() {
    NSBeep();
}

void toggle_mono() {}
void gr_savpix() {}

#define MAX_SAVED_SCREENS 10
static CGImageRef saved_screens[MAX_SAVED_SCREENS];
static int saved_screen_idx = 0;

void g_savscr(int x1, int y1, int x2, int y2) {
    dispatch_sync(dispatch_get_main_queue(), ^{
        if (offscreenContext && saved_screen_idx < MAX_SAVED_SCREENS) {
            saved_screens[saved_screen_idx++] = CGBitmapContextCreateImage(offscreenContext);
        }
    });
}

void g_rstscr(int x1, int y1) {
    dispatch_sync(dispatch_get_main_queue(), ^{
        if (offscreenContext && saved_screen_idx > 0) {
            CGImageRef img = saved_screens[--saved_screen_idx];
            if (img) {
                CGContextSaveGState(offscreenContext);
                CGContextSetBlendMode(offscreenContext, kCGBlendModeCopy);
                CGContextDrawImage(offscreenContext, CGRectMake(0, 0, xw_width, xw_height), img);
                CGContextRestoreGState(offscreenContext);
                CGImageRelease(img);
            }
        }
    });
    redraw_view();
}
int gr_getmsb(int x, int y) { return 0; }
int gr_getpix(int x, int y) { return 0; }
void gr_remap_palette(int n, int c) {}
void gr_setfillmask(unsigned char *m) {}
void gr_dotty(int t) {}
void msleep(int msec) { usleep(msec * 1000); }

// Missing from some headers but referenced
void set_fg_color(int c) {}
int get_color(int p) { return ct[p].r; }

// ----------------------------------------------------
// Application Entry Point
// ----------------------------------------------------
extern int pgm_main(int argc, char **argv);

@interface EMAVAppDelegate : NSObject <NSApplicationDelegate>
@property (assign) int argc;
@property (assign) char **argv;
@end

@implementation EMAVAppDelegate
- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    [NSThread detachNewThreadSelector:@selector(runProgram) toTarget:self withObject:nil];
}

- (void)runProgram {
    pgm_main(self.argc, self.argv);
    // When the C program finishes, terminate the app
    dispatch_async(dispatch_get_main_queue(), ^{
        [NSApp terminate:nil];
    });
}
@end

int main(int argc, char **argv) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        EMAVAppDelegate *delegate = [[EMAVAppDelegate alloc] init];
        delegate.argc = argc;
        delegate.argv = argv;
        [app setDelegate:delegate];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        [app run];
    }
    return 0;
}
