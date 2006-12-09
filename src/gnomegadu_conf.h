#ifndef GNOME_GADU_CONF_H
#define GNOME_GADU_CONF_H 1

#include <gconf/gconf-client.h>
#include <gnome-keyring-1/gnome-keyring.h>

#define GNOMEGADU_CONF_ROOT		"/apps/gnome-gadu"

GConfClient *gconf;

void gnomegadu_conf_init ();

gboolean  gnomegadu_conf_rename_account (gchar * old_name, gchar * new_name);
gchar    *gnomegadu_conf_add_account (gchar * name, gchar * uin, gchar * password);
gboolean  gnomegadu_conf_del_account (gchar * account_name);
GSList   *gnomegadu_conf_get_accounts ();

gchar *gnomegadu_conf_find_account_path (gchar * account_name);
void   gnomegadu_conf_free_list_of_string (gpointer data, gpointer user_data);

gchar   *gnomegadu_conf_get_account_uin (gchar * account_name);
gboolean gnomegadu_conf_set_account_uin (gchar * account_name, gchar * value_uin);

gchar   *gnomegadu_conf_get_account_password (gchar * account_name);
gboolean gnomegadu_conf_set_account_password (gchar * account_name, gchar * value_password);

gchar   *gnomegadu_conf_get_default_account_name ();
gboolean gnomegadu_conf_set_default_account_name (gchar * account_name);

gboolean gnomegadu_conf_set_profile (gchar * account_name);
gchar   *gnomegadu_conf_get_profile ();

GSList *gnomegadu_conf_get_contacts ();

gchar *gnomegadu_conf_contact_path_find_uin (gchar * uin);
gchar *gnomegadu_conf_contact_path_find_uuid (gchar * uuid);

gchar *gnomegadu_conf_contact_get_uin_for_uuid (gchar * uuid);
gchar *gnomegadu_conf_contact_get_display_for_uuid (gchar * uuid);
gchar *gnomegadu_conf_contact_get_uuid_for_uin (gchar * uin);

#endif
