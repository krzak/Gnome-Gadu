/***************************************************************************
 *            gnomegadu_ui_accounts_pref.c
 *
 *  Tue Aug  8 22:05:56 2006
 *  Copyright  2006  Marcin Krzyżanowski
 *  krzak@hakore.com
 ****************************************************************************/
#include "config.h"
#include "gnomegadu_ui_account_chooser.h"
#include "gnomegadu_conf.h"

#include <string.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

void
on_ChooseAccountDialog_response (GtkDialog * dialog, gint response, gpointer user_data)
{
	gchar *account_name;
	GtkComboBox *combo;

	if (response == GTK_RESPONSE_OK)
	{
		combo = GTK_COMBO_BOX (glade_xml_get_widget (gladexml_account_chooser, "AccountsCombo"));
		account_name = gtk_combo_box_get_active_text (combo);

		gnomegadu_conf_set_profile (account_name);

		gtk_widget_destroy (GTK_WIDGET (dialog));
		g_free (account_name);
	}
}

void
on_ChooseAccountDialog_close (GtkDialog * dialog, gpointer user_data)
{
	on_ChooseAccountDialog_response(dialog,GTK_RESPONSE_OK,NULL);
}


void
gnomegadu_ui_account_chooser_activate ()
{
	GtkComboBox *combo;
	GSList *accounts, *accounts_start;
	gchar *account_name, *account_dir, *account_dir_name;


	gladexml_account_chooser = glade_xml_new (PACKAGE_DATA_DIR "/gnomegadu.glade", "ChooseAccountDialog", NULL);


	accounts = gnomegadu_conf_get_accounts ();
	accounts_start = accounts;

	if (accounts)
	{
		combo = GTK_COMBO_BOX (glade_xml_get_widget (gladexml_account_chooser, "AccountsCombo"));
		while (accounts)
		{
			account_dir = accounts->data;

			account_dir_name = g_strconcat (account_dir, "/name", NULL);
			account_name = gconf_client_get_string (gconf, account_dir_name, NULL);

			g_free (account_dir);
			g_free (account_dir_name);

			gtk_combo_box_append_text (combo, g_strdup (account_name));
			accounts = accounts->next;
		}
	}

	gtk_combo_box_set_active (combo, 0);

	g_slist_free (accounts_start);
	//g_slist_foreach (accounts_start, gnomegadu_conf_free_list_of_string, NULL);

	glade_xml_signal_autoconnect (gladexml_account_chooser);
}
