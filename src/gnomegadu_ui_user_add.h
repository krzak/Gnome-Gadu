#ifndef GNOME_GADU_UI_USER_ADD_H
#define GNOME_GADU_UI_USER_ADD_H 1

#include <gtk/gtk.h>
#include <glade/glade.h>

GladeXML *gladexml_user_add;

gboolean on_ContactAdd_activate (GtkWidget * widget, GdkEvent * event, gpointer user_data);

#endif
