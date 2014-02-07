/**
 * simpleswitcher
 *
 * MIT/X11 License
 * Copyright (c) 2014 Qball  Cow <qball@gmpclient.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <err.h>
#include <math.h>
#include <errno.h>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/extensions/Xinerama.h>

// GDK
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf-xlib/gdk-pixbuf-xlib.h>

// CLI: argument parsing
static int find_arg( const int argc, char * const argv[], const char * const key )
{
    int i;

    for ( i = 0; i < argc && strcasecmp( argv[i], key ); i++ );

    return i < argc ? i: -1;
}
static void find_arg_str( int argc, char *argv[], char *key, char** val )
{
    int i = find_arg( argc, argv, key );

    if ( val != NULL &&  i > 0 && i < argc-1 ) {
        *val = argv[i+1];
    }
}


// Monitor layout stuff.
typedef struct {
    double x,y;
    double w,h;

} MMB_Rectangle;

typedef struct {
    // Size of the total screen area.
    MMB_Rectangle base;

    // Number of monitors.
    int num_monitors;
    // List of monitors;
    MMB_Rectangle *monitors;

} MMB_Screen;

/**
 * @param display An X11 Display
 *
 * Create MMB_Screen that holds the monitor layout of display.
 *
 * @returns filled in MMB_Screen
 */
static MMB_Screen *mmb_screen_create( Display *display )
{
    // Create empty structure.
    MMB_Screen *retv = malloc( sizeof( *retv ) );
    memset( retv, 0, sizeof( *retv ) );

    Screen *screen = DefaultScreenOfDisplay( display );
    retv->base.w = WidthOfScreen( screen );
    retv->base.h = HeightOfScreen( screen );

    // Parse xinerama setup.
    if ( XineramaIsActive( display ) ) {
        XineramaScreenInfo *info = XineramaQueryScreens( display, &( retv->num_monitors ) );

        if ( info != NULL ) {
            retv->monitors = malloc( retv->num_monitors*sizeof( MMB_Rectangle ) );

            for ( int i = 0; i < retv->num_monitors; i++ ) {
                retv->monitors[i].x = info[i].x_org;
                retv->monitors[i].y = info[i].y_org;
                retv->monitors[i].w = info[i].width;
                retv->monitors[i].h = info[i].height;
            }

            XFree( info );
        }
    }

    if ( retv->monitors == NULL ) {
        retv->num_monitors = 1;
        retv->monitors = malloc( 1*sizeof( MMB_Rectangle ) );
        retv->monitors[0] = retv->base;
    }

    return retv;
}
/**
 * @param screen a Pointer to the MMB_Screen pointer to free.
 *
 * Free MMB_Screen object and set pointer to NULL.
 */
static void mmb_screen_free( MMB_Screen **screen )
{
    if ( screen == NULL || *screen == NULL ) return;

    if ( ( *screen )->monitors != NULL ) free( ( *screen )->monitors );

    free( *screen );
    screen = NULL;
}

static void mmb_screen_print( const MMB_Screen *screen )
{
    printf( "Total size:    %.0f %.0f\n", screen->base.w, screen->base.h );
    printf( "Num. monitors: %d\n", screen->num_monitors );

    for ( int i =0; i < screen->num_monitors; i++ ) {
        printf( "\t%2d: %.0f %.0f -> %.0f %.0f\n",
                i,
                screen->monitors[i].x,
                screen->monitors[i].y,
                screen->monitors[i].w,
                screen->monitors[i].h
              );
    }

}


static void renderer_init( Display *display )
{
    // Init gdk for this display.
    gdk_pixbuf_xlib_init( display, DefaultScreen( display ) );


}

static GdkPixbuf * renderer_create_empty_background( MMB_Screen *screen )
{
    GdkPixbuf *pb = gdk_pixbuf_new( GDK_COLORSPACE_RGB, TRUE, 8, screen->base.w, screen->base.h );

    if ( pb == NULL ) {
        fprintf( stderr, "Failed to create background pixmap\n" );
        exit( EXIT_FAILURE );
    }

    // Make the wallpaper black.
    gdk_pixbuf_fill( pb, 0x000000FF );
    return pb;
}


