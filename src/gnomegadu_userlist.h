#ifndef GNOME_GADU_USERLIST_H
#define GNOME_GADU_USERLIST_H 1

#include <gconf/gconf-client.h>
#include "gnomegadu_protocol.h"

gboolean on_ContactDelete_activate (GtkWidget * widget, GdkEvent * event, gpointer user_data);

void gconf_client_contacts_value_changed_cb (GConfClient * client, guint cnxn_id, GConfEntry * entry, gpointer user_data);

GnomeGaduProtocolStatus gnomegadu_userlist_get_model_status (gchar * uuid_search);

void gnomegadu_userlist_set_model_status (gchar * uin_str, GnomeGaduProtocolStatus new_status, gchar *status_descr);
void gnomegadu_userlist_cleanup_model_status ();

GList       *gnomegadu_userlist_get_groups();
GtkTreeIter *gnomegadu_userlist_group_find_iter(GtkTreeStore *treestore, gchar *group_name);

#endif
