#include <stdio.h>
#include <stdlib.h> // for exit()

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xft/Xft.h>
#include <X11/XKBlib.h>
#include <string.h>

/* Global Vars */
static int screen;
static Display *display;
static Window window, root, parentwindow;
static XIC xic;
static int w, h;

/* Functions */
static void setup(void);
static void run(void);
static void keypress(XKeyEvent *);
static void drawtray(void);
static void cleanup(void);

typedef struct {
    unsigned int w, h;
    Display *dpy;
    Window win;
    GC gc;
} Draw;

static Draw *drw;

static void
cleanup(void)
{
    XUngrabKey(display, AnyKey, AnyModifier, root);
    XCloseDisplay(display);
}

static void
drawtray(void)
{
    GC gc = XCreateGC(display, window, 0, NULL); 

    int recWidth = w*3/5;
    int recHeight = h/10;

    const char* labels[5] =
    {
        "Shutdown",
        "Restart",
        "Logout",
        "Sleep",
        "Cancel"
    }; 


    int recX = (w-recWidth) / 2;
    XSetForeground(display, gc, WhitePixel(display, screen));
    for (int i = 0; i < 5; i++) {
        // This is awful
        int gap = ((h - (((h/8)-1)*2)) - ( 5 * recHeight)) / 4;
        int recY = (h/8) + (i * ( recHeight + gap));
        printf("%d\n", gap);
        printf("%d\n", recX);
        XDrawRectangle(display, window, gc, recX, recY, recWidth, recHeight);
        XDrawString(display, window, gc, recX+35, recY+20, labels[i], strlen(labels[i]));
    }
    // XDrawArc(display, window, gc, 100, 70,  100, 100, 0, 360*64);
    // XDrawArc(display, window, gc, 100, 190, 100, 100, 0, 360*64);
    // XDrawArc(display, window, gc, 100, 310, 100, 100, 0, 360*64);
}


static void
keypress(XKeyEvent *ev)
{
    char buf[64];
    KeySym ksym = NoSymbol;
    Status status;

    XmbLookupString(xic, ev, buf, sizeof buf, &ksym, &status);

    switch(ksym) {
        case XK_Escape: 
            cleanup();
            exit(1); 
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
                keypress(&ev.xkey);
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
setup(void)
{
    XSetWindowAttributes swa;
    XIM xim;
    XClassHint ch = {"xtray", "xtray"};

    // drw->w = 300;
    // drw->h = 500;

    XWindowAttributes pwa; 
    XGetWindowAttributes(display, parentwindow, &pwa);

    w = 175;
    h = 300;

    int borderWidth = 1;
    int leftPadding = 10;
    int topPadding  = 40;

    // Create Xwindow */
    swa.override_redirect = True;
	swa.background_pixel = BlackPixel(display, screen);
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
    XSelectInput(display, window, FocusChangeMask | SubstructureNotifyMask | KeyPressMask);

    XGrabKeyboard(display, root, True, GrabModeAsync, GrabModeAsync, CurrentTime);


    drawtray();
}

int main() {
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