static int renderer_overlay_wallpaper( GdkPixbuf *background,
                                       const char *wallpaper,
                                       MMB_Screen *screen,
                                       int clip )
{
    GError *error = NULL;
    GdkPixbuf *image = gdk_pixbuf_new_from_file( wallpaper, &error );

    if ( error != NULL ) {
        fprintf( stderr, "Failed to parse image: %s\n" , error->message );
        return 0;
    }

    double wp_width  = gdk_pixbuf_get_width( image );
    double wp_height = gdk_pixbuf_get_height( image );

    for ( int monitor = 0; monitor < screen->num_monitors; monitor++ ) {

        MMB_Rectangle rectangle = screen->monitors[monitor];
        double w_scale = wp_width/( double )( rectangle.w );
        double h_scale = wp_height/( double )( rectangle.h );
        printf( "%f %f\n", w_scale, h_scale );

        // Picture is small then screen, center it.
        if ( w_scale < 1 && h_scale < 1 ) {
            gdk_pixbuf_copy_area( image, 0,0,
                                  wp_width, wp_height,
                                  background,
                                  rectangle.x + ( rectangle.w-wp_width )/2,
                                  rectangle.y + ( rectangle.h-wp_height )/2
                                );
        }
        // Picture is smaller on one of the sides and we want to clip.
        else if ( clip && ( w_scale < 1 || h_scale < 1 ) ) {
            double x_off = ( ( float )rectangle.w-wp_width )/2.0;
            double y_off = ( ( float )rectangle.h-wp_height )/2.0;
            gdk_pixbuf_copy_area( image,
                                  -( x_off < 0 )*x_off,-( y_off < 0 )*y_off,
                                  wp_width+( x_off < 0 )*2*x_off,
                                  wp_height+( y_off < 0 )*2*y_off,
                                  background,
                                  rectangle.x + ( ( x_off> 0 )?x_off:0 ),
                                  rectangle.y + ( ( y_off> 0 )?y_off:0 ) );

        }
        // Picture is bigger/equal then screen.
        // Scale to fit.
        else {
            int new_w= 0;
            int new_h = 0;
            double x_off = 0;
            double y_off = 0;

            if ( clip ) {
                if ( w_scale < h_scale ) {
                    new_w = wp_width/w_scale;
                    new_h = wp_height/w_scale;
                } else {
                    new_w = wp_width/h_scale;
                    new_h = wp_height/h_scale;
                }

                x_off = ( ( new_w-rectangle.w )/2.0 );
                y_off = ( ( new_h-rectangle.h )/2.0 );
            } else {
                if ( w_scale > h_scale ) {
                    new_w = wp_width/w_scale;
                    new_h = wp_height/w_scale;
                } else {
                    new_w = wp_width/h_scale;
                    new_h = wp_height/h_scale;
                }
            }

            GdkPixbuf *scaled_wp = gdk_pixbuf_scale_simple( image, new_w, new_h, GDK_INTERP_HYPER );
            gdk_pixbuf_copy_area(
                scaled_wp,
                ( int )ceil( x_off ),( int )ceil( y_off ),
                new_w-x_off*2,
                new_h-y_off*2,
                background,
                rectangle.x + ( ( double )rectangle.w-new_w+x_off*2 )/2,
                rectangle.y + ( ( double )rectangle.h-new_h+y_off*2 )/2 );

            g_object_unref( scaled_wp );
        }
    }

    g_object_unref( image );
    return 1;
}


static void renderer_store_image( GdkPixbuf *bg, const char *output )
{
    gdk_pixbuf_save( bg, output, "png", NULL, NULL );
}

