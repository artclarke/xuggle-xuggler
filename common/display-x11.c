/*****************************************************************************
 * x264: h264 encoder
 *****************************************************************************
 * Copyright (C) 2005 Tuukka Toivonen <tuukkat@ee.oulu.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *****************************************************************************/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <string.h>

#include "display.h"

static long event_mask = ConfigureNotify|ExposureMask|KeyPressMask|ButtonPressMask|StructureNotifyMask|ResizeRedirectMask;

static Display *disp_display = NULL;
static struct disp_window {
	int init;
	Window window;
} disp_window[10];

static inline void disp_chkerror(int cond, char *e) {
	if (!cond) return;
	fprintf(stderr, "error: %s\n", e ? e : "?");
	abort();
}

static void disp_init_display(void) {
	Visual *visual;
	int dpy_class;
	int screen;
	int dpy_depth;

	if (disp_display != NULL) return;
	memset(&disp_window, 0, sizeof(disp_window));
	disp_display = XOpenDisplay("");
	disp_chkerror(!disp_display, "no display");
	screen = DefaultScreen(disp_display);
	visual = DefaultVisual(disp_display, screen);
	dpy_class = visual->class;
	dpy_depth = DefaultDepth(disp_display, screen);
	disp_chkerror(!((dpy_class == TrueColor && dpy_depth == 32)
		|| (dpy_class == TrueColor && dpy_depth == 24)
		|| (dpy_class == TrueColor && dpy_depth == 16)
		|| (dpy_class == PseudoColor && dpy_depth == 8)),
		"requires 8 bit PseudoColor or 16/24/32 bit TrueColor display");
}

static void disp_init_window(int num, int width, int height, const unsigned char *tit) {
	XSizeHints *shint;
	XSetWindowAttributes xswa;
	XEvent xev;
	int screen = DefaultScreen(disp_display);
	Visual *visual = DefaultVisual (disp_display, screen);
	unsigned int fg, bg;
	unsigned int mask;
	char title[200];
	Window window;
	int dpy_depth;

	if (tit) {
		snprintf(title, 200, "%s: %i/disp", tit, num);
	} else {
		snprintf(title, 200, "%i/disp", num);
	}
	shint = XAllocSizeHints();
	disp_chkerror(!shint, "memerror");
	shint->min_width = shint->max_width = shint->width = width;
	shint->min_height = shint->max_height = shint->height = height;
	shint->flags = PSize | PMinSize | PMaxSize;
	disp_chkerror(num<0 || num>=10, "bad win num");
	if (!disp_window[num].init) {
		disp_window[num].init = 1;
		bg = WhitePixel(disp_display, screen);
		fg = BlackPixel(disp_display, screen);
		dpy_depth = DefaultDepth(disp_display, screen);
		if (dpy_depth==32 || dpy_depth==24 || dpy_depth==16) {
			mask |= CWColormap;
			xswa.colormap = XCreateColormap(disp_display, DefaultRootWindow(disp_display), visual, AllocNone);
		}
		xswa.background_pixel = bg;
		xswa.border_pixel = fg;
		xswa.backing_store = Always;
		xswa.backing_planes = -1;
		xswa.bit_gravity = NorthWestGravity;
		mask = CWBackPixel | CWBorderPixel | CWBackingStore | CWBackingPlanes | CWBitGravity;
		window = XCreateWindow(disp_display, DefaultRootWindow(disp_display),
				shint->x, shint->y, shint->width, shint->height,
				1, dpy_depth, InputOutput, visual, mask, &xswa);
		disp_window[num].window = window;

		XSelectInput(disp_display, window, event_mask);
		XSetStandardProperties(disp_display, window, title, title, None, NULL, 0, shint);	/* Tell other applications about this window */
		XMapWindow(disp_display, window);							/* Map window. */
		do {											/* Wait for map. */
			XNextEvent(disp_display, &xev);
		} while (xev.type!=MapNotify || xev.xmap.event!=window);
		//XSelectInput(disp_display, window, KeyPressMask);					/*  XSelectInput(display, window, NoEventMask);*/
	}
	window = disp_window[num].window;
	XSetStandardProperties(disp_display, window, title, title, None, NULL, 0, shint);		/* Tell other applications about this window */
	XResizeWindow(disp_display, window, width, height);
	XSync(disp_display, 1);
	XFree(shint);
}

void disp_sync(void) {
	XSync(disp_display, 1);
}

