/*
    2024 - Matthew Archer <matthewarcherr@gmail.com>
    See LICENSE file for copywrite and license details
*/

#include <stdio.h>
#include <stdlib.h> // for exit() and system()

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <string.h>
#include <time.h> // for nanosleep()

#include "config.h"

/* Global Vars */
static int screen;
static Display *display;
static Window window, root, parentwindow;
static XIC xic;
static GC gc;
static XFontStruct *font;
static int selection = -1;

static int w = window_width;
static int h = window_height;

static int labelsCount = sizeof(labels)/sizeof(labels[0]);

/* Functions */
static void setup(void);
static void tryGrabKeyboard(void);
static void run(void);
static void keyPress(XKeyEvent *);
static void moveMouse(XMotionEvent *);
static void drawRectangles(int, int);
static void drawTray(void);
static void setSelection(int);
static void pickSelection(void);
static void cleanup(void);

static void
cleanup(void)
{
    XUngrabKey(display, AnyKey, AnyModifier, root);
    XCloseDisplay(display);
}

static void
pickSelection(void)
{
    switch (selection) {
        case -1:
            break;
        case 0:
            system("sudo shutdown now"); 
            break;
        case 1:
            system("sudo reboot"); 
            break;
        case 2:
            system("kill -TERM $(pidof dwm)"); 
            break;
        case 3:
            system("dunstify 'sleep' "); 
            break;
        case 4:
            cleanup();
            exit(1);
            break;
    }

}

static void
setSelection(int slc)
{
    int recWidth = w*3/5;
    int recHeight = h/10;
    
    if (slc == -2) {
        selection = labelsCount-1;
    } else if (slc >= labelsCount || slc == -1) {
        drawRectangles(recWidth, recHeight);
        selection = -1;
        return;
    } else {
        selection = slc;
    }
    
    
    drawRectangles(recWidth, recHeight);
    XSetForeground(display, gc, sel_colour);
    
    
    int recX = (w-recWidth) / 2;
    int gap = ((h - (((h/8)-1)*2)) - ( labelsCount * recHeight)) / 4;
    int recY = (h/8) + (selection * ( recHeight + gap));
    XDrawRectangle(display, window, gc, recX, recY, recWidth, recHeight);
    
    // Calculate the starting position of the text to be centered
    int tx = recX + (recWidth - XTextWidth(font, labels[selection], strlen(labels[selection]))) / 2;
    int ty = recY + (recHeight + (font->ascent + font->descent)) / 2 - font->descent;
    
    XDrawString(display, window, gc, tx, ty, labels[selection], strlen(labels[selection]));
    
}

static void
drawTray(void)
{
    gc = XCreateGC(display, window, 0, NULL); 
    
    // Load font
    font = XLoadQueryFont(display, "fixed");
    if (!font) {
        fprintf(stderr, "Unable to load font\n");
        cleanup(); exit(1);
    }
    XSetFont(display, gc, font->fid);
    
    int recWidth = w*3/5;
    int recHeight = h/10;
    
    drawRectangles(recWidth, recHeight);
}

static void
drawRectangles(int recWidth, int recHeight)
{
    int recX = (w-recWidth) / 2;
    for (int i = 0; i < labelsCount; i++) {
        // This is awful
        int gap = ((h - (((h/8)-1)*2)) - ( labelsCount * recHeight)) / 4;
        int recY = (h/8) + (i * ( recHeight + gap));
        XDrawRectangle(display, window, gc, recX, recY, recWidth, recHeight);
        
        XSetForeground(display, gc, btn_colour);
        XFillRectangle(display, window, gc, recX, recY, recWidth+1, recHeight+1);
        XSetForeground(display, gc, 0xFFFFFF);
        
        // Calculate the starting position of the text
        int tx = recX + (recWidth - XTextWidth(font, labels[i], strlen(labels[i]))) / 2;
        int ty = recY + (recHeight + (font->ascent + font->descent)) / 2 - font->descent;
        
        XDrawString(display, window, gc, tx, ty, labels[i], strlen(labels[i]));
    }
}

