#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <glade/glade.h>

#include "config.h"
#include "eggtrayicon.h"
#include "gnomegadu_conf.h"
#include "gnomegadu_tray.h"
#include "gnomegadu_ui.h"

static void
gnomegadu_tray_show_hide_main_window ()
{
	GtkWidget *main_window = glade_xml_get_widget (gladexml, "MainWindow");
	gboolean unvisible;
	g_assert (main_window);

	unvisible = (gboolean) g_object_get_data (G_OBJECT (main_window), "unvisible");

	if (unvisible)
	{
		gtk_widget_show (main_window);
		g_object_set_data (G_OBJECT (main_window), "unvisible", (gpointer) FALSE);
	}
	else
	{
		gtk_widget_hide (main_window);
		g_object_set_data (G_OBJECT (main_window), "unvisible", (gpointer) TRUE);
	}
}

gboolean
tray_button_press_cb (GtkWidget * widget, GdkEventButton * event, gpointer user_data)
{
	if (event->type == GDK_BUTTON_PRESS && event->button == 1)
	{
		gnomegadu_tray_show_hide_main_window();
	}

	if (event->type == GDK_BUTTON_PRESS && (event->button == 2 || event->button == 3))
	{
		GtkWidget *main_window = glade_xml_get_widget (gladexml, "MainWindow");
		GtkMenu *menu = GTK_MENU (glade_xml_get_widget (gladexml_tray_menu, "TrayPopupMenu"));
		GtkWidget *show_check_item = glade_xml_get_widget (gladexml_tray_menu, "popup_show_main_window");
		gboolean unvisible;
		
		g_assert(main_window);
		g_assert (menu);

		unvisible = (gboolean) g_object_get_data (G_OBJECT (main_window), "unvisible");
		
		g_object_set_data(G_OBJECT(show_check_item),"just_update",(gpointer)TRUE);
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(show_check_item),!unvisible);
		g_object_set_data(G_OBJECT(show_check_item),"just_update",(gpointer)FALSE);
		
		gtk_menu_popup (menu, NULL, NULL, NULL, NULL, event->button, event->time);
	}

	return FALSE;
}

gboolean
tray_destroy_cb (GtkWidget * widget, gpointer user_data)
{
	gtk_widget_destroy (GTK_WIDGET (tray_icon));
	tray_icon = NULL;
	tray_image = NULL;
	return TRUE;
}


void
on_tray_available_activate (GtkWidget * widget, gpointer user_data)
{
	GtkComboBox *combobox = GTK_COMBO_BOX (glade_xml_get_widget (gladexml, "StatusComboBox"));
	gtk_combo_box_set_active (combobox, 0);
}

void
on_tray_busy_activate (GtkWidget * widget, gpointer user_data)
{
	GtkComboBox *combobox = GTK_COMBO_BOX (glade_xml_get_widget (gladexml, "StatusComboBox"));
	gtk_combo_box_set_active (combobox, 1);
}

void
on_tray_unavailable_activate (GtkWidget * widget, gpointer user_data)
{
	GtkComboBox *combobox = GTK_COMBO_BOX (glade_xml_get_widget (gladexml, "StatusComboBox"));
	gtk_combo_box_set_active (combobox, 3);
}

void
on_popup_show_main_window_activate (GtkWidget * widget, gpointer user_data)
{
		if (!g_object_get_data(G_OBJECT(widget),"just_update"))
			gnomegadu_tray_show_hide_main_window();
}

void
on_popup_quit_activate (GtkWidget * widget, gpointer user_data)
{
	gtk_main_quit ();
}

void
gnomegadu_tray_init ()
{
	GtkWidget *tray_event_box;
	GdkPixbuf *pix = NULL;

	tray_image = gtk_image_new ();

	tray_icon = egg_tray_icon_new ("Gnome-Gadu");
	tray_event_box = gtk_event_box_new ();

	pix = create_pixbuf (USER_NOTAVAIL_ICON);
	gtk_image_set_from_pixbuf (GTK_IMAGE (tray_image), pix);
	g_object_unref (pix);

	gtk_container_add (GTK_CONTAINER (tray_event_box), tray_image);

	gtk_widget_show (tray_event_box);
	gtk_widget_show (tray_image);

	gtk_container_add (GTK_CONTAINER (tray_icon), tray_event_box);
	gtk_widget_show_all (GTK_WIDGET (tray_icon));

	gtk_widget_add_events (GTK_WIDGET (tray_icon), GDK_BUTTON_PRESS_MASK);
	g_signal_connect (tray_icon, "destroy", G_CALLBACK (tray_destroy_cb), tray_event_box);

	g_signal_connect (tray_icon, "button_press_event", G_CALLBACK (tray_button_press_cb), NULL);

	gladexml_tray_menu = glade_xml_new (PACKAGE_DATA_DIR "/gnomegadu.glade", "TrayPopupMenu", NULL);
	glade_xml_signal_autoconnect (gladexml_tray_menu);
}