static void renderer_update_X11_background( Display *display, GdkPixbuf *image )
{
    Atom prop_root, prop_esetroot, type;
    Window root;
    Pixmap pmap_d1, pmap_d1_mask;
    int format;


    root = RootWindow( display, DefaultScreen( display ) );

    // Create the Pixmap and the transparency map (we do not use)
    gdk_pixbuf_xlib_render_pixmap_and_mask( image,
                                            &pmap_d1,
                                            &pmap_d1_mask,
                                            0 );
    // Free the mask
    XFreePixmap( display, pmap_d1_mask );

    // Not sure about this one, copied from FEH.
    prop_root = XInternAtom( display, "_XROOTPMAP_ID", True );
    prop_esetroot = XInternAtom( display, "ESETROOT_PMAP_ID", True );

    // If properties exist, kill the client that set this data.
    // and free it resources.
    if ( prop_root != None && prop_esetroot != None ) {
        unsigned long length,after;
        unsigned char *data_root, *data_esetroot;
        XGetWindowProperty( display, root, prop_root, 0L, 1L,
                            False, AnyPropertyType, &type, &format, &length, &after, &data_root );

        if ( type == XA_PIXMAP ) {
            XGetWindowProperty( display, root,
                                prop_esetroot, 0L, 1L,
                                False, AnyPropertyType,
                                &type, &format, &length, &after, &data_esetroot );

            if ( data_root && data_esetroot ) {
                if ( type == XA_PIXMAP && *( ( Pixmap * ) data_root ) == *( ( Pixmap * ) data_esetroot ) ) {
                    XKillClient( display, *( ( Pixmap * )
                                             data_root ) );
                    XSync( display, False );
                }
            }

            if ( data_esetroot ) XFree( ( void * )data_esetroot );
        }

        if ( data_root ) XFree( ( void * )data_root );
    }

    /* This will locate the property, creating it if it doesn't exist */
    prop_root = XInternAtom( display, "_XROOTPMAP_ID", False );
    prop_esetroot = XInternAtom( display, "ESETROOT_PMAP_ID", False );

    if ( prop_root == None || prop_esetroot == None ) {
        fprintf( stderr, "creation of pixmap property failed." );
        return;
    }

    // Store the pmap_d1 in the X server.
    XChangeProperty( display, root, prop_root, XA_PIXMAP, 32,
                     PropModeReplace, ( unsigned char * ) &pmap_d1, 1 );
    XChangeProperty( display, root, prop_esetroot, XA_PIXMAP, 32,
                     PropModeReplace, ( unsigned char * ) &pmap_d1, 1 );


    // Set it
    XSetWindowBackgroundPixmap( display, root, pmap_d1 );
    XClearWindow( display, root );

    XSync( display, True );

    // Make sure X keeps the data we set.
    XSetCloseDownMode( display, RetainPermanent );
}

// X error handler
static int ( *xerror )( Display *, XErrorEvent * );
int X11_oops( Display *display, XErrorEvent *ee )
{
    if ( ee->error_code == BadWindow
         || ( ee->request_code == X_GrabButton && ee->error_code == BadAccess )
         || ( ee->request_code == X_GrabKey && ee->error_code == BadAccess )
       ) return 0;

    fprintf( stderr, "error: request code=%d, error code=%d\n", ee->request_code, ee->error_code );
    return xerror( display, ee );
}

static void help()
{
    int code = execlp( "man","man", MANPAGE_PATH,NULL );

    if ( code == -1 ) {
        fprintf( stderr, "Failed to execute man: %s\n", strerror( errno ) );
    }
}

int main ( int argc, char **argv )
{
    Display *display;

    // catch help request
    if ( find_arg( argc, argv, "-help" ) >= 0
         || find_arg( argc, argv, "--help" ) >= 0
         || find_arg( argc, argv, "-h" ) >= 0 ) {
        help();
        return EXIT_SUCCESS;
    }

    // Get DISPLAY
    const char *display_str= getenv( "DISPLAY" );

    if ( !( display = XOpenDisplay( display_str ) ) ) {
        fprintf( stderr, "cannot open display!\n" );
        return EXIT_FAILURE;
    }

    // Setup error handler.
    XSync( display, False );
    xerror = XSetErrorHandler( X11_oops );
    XSync( display, False );

    // Initialize renderer
    renderer_init( display );

    // Get monitor layout. (xinerama aware)
    MMB_Screen *mmb_screen = mmb_screen_create( display );


    // Check input file.
    char *image_file = NULL;
    find_arg_str( argc, argv, "-input", &image_file );

    if ( image_file ) {
        int clip = ( find_arg( argc, argv, "-clip" ) >= 0 );
        GdkPixbuf *pb = renderer_create_empty_background( mmb_screen );


        if ( renderer_overlay_wallpaper( pb, image_file, mmb_screen,clip ) ) {
            char *output_file = NULL;
            find_arg_str( argc, argv, "-output", &output_file );

            if ( output_file ) {
                renderer_store_image( pb, output_file );
            } else {
                renderer_update_X11_background( display, pb );
            }
        }

        g_object_unref( pb );
    }

    // Print layout
    if ( find_arg( argc, argv, "-print" ) >= 0 ) {
        mmb_screen_print( mmb_screen );
    }

    // Cleanup
    mmb_screen_free( &mmb_screen );
    XCloseDisplay( display );
}