static void
moveMouse(XMotionEvent *xme)
{
    int mx = xme->x;
    int my = xme->y;
    
    int recWidth = w*3/5;
    int recHeight = h/10;
    
    int recX = (w-recWidth) / 2;
    
    for (int i = 0; i < labelsCount; i++) {
        int gap = ((h - (((h/8)-1)*2)) - (labelsCount * recHeight)) / 4;
        int recY = (h/8) + (i * ( recHeight + gap));
        if (mx >= recX && mx <= recX + recWidth &&
            my >= recY && my <= recY + recHeight) {
                setSelection(i);
                return;
            }
    }
    setSelection(-1);
}


static void
keyPress(XKeyEvent *ev)
{
    char buf[64];
    KeySym ksym = NoSymbol;
    Status status;
    
    XmbLookupString(xic, ev, buf, sizeof buf, &ksym, &status);
    
    switch(ksym) {
        case XK_Escape: 
            cleanup();
            exit(1); 
        case XK_Tab:
            setSelection(selection+1); break;
        case XK_Down:
            setSelection(selection+1); break;
        case XK_Up:
            setSelection(selection-1); break;
        case XK_Return:
            pickSelection(); break;
        default: break;
    }
}


static void 
run(void)
{
    XEvent ev;
    
    /* Main event loop */
    while (!XNextEvent(display, &ev)) {
        if (XFilterEvent(&ev, window))
            continue;
        switch(ev.type) {
            case KeyPress:
                keyPress(&ev.xkey);
                break;
            case MotionNotify:
                moveMouse(&ev.xmotion);
                break;
            case ButtonPress:
                pickSelection();
                break;
            case DestroyNotify:
                if (ev.xdestroywindow.window != window)
                    break;
                cleanup();
                exit(1);
        }
    
    }

}

static void
tryGrabKeyboard(void)
{
    struct timespec req, rem;
    req.tv_sec = 0;
    req.tv_nsec = 1000000;
    
    for (int i = 0; i < 1000; i++) {
        if(XGrabKeyboard(display, root, True, GrabModeAsync, 
                        GrabModeAsync, CurrentTime) == GrabSuccess)
                        return;
        nanosleep(&req, &rem);
    }
    fprintf(stderr, "Unable to grab keyboard\n");
    cleanup(); exit(1);
}

static void 
setup(void)
{
    XSetWindowAttributes swa;
    XIM xim;
    XClassHint ch = {"xtray", "xtray"};
    
    XWindowAttributes pwa; 
    XGetWindowAttributes(display, parentwindow, &pwa);
    
    // Create Xwindow */
    swa.override_redirect = True;
	swa.background_pixel = bg_colour;
	swa.event_mask = ExposureMask | KeyPressMask | VisibilityChangeMask;
    swa.border_pixel = WhitePixel(display, screen);
    window = XCreateWindow(display, root, pwa.width-(w+leftPadding+(borderWidth*2)), topPadding, w, h, borderWidth,
                            CopyFromParent, CopyFromParent, CopyFromParent,
                            CWOverrideRedirect | CWBackPixel | CWEventMask | CWBorderPixel, &swa);
    XSetClassHint(display, window, &ch);
   	
   	
   	/* input methods */
	if ((xim = XOpenIM(display, NULL, NULL, NULL)) == NULL) {
        cleanup(); exit(0);
    }
	xic = XCreateIC(xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
	                 XNClientWindow, window, XNFocusWindow, window, NULL);
    
    
    XMapRaised(display, window); // Maps the window and raises it to the top of the stack
    XSelectInput(display, window, FocusChangeMask | SubstructureNotifyMask | KeyPressMask | PointerMotionMask | ButtonPressMask);
    
    tryGrabKeyboard();
    
    drawTray();
}

int 
main() 
{
    // Open connection to X server
    if (!(display = XOpenDisplay(NULL)))
        exit(1);
    
    screen = DefaultScreen(display);
    root = RootWindow(display, screen);
    
    parentwindow = root;
    
    setup();
    run();
    
    // Close connection to X server
    XCloseDisplay(display);
    return 0;
}
