#ifndef GNOME_GADU_UI_IMPORT_USERLIST_H
#define GNOME_GADU_UI_IMPORT_USERLIST_H 1

#include <gtk/gtk.h>
#include <glade/glade.h>

GladeXML *gladexml_import_userlist;
GladeXML *gladexml_import_userlist_progress;


void gnomegadu_ui_import_userlist_process_line (gchar * line, GConfChangeSet *changeset);

#endif
