#ifndef GNOME_GADU_UI_H
#define GNOME_GADU_UI_H 1

#include <gtk/gtk.h>
#include <glade/glade.h>


#define EMPTY_GROUP "Bez grupy"

#define USER_AVAIL_ICON "user_orange.png"
#define USER_AWAY_ICON "user_orange_away.png"
#define USER_NOTAVAIL_ICON "user.png"
#define USER_INVISIBLE_ICON "user_red.png"
#define USER_GROUP_ICON "group.png"
#define ADD_DESCRIPTION_ICON "font_add.png"

#define DISPLAYED_MARKUP "%s\n<span size=\"smaller\" style=\"italic\" weight=\"ultralight\" foreground=\"#999999\">%s</span>"

GladeXML *gladexml;
GladeXML *gladexml_menu;

enum
{
	UI_CONTACTS_COLUMN_ICON,
	UI_CONTACTS_COLUMN_DISPLAYED,
	UI_CONTACTS_COLUMN_UUID,
	UI_CONTACTS_COLUMN_STATUS,
	UI_CONTACTS_COLUMN_STATUS_DESCR,
	UI_CONTACTS_COLUMN_IS_GROUP,
	UI_CONTACTS_N_COLUMNS
};

enum
{
	UI_STATUS_COLUMN_ICON,
	UI_STATUS_COLUMN_NAME,
	UI_STATUS_COLUMN_STATUS,
	UI_STATUS_N_COLUMNS
};


void gnomegadu_ui_init ();

gboolean on_MainWindow_delete_event (GtkWidget * widget, GdkEvent * event, gpointer user_data);
void     on_MainMenuQuit_activate (GtkWidget * widget, gpointer user_data);

gboolean on_AccountPreferences_activate (GtkWidget * widget, GdkEvent * event, gpointer user_data);
void     on_AccountPreferencesCloseButton_clicked (GtkButton * button);

GdkPixbuf *create_pixbuf (const gchar * filename);

void gnomegedu_ui_init_userlist ();
void gnomegadu_ui_init_contacts_treeview();
void gnomegadu_ui_init_statusbar();

void on_ContactsTreeView_cursor_changed (GtkTreeView * treeview, gpointer user_data);

#endif
