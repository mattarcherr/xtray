#include <stdlib.h> // for exit()

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xft/Xft.h>
#include <X11/XKBlib.h>

/* Global Vars */
static Display *dpy;
static Window root, parentwin, win;

static int bh, mw, mh;
static Display *display;
static Window window;
static XEvent event;

static void
keypress(XKeyEvent *ev)
{

}


static void 
run(void)
{
    XEvent ev;

    /* Main event loop */
    while (!XNextEvent(display, &ev)) {
        switch(ev.type) {
            case KeyPress:
                break;
            case DestroyNotify:
                if (ev.xdestroywindow.window != win)
                    break;
                // cleanup();
                exit(1);
        }

    }

}

static void 
setup(void)
{
    // Open connection to X server
    display = XOpenDisplay(NULL);
    if (display == NULL) {
        exit(1);
    }

    // Create a simple window
    window = XCreateSimpleWindow(display, RootWindow(display, 0), 10, 10, 640, 480, 1,
                                 BlackPixel(display, 0), WhitePixel(display, 0));
    XSelectInput(display, window, ExposureMask | KeyPressMask);
    XMapWindow(display, window);
}

int main() {

    setup();
    run();

    // Close connection to X server
    XCloseDisplay(display);
    return 0;
}
