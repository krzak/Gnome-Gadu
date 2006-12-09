#include "config.h"

#include <gtk/gtk.h>
#include "gnomegadu_stock.h"

void
gnomegadu_stock_icons_init ()
{
	GtkIconFactory *factory;
	GtkIconSet *icon_set;
	GtkIconSource *icon_source;
	gchar *filename, *full_filename;
	gint i;

	factory = gtk_icon_factory_new ();
	
	gtk_icon_size_register(GNOMEGADU_ICON_SIZE,16,16);

	for (i = 0; i < (int) G_N_ELEMENTS (gnomegadu_stock_items); i++) {
		filename = g_strconcat (gnomegadu_stock_items[i].stock_id, ".png", NULL);
		full_filename = g_strconcat (PACKAGE_DATA_DIR, "/", filename, NULL);

		icon_source = gtk_icon_source_new ();
		gtk_icon_source_set_icon_name (icon_source, gnomegadu_stock_items[i].stock_id);
		gtk_icon_source_set_filename (icon_source, full_filename);
		gtk_icon_source_set_size (icon_source, gtk_icon_size_from_name(GNOMEGADU_ICON_SIZE));
//		gtk_icon_source_set_size_wildcarded (icon_source, FALSE);
		
		icon_set = gtk_icon_set_new ();
		gtk_icon_set_add_source (icon_set, icon_source);

		gtk_icon_factory_add (factory, gnomegadu_stock_items[i].stock_id, icon_set);
		gtk_icon_set_unref (icon_set);

		gtk_icon_source_free (icon_source);
		g_free (full_filename);
		g_free (filename);
	}

	gtk_stock_add_static (gnomegadu_stock_items, G_N_ELEMENTS (gnomegadu_stock_items));

	gtk_icon_factory_add_default (factory);
	g_object_unref (factory);
}

GdkPixbuf *gnomegadu_stock_get_pixbuf(const gchar *stock_id)
{
    GdkPixbuf  *pix = NULL;
    GtkIconSet *iconset = gtk_icon_factory_lookup_default(stock_id);

    g_assert(iconset);
    
    pix = gtk_icon_set_render_icon(iconset,
		    gtk_widget_get_default_style(),
		    gtk_widget_get_default_direction(),
		    GTK_STATE_NORMAL,
		    gtk_icon_size_from_name(GNOMEGADU_ICON_SIZE),
		    NULL,
		    NULL);

    return pix;	
}