#ifndef GNOME_GADU_UI_ACCOUNTS_PREF_H
#define GNOME_GADU_UI_ACCOUNTS_PREF_H 1

#include <gtk/gtk.h>
#include <glade/glade.h>

GladeXML *gladexml_account_preferences;

enum
{
	UI_PREF_COLUMN_ACCOUNT_NAME,
	UI_PREF_N_COLUMNS
};

gchar *gnomegadu_ui_get_selected_account_name();
void gnomegadu_ui_set_account_data (gchar * account_name);
void gnomegadu_ui_accounts_pref_entries_sensitive(gboolean val);
void gnomegadu_ui_accounts_pref_init_list();

#endif
