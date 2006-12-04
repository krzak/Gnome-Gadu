#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <glade/glade.h>

#include "config.h"
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

void on_StatusIcon_activate(GtkStatusIcon *status_icon, gpointer user_data) {
	gnomegadu_tray_show_hide_main_window();
}

void on_StatusIcon_popupmenu(GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer user_data) {
	GtkWidget *main_window = glade_xml_get_widget (gladexml, "MainWindow");
	GtkMenu *menu = GTK_MENU (glade_xml_get_widget (gladexml_tray_menu, "TrayPopupMenu"));
	GtkWidget *show_check_item = glade_xml_get_widget (gladexml_tray_menu, "popup_show_main_window");
	gboolean unvisible;
	
	unvisible = (gboolean) g_object_get_data (G_OBJECT (main_window), "unvisible");
	
	g_object_set_data(G_OBJECT(show_check_item),"just_update",(gpointer)TRUE);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(show_check_item),!unvisible);
	g_object_set_data(G_OBJECT(show_check_item),"just_update",(gpointer)FALSE);
	
	gtk_menu_popup (menu, NULL, NULL, NULL, NULL, button, activate_time);
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
	GdkPixbuf *pix = NULL;

	pix = create_pixbuf (USER_NOTAVAIL_ICON);
	status_icon = gtk_status_icon_new_from_pixbuf(pix);
	g_object_unref (pix);
	
	if (!gtk_status_icon_is_embedded)
	    g_printerr("There is no 'Notification Area'");

	g_signal_connect(G_OBJECT(status_icon), "popup-menu", G_CALLBACK(on_StatusIcon_popupmenu), NULL);
	g_signal_connect(G_OBJECT(status_icon), "activate", G_CALLBACK(on_StatusIcon_activate), NULL);
	
	gladexml_tray_menu = glade_xml_new (PACKAGE_DATA_DIR "/gnomegadu.glade", "TrayPopupMenu", NULL);
	glade_xml_signal_autoconnect (gladexml_tray_menu);
}
