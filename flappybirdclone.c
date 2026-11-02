/*To-do:
 -set BG color
 -proper screen clearing
 -player physics
 -pipes
 -losing
 * */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define STATUS_ERROR 0

typedef struct {
    uint8_t *Mem;
    uint64_t Size;
    uint32_t W;
    uint32_t H;
    uint32_t Pitch;
} buf;

void setpixel(buf *Buf, int32_t x, int32_t y, uint32_t color){
    uint32_t *pixel = (uint32_t *)((uint32_t *)Buf->Mem + (599-y) * Buf->W + x);
    *pixel = color;
}

void rect(buf *Buf, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color){
    for (int i = x; i < x + w; i++){
        for (int j = x; j < y + h; j++)
            setpixel(Buf, i, j, color);
    }
}

int main(){
    Display *dis = XOpenDisplay(0);
    Window rootwin = XDefaultRootWindow(dis);
    int scr = DefaultScreen(dis);
    GC context = XDefaultGC(dis, scr);
    int scrdepth = 24;
    XVisualInfo visinfo = {};
    if (STATUS_ERROR == XMatchVisualInfo(dis, scr, scrdepth, TrueColor, &visinfo))
        printf("no matching visual info\n");

    int winx = 0;
    int winy = 0;
    int winw = 800;
    int winh = 600;
    int borderw = 0;
    int windepth = visinfo.depth;
    int winclass = InputOutput;
    Visual *winvis = visinfo.visual;

    int attvaluemask = CWBackPixel | CWEventMask;
    XSetWindowAttributes winatt = {};
    winatt.background_pixel = 0xffffff;
    winatt.event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask | ExposureMask;
    Window win = XCreateWindow(dis, rootwin, winx, winy, winw, winh, borderw, windepth, winclass, winvis, attvaluemask, &winatt);
    XMapWindow(dis, win);
    XStoreName(dis, win, "C Flappy Bird Close");

    Atom wm_delete_win = XInternAtom(dis, "wm_delete_win", False);
    if (!XSetWMProtocols(dis, win, &wm_delete_win, 1))
        printf("could register wm_delete_win property\n");

    int bitsperpixel = 32;
    int bytesperpixel = bitsperpixel/8;
    int winbufsize = winw * winh * bytesperpixel;

    buf Buf = {};
    Buf.W = winw;
    Buf.H = winh;
    Buf.Pitch = Buf.W * bytesperpixel;
    Buf.Size = Buf.Pitch * Buf.H;
    Buf.Mem = (uint8_t *)malloc(Buf.Size);

    int offset = 0;
    int bytesbetweenscanlines = 0;
    XImage *winbuf = XCreateImage(dis, visinfo.visual, visinfo.depth, ZPixmap, offset, (char *)Buf.Mem, winw, winh, bitsperpixel, bytesbetweenscanlines);
    
    int playerx = 200;
    int playery = 250;
    int playeryv = 0;

    int running = 1;
    while (running){
        while (XPending(dis) > 0){
	    XEvent gevent = {};
	    XNextEvent(dis, &gevent);
	    switch (gevent.type){
		case KeyPress:
		case KeyRelease:
		{
		    XKeyPressedEvent *event = (XKeyPressedEvent *)&gevent;
		    if (event->keycode == XKeysymToKeycode(dis, XK_Escape))
		        running = 0;
		    if (event->keycode == XKeysymToKeycode(dis, XK_Up))
		        playeryv = 10;
		}
		case ClientMessage:
		{
		    XClientMessageEvent *event = (XClientMessageEvent *)&gevent;
		    if ((Atom)event->data.l[0] == wm_delete_win){
		        XDestroyWindow(dis, win);
		        running = 0;
		    }
		} break;
		case ConfigureNotify:
		{
		    XConfigureEvent *event = (XConfigureEvent *)&gevent;
		    winw = event->width;
		    winh = event->height;
		    XDestroyImage(winbuf);
                     
                    Buf.W = winw;
                    Buf.H = winh;
                    Buf.Pitch = Buf.W * bytesperpixel;
                    Buf.Size = Buf.Pitch * Buf.H;
                    Buf.Mem = (uint8_t *)malloc(Buf.Size);
                    winbuf = XCreateImage(dis, visinfo.visual, visinfo.depth, ZPixmap, offset, (char *)Buf.Mem, winw, winh, bitsperpixel, bytesbetweenscanlines);
		} break;
	    }
	}
        //logic
	playeryv -= 2;
	playery += playeryv;

        //render
	rect(&Buf, 0, playery, 50, 50, 0xffff00);

	int sx = 0;
	int sy = 0;
	int dx = 0;
	int dy = 0;
        XPutImage(dis, win, context, winbuf, sx, sy, dx, dy, winw, winh);
    }
}
