#include "X11/Xlib.h"
#include <X11/Xatom.h>
#include <X11/Xfuncproto.h>
#include <X11/Intrinsic.h>	/* Xlib, Xutil, Xresource, Xfuncproto */
#include <gdk-pixbuf-xlib/gdk-pixbuf-xlib.h>

void fix_set(GdkPixbuf *image)
{
	Atom prop_root, prop_esetroot, type;
	Display *disp2;
	Window root2;
	int depth2;
	Pixmap pmap_d1, pmap_d1_mask;
	XGCValues gcvalues;
	GC gc;
	int in, out, w, h;
	int format;
	unsigned long length,after;
	unsigned char *data_root, *data_esetroot;
	int width = gdk_pixbuf_get_width(image);
	int height = gdk_pixbuf_get_height(image);

	/* create new display, copy pixmap to new display */
	disp2 = XOpenDisplay(NULL);
	if (!disp2)
		printf("Can't reopen X display.");
	root2 = RootWindow(disp2, DefaultScreen(disp2));
	depth2 = DefaultDepth(disp2, DefaultScreen(disp2));

	// Init gdk for this display.
	gdk_pixbuf_xlib_init(disp2, DefaultScreen(disp2));
	// Create the Pixmap and the transparency map (we do not use)	
	gdk_pixbuf_xlib_render_pixmap_and_mask(image, &pmap_d1, &pmap_d1_mask,0);	
	// Free the mask
	XFreePixmap(disp2, pmap_d1_mask);

	// Not sure about this one, copied from FEH.
	prop_root = XInternAtom(disp2, "_XROOTPMAP_ID", True);
	prop_esetroot = XInternAtom(disp2, "ESETROOT_PMAP_ID", True);

	if (prop_root != None && prop_esetroot != None) {
		XGetWindowProperty(disp2, root2, prop_root, 0L, 1L,
				False, AnyPropertyType, &type, &format, &length, &after, &data_root);
		if (type == XA_PIXMAP) {
			XGetWindowProperty(disp2, root2,
					prop_esetroot, 0L, 1L,
					False, AnyPropertyType,
					&type, &format, &length, &after, &data_esetroot);
			if (data_root && data_esetroot) {
				if (type == XA_PIXMAP && *((Pixmap *) data_root) == *((Pixmap *) data_esetroot)) {
					XKillClient(disp2, *((Pixmap *)
								data_root));
				}
			}
		}
	}

	/* This will locate the property, creating it if it doesn't exist */
	prop_root = XInternAtom(disp2, "_XROOTPMAP_ID", False);
	prop_esetroot = XInternAtom(disp2, "ESETROOT_PMAP_ID", False);

	if (prop_root == None || prop_esetroot == None)
		printf("creation of pixmap property failed.");

	XChangeProperty(disp2, root2, prop_root, XA_PIXMAP, 32, PropModeReplace, (unsigned char *) &pmap_d1, 1);
	XChangeProperty(disp2, root2, prop_esetroot, XA_PIXMAP, 32,
			PropModeReplace, (unsigned char *) &pmap_d1, 1);
	// Set it
	XSetWindowBackgroundPixmap(disp2, root2, pmap_d1);
	XClearWindow(disp2, root2);
	XFlush(disp2);
	XSetCloseDownMode(disp2, RetainPermanent);
	XCloseDisplay(disp2);
}
