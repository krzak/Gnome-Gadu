#ifndef GNOME_GADU_UI_USER_EDIT_H
#define GNOME_GADU_UI_USER_EDIT_H 1

#include <gtk/gtk.h>
#include <glade/glade.h>

GladeXML *gladexml_user_edit;

gboolean on_ContactEdit_activate (GtkWidget * widget, GdkEvent * event, gpointer user_data);

#endif
