#include <stdio.h>
#include <stdlib.h> // for exit() and system()

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
static GC gc;
static XFontStruct* font;
static int w, h;
static int selection = -1;

/* Functions */
static void setup(void);
static void run(void);
static void keypress(XKeyEvent *);
static void drawtray(void);
static void drawRectangles(int, int);
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
select(void)
{
    switch (selection) {
        case -1:
            break;
        case 0:
            system("dunstify 'shutdown' "); 
            break;
        case 1:
            system("dunstify 'restart' "); 
            break;
        case 2:
            system("dunstify 'logout' "); 
            break;
        case 3:
            system("dunstify 'sleep' "); 
            break;
        case 4:
            system("dunstify 'cancel' "); 
            cleanup();
            exit(1);
            break;
    }

}

static void
inc_selection(void)
{
    int recWidth = w*3/5;
    int recHeight = h/10;
    
    if (selection < (5-1))
    {
        ++selection;
    } else {
        drawRectangles(recWidth, recHeight);
        selection = -1;
        return;
    }
    
    
    drawRectangles(recWidth, recHeight);
    
    XColor c;    
    
    XSetForeground(display, gc, 0xFF0000);
    
    const char* labels[5] =
    {
        "Shutdown",
        "Restart",
        "Logout",
        "Sleep",
        "Cancel"
    }; 
    
    int recX = (w-recWidth) / 2;
    int gap = ((h - (((h/8)-1)*2)) - ( 5 * recHeight)) / 4;
    int recY = (h/8) + (selection * ( recHeight + gap));
    XDrawRectangle(display, window, gc, recX, recY, recWidth, recHeight);
    
    // Calculate the starting position of the text to be centered
    int tx = recX + (recWidth - XTextWidth(font, labels[selection], strlen(labels[selection]))) / 2;
    int ty = recY + (recHeight + (font->ascent + font->descent)) / 2 - font->descent;
    
    XDrawString(display, window, gc, tx, ty, labels[selection], strlen(labels[selection]));
    
}

static void
drawtray(void)
{
    gc = XCreateGC(display, window, 0, NULL); 
    
    // Load font
    font = XLoadQueryFont(display, "fixed");
    if (!font) {
        fprintf(stderr, "Unable to load font\n");
        exit(1);
    }
    XSetFont(display, gc, font->fid);
    
    int recWidth = w*3/5;
    int recHeight = h/10;
    
    drawRectangles(recWidth, recHeight);
    
    // XDrawArc(display, window, gc, 100, 70,  100, 100, 0, 360*64);
    // XDrawArc(display, window, gc, 100, 190, 100, 100, 0, 360*64);
    // XDrawArc(display, window, gc, 100, 310, 100, 100, 0, 360*64);
}

static void
drawRectangles(int recWidth, int recHeight)
{
    XSetForeground(display, gc, 0xFFFFFF);
    const char* labels[5] =
    {
        "Shutdown",
        "Restart",
        "Logout",
        "Sleep",
        "Cancel"
    }; 
    
    int recX = (w-recWidth) / 2;
    for (int i = 0; i < 5; i++) {
        // This is awful
        int gap = ((h - (((h/8)-1)*2)) - ( 5 * recHeight)) / 4;
        int recY = (h/8) + (i * ( recHeight + gap));
        XDrawRectangle(display, window, gc, recX, recY, recWidth, recHeight);
        
        // Calculate the starting position of the text to be centered
        int tx = recX + (recWidth - XTextWidth(font, labels[i], strlen(labels[i]))) / 2;
        int ty = recY + (recHeight + (font->ascent + font->descent)) / 2 - font->descent;
        
        XDrawString(display, window, gc, tx, ty, labels[i], strlen(labels[i]));
    }
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
        case XK_Tab:
            inc_selection(); break;
        case XK_Return:
            select(); break;
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
