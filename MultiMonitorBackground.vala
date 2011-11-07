/* sshconf.vala
 *
 * Copyright (C) 2011 Qball Cow <qball@gmpclient.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author:
 * 	Qball Cow <qball@gmpclient.org>
 */
using Gtk;
using Posix;

/**
 * Set the actual background on the X server
 */
static void set_background(Gdk.Pixbuf background_pb)
{
		Fix.set(background_pb);
}


/**
 * Option parser
 */
string  clipping = null;
const GLib.OptionEntry[] entries = {
        {"clipping", 'c', 0, GLib.OptionArg.STRING, ref clipping, "Enable clipping", null},
        {null}
};

/**
 * Main program
 */
static int main (string[] argv)
{
	bool clip = true;

	/**
	 * Parse command line list
	 */
    GLib.OptionContext og = new GLib.OptionContext("Evo");
    og.add_main_entries(entries,null);
	try{
		og.parse(ref argv);
	}catch (Error e) {
		GLib.error("Failed to parse command line options: %s\n", e.message);
	}
	/**
	 * Parse clipping
	 */	
	if(clipping != null) {
		if(clipping.down() == "true") {
			clip = true;
		} else {
			clip = false;
		}
	}

	/**
	 * Get filename
	 */
	if(argv.length  < 2) {
		GLib.error("Failed parsing commandlines");
	}
	// Initialize GTK.
	Gdk.init(ref argv);

	// Get display
	Gdk.Display display = Gdk.Display.get_default();

	// Get number of screens.
	int num_screens = display.get_n_screens();

	int set = 1;
/*
	do {	
*/		weak string file = argv[set];
		set++;
		// Walk the screens.
		for(int screen = 0; screen < num_screens ; screen++)
		{
			unowned Gdk.Screen gscreen = display.get_screen(screen);

			int rw_width = gscreen.get_width();
			int rw_height = gscreen.get_height();

			// Create background pixmap.
			var background_pb = new Gdk.Pixbuf(Gdk.Colorspace.RGB,true,8, rw_width, rw_height);
			background_pb.fill(0x00000000);

			// Iterate over all the monitors and draw the image there.

			// Get number of monitors.
			int num_monitors = gscreen.get_n_monitors();
			for(int monitor = 0; monitor < num_monitors; monitor++)
			{
				Gdk.Rectangle rectangle;
				// get monitor size
				gscreen.get_monitor_geometry(monitor, out rectangle);

				// Gdk.Pixbuf loading and drawing inside.
				try
				{
					Gdk.Pixbuf? pb = new Gdk.Pixbuf.from_file(file); 
					int new_width = 0;
					int new_height = 0;
					// Get size
					int pb_width = pb.get_width();
					int pb_height = pb.get_height();
					// Scaling
					double x_scale = pb_width/(double)rectangle.width;
					double y_scale = pb_height/(double)rectangle.height;

					if(x_scale >= 1 && y_scale >= 1 && clip) 
					{
						GLib.debug("Clip image to fit monitor");
						// Clip the image so the whole monitor is covered.
						if(x_scale > y_scale)
						{
							new_width = (int)(pb_width/y_scale);
							new_height = (int)(pb_height/y_scale);
							pb = pb.scale_simple(new_width, new_height, Gdk.InterpType.HYPER); 
						} else {
							new_width = (int)(pb_width/x_scale);
							new_height = (int)(pb_height/x_scale);
							pb = pb.scale_simple(new_width, new_height, Gdk.InterpType.HYPER); 
						}

						int x_offset = (int)GLib.Math.ceil((new_width - rectangle.width)/2.0);
						int y_offset = (int)GLib.Math.ceil((new_height- rectangle.height)/2.0);
						pb.copy_area(
								x_offset,y_offset,
								new_width-x_offset*2,new_height-y_offset*2,
								background_pb,
								rectangle.x,rectangle.y
								);
					}
					else if (y_scale < 1 && x_scale < 1)
					{
						GLib.debug("Does not fit.");
						// Image is to small, center it.
						pb.copy_area(0,0,
								pb_width, pb_height,
								background_pb,
								rectangle.x + (rectangle.width-pb_width)/2,
								rectangle.y + (rectangle.height-pb_height)/2
								);
					}
					else 
					{
						GLib.debug("Scale image to fit monitor");
						// Fit the image as good as possible.
						if(x_scale > y_scale)
						{
							new_width = (int)(pb_width/x_scale);
							new_height = (int)(pb_height/x_scale);
						}
						else 
						{
							new_width = (int)(pb_width/y_scale);
							new_height = (int)(pb_height/y_scale);
						}
						pb = pb.scale_simple(new_width, new_height, Gdk.InterpType.HYPER); 
						pb.copy_area(0,0,
								new_width, new_height,
								background_pb,
								rectangle.x + (rectangle.width-new_width)/2,
								rectangle.y + (rectangle.height-new_height)/2
								);
					}

				} catch (Error e) {
					GLib.error("Failed loading pixbuf: %s\n", e.message);
				}

			}
			set_background(background_pb);
		}
/*		Posix.sleep(5);
		if(set == argv.length) {
			set = 1;
		}
	}while( set < argv.length);
*/
	return 1;
}