void disp_setcolor(unsigned char *name) {
	int screen;
	GC gc;
	XColor c_exact, c_nearest;
	Colormap cm;
	Status st;

	screen = DefaultScreen(disp_display);
	gc = DefaultGC(disp_display, screen);		/* allocate colors */
	cm = DefaultColormap(disp_display, screen);
	st = XAllocNamedColor(disp_display, cm, name, &c_nearest, &c_exact);
	disp_chkerror(st!=1, "XAllocNamedColor error");
	XSetForeground(disp_display, gc, c_nearest.pixel);
}

void disp_gray(int num, char *data, int width, int height, int stride, const unsigned char *tit) {
	Visual *visual;
	XImage *ximage;
	unsigned char *image;
	int y,x,pixelsize;
	char dummy;
	int t = 1;
	int dpy_depth;
	int screen;
	GC gc;
	//XEvent xev;

	disp_init_display();
	disp_init_window(num, width, height, tit);
	screen = DefaultScreen(disp_display);
	visual = DefaultVisual(disp_display, screen);
	dpy_depth = DefaultDepth(disp_display, screen);
	ximage = XCreateImage(disp_display, visual, dpy_depth, ZPixmap, 0, &dummy, width, height, 8, 0);
	disp_chkerror(!ximage, "no ximage");
	if (*(char *)&t == 1) {
		ximage->byte_order = LSBFirst;
		ximage->bitmap_bit_order = LSBFirst;
	} else {
		ximage->byte_order = MSBFirst;
		ximage->bitmap_bit_order = MSBFirst;
	}
	pixelsize = dpy_depth>8 ? sizeof(int) : sizeof(unsigned char);
	image = malloc(width * height * pixelsize);
	disp_chkerror(!image, "malloc failed");
	for (y=0; y<height; y++) for (x=0; x<width; x++) {
		memset(&image[(width*y + x)*pixelsize], data[y*stride+x], pixelsize);
	}
	ximage->data = image;
	gc = DefaultGC(disp_display, screen);	/* allocate colors */

//	XUnmapWindow(disp_display, disp_window[num].window);							/* Map window. */
//	XMapWindow(disp_display, disp_window[num].window);							/* Map window. */
	XPutImage(disp_display, disp_window[num].window, gc, ximage, 0, 0, 0, 0, width, height);
//		do {											/* Wait for map. */
//			XNextEvent(disp_display, &xev);
//		} while (xev.type!=MapNotify || xev.xmap.event!=disp_window[num].window);
	XPutImage(disp_display, disp_window[num].window, gc, ximage, 0, 0, 0, 0, width, height);

	XDestroyImage(ximage);
	XSync(disp_display, 1);

}

void disp_gray_zoom(int num, char *data, int width, int height, int stride, const unsigned char *tit, int zoom) {
	unsigned char *dataz;
	int y,x,y0,x0;
	dataz = malloc(width*zoom * height*zoom);
	disp_chkerror(!dataz, "malloc");
	for (y=0; y<height; y++) for (x=0; x<width; x++) {
		for (y0=0; y0<zoom; y0++) for (x0=0; x0<zoom; x0++) {
			dataz[(y*zoom + y0)*width*zoom + x*zoom + x0] = data[y*stride+x];
		}
	}
	disp_gray(num, dataz, width*zoom, height*zoom, width*zoom, tit);
	free(dataz);
}

void disp_point(int num, int x1, int y1) {
	int screen;
	GC gc;
	screen = DefaultScreen(disp_display);
	gc = DefaultGC(disp_display, screen);		/* allocate colors */
	XDrawPoint(disp_display, disp_window[num].window, gc, x1, y1);
//	XSync(disp_display, 1);
}

void disp_line(int num, int x1, int y1, int x2, int y2) {
	int screen;
	GC gc;
	screen = DefaultScreen(disp_display);
	gc = DefaultGC(disp_display, screen);		/* allocate colors */
	XDrawLine(disp_display, disp_window[num].window, gc, x1, y1, x2, y2);
//	XSync(disp_display, 1);
}

void disp_rect(int num, int x1, int y1, int x2, int y2) {
	int screen;
	GC gc;
	screen = DefaultScreen(disp_display);

	gc = DefaultGC(disp_display, screen);		/* allocate colors */
	XDrawRectangle(disp_display, disp_window[num].window, gc, x1, y1, x2-x1, y2-y1);
//	XSync(disp_display, 1);
}
