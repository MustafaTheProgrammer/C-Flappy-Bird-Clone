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
    if (x >= 0 && x <= Buf->W-1 && y >= 0 && y <= Buf->H-1){
        uint32_t *pixel = (uint32_t *)((uint32_t *)Buf->Mem + (599-y) * Buf->W + x);
        *pixel = color;
    }
}

void rect(buf *Buf, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color){
    for (int i = x; i < x + w; i++){
        for (int j = y; j < y + h; j++)
            setpixel(Buf, i, j, color);
    }
}

int aabb(int minx1, int miny1, int maxx1, int maxy1, int minx2, int miny2, int maxx2, int maxy2){
    return minx1 <= maxx2 && maxx1 >= minx2 && miny1 <= maxy2 && maxy1 >= miny2;
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
    
    int x = 100;
    double playery = 200;
    double playeryv;
    int state = 0;
    int score = 0;

    int pipe1botx = 800;
    int pipe1boty = 0;
    int pipe1topx = 800;

    int pipe2botx = 1250;
    int pipe2boty = -100;
    int pipe2topx = 1250;

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
		    if (event->keycode == XKeysymToKeycode(dis, XK_Up)){
		        if (state == 0)
			    state = 1;
		        if (state == 1)
			    playeryv = 5;
		    }
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
	if (state == 1){
	    playeryv -= 0.1;
	    playery += playeryv;

	    if (playery + 49 > 599){
	        playery = 549;
		playeryv = 0;
	    }

	    pipe1botx -= 2;
	    pipe1topx -= 2;
	    if (pipe1botx == x)
		score++;

	    if (pipe1botx == -76){
	        pipe1botx = 800;
		pipe1topx = 800;
	    }

	    pipe2botx -= 2;
	    pipe2topx -= 2;
	    if (pipe2botx == x)
		score++;

	    if (pipe2botx == -76){
	        pipe2botx = 800;
		pipe2topx = 800;
	    }

	    if (aabb(x, playery, x + 49, playery + 49, pipe1botx, pipe1boty, pipe1botx + 74, pipe1boty + 199))
		running = 0;
	    if (aabb(x, playery, x + 49, playery + 49, pipe1topx, pipe1boty+500, pipe1topx + 74, pipe1boty + 699))
		running = 0;
	    if (aabb(x, playery, x + 49, playery + 49, pipe2botx, pipe2boty, pipe2botx + 74, pipe2boty + 199))
		running = 0;
	    if (aabb(x, playery, x + 49, playery + 49, pipe2topx, pipe2boty+500, pipe2topx + 74, pipe2boty + 699))
		running = 0;
	    if (playery + 49 < 0)
		running = 0;
	}

        //render
	if (state == 1){
	    rect(&Buf, 0, 0, Buf.W, Buf.H, 0x528afa);
	    rect(&Buf, x, (int32_t)playery, 50, 50, 0xffff00);

	    rect(&Buf, pipe1botx, pipe1boty, 75, 200, 0x38fc3b);
	    rect(&Buf, pipe1botx, pipe1boty+500, 75, 200, 0x38fc3b);
	    rect(&Buf, pipe2botx, pipe2boty, 75, 200, 0x38fc3b);
	    rect(&Buf, pipe2botx, pipe2boty+500, 75, 200, 0x38fc3b);
	}

	int sx = 0;
	int sy = 0;
	int dx = 0;
	int dy = 0;
        XPutImage(dis, win, context, winbuf, sx, sy, dx, dy, winw, winh);
    }
    printf("\033[H\033[2J");
    printf("Score: %d\n", score);
}
