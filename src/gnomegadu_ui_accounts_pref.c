/***************************************************************************
 *            gnomegadu_ui_accounts_pref.c
 *
 *  Tue Aug  8 22:05:56 2006
 *  Copyright  2006  Marcin Krzyżanowski
 *  krzak@hakore.com
 ****************************************************************************/
#include "config.h"
#include "gnomegadu_ui_accounts_pref.h"
#include "gnomegadu_ui_register_account.h"
#include "gnomegadu_conf.h"

#include <string.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

/*
 * skasuj zaznaczone konto 
 */
void
on_DelAccountButton_clicked (GtkButton * button, gpointer user_date)
{
	GtkTreeView *accounts_tree_view;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *account_name;
	GList *selected_list = NULL;
	GList *selected_list_start = NULL;
	
	accounts_tree_view = GTK_TREE_VIEW (glade_xml_get_widget (gladexml_account_preferences, "AccountsPreferencesTreeView"));

	g_assert (accounts_tree_view);

	selection = gtk_tree_view_get_selection (accounts_tree_view);
	model = gtk_tree_view_get_model (accounts_tree_view);

	selected_list = gtk_tree_selection_get_selected_rows(selection,&model);
	selected_list_start = selected_list;
	while (selected_list)
	{
		GtkTreePath *treepath = selected_list->data;
		gtk_tree_model_get_iter(model,&iter,treepath);
		
		gtk_tree_model_get (model, &iter, UI_PREF_COLUMN_ACCOUNT_NAME, &account_name, -1);
		g_assert (account_name);

		if (gtk_list_store_iter_is_valid (GTK_LIST_STORE (model), &iter))
		{
			if (gnomegadu_conf_del_account (account_name))
			{
				gtk_list_store_remove (GTK_LIST_STORE (model), &iter);

				if (gtk_tree_model_get_iter_first (model, &iter))
					gtk_tree_selection_select_iter (selection, &iter);
			}
			g_free (account_name);
		}
		selected_list = g_list_next(selected_list);
	}
	
	g_list_foreach (selected_list_start, (GFunc)gtk_tree_path_free, NULL);
	g_list_free (selected_list_start);	
}

void
on_AddAccountButton_clicked (GtkButton * button, gpointer user_data)
{
	gnomegadu_ui_register_account_add();
}


/* modyfikacja nazwy konta */
void
accounts_edited_text_cb (GtkCellRendererText * cellrenderertext, gchar * path_string, gchar * new_text, gpointer model_arg)
{
	GtkTreeModel *model;
	GtkListStore *list_store;
	GtkTreePath *path;
	GtkTreeIter iter;
	gchar *prev_name;
	GtkTreeView *accounts_tree_view;


	if (g_utf8_strlen (new_text,-1) < 1)
		return;

	accounts_tree_view = GTK_TREE_VIEW (glade_xml_get_widget (gladexml_account_preferences, "AccountsPreferencesTreeView"));
	g_assert(accounts_tree_view);
	
	model = gtk_tree_view_get_model (accounts_tree_view);
	list_store = GTK_LIST_STORE (model);
	
	path = gtk_tree_path_new_from_string (path_string);
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, UI_PREF_COLUMN_ACCOUNT_NAME, &prev_name, -1);

	if (strcasecmp (prev_name, new_text) && gnomegadu_conf_rename_account (prev_name, new_text))
	{
		gtk_list_store_set (list_store, &iter, UI_PREF_COLUMN_ACCOUNT_NAME, new_text, -1);
	}

	gtk_tree_path_free (path);
}

void
gnomegadu_ui_accounts_pref_entries_sensitive (gboolean val)
{
	GtkWidget *uin_entry;
	GtkWidget *password_entry;
	GtkWidget *togglebutton;

	uin_entry = GTK_WIDGET (glade_xml_get_widget (gladexml_account_preferences, "AccountUINEntry"));
	gtk_widget_set_sensitive (uin_entry, val);

	password_entry = GTK_WIDGET (glade_xml_get_widget (gladexml_account_preferences, "AccountPasswordEntry"));
	gtk_widget_set_sensitive (password_entry, val);

	togglebutton = GTK_WIDGET (glade_xml_get_widget (gladexml_account_preferences, "CheckButtonDefaultAccount"));
	gtk_widget_set_sensitive (togglebutton, val);

}

void
account_changed_cb (GtkTreeSelection * selection, gpointer user_data)
{
	gchar *account_name;
	GtkTreeModel *model;
	GtkTreeIter iter;

	if (gtk_tree_selection_get_selected (selection, &model, &iter))
	{
		g_assert (model);

		gtk_tree_model_get (model, &iter, UI_PREF_COLUMN_ACCOUNT_NAME, &account_name, -1);
		g_assert (account_name);

		gnomegadu_ui_set_account_data (g_strdup (account_name));
		gnomegadu_ui_accounts_pref_entries_sensitive (TRUE);

		g_free (account_name);
	}
	else
	{
		gnomegadu_ui_accounts_pref_entries_sensitive (FALSE);
	}
}

void
on_AccountUINEntry_changed (GtkEditable * editable, gpointer user_data)
{
	GtkEntry *uin_entry = GTK_ENTRY (editable);
	gchar *account_name = gnomegadu_ui_get_selected_account_name ();

	gnomegadu_conf_set_account_uin (account_name, (gchar *)gtk_entry_get_text (uin_entry));
	g_free (account_name);
}

void
on_AccountPasswordEntry_changed (GtkEditable * editable, gpointer user_data)
{
	GtkEntry *password_entry = GTK_ENTRY (editable);
	gchar *account_name = gnomegadu_ui_get_selected_account_name ();

	if (account_name)
	{
		gnomegadu_conf_set_account_password (account_name, (gchar *)gtk_entry_get_text (password_entry));
	}
	else
	{

	}
	g_free (account_name);
}


/*
 * Close AccountPreferences dialog
 */
void
on_AccountPreferences_response (GtkDialog * dialog, gint response, gpointer user_data)
{
	if (response == GTK_RESPONSE_CLOSE)
	{
		gtk_widget_destroy (GTK_WIDGET (dialog));
	}
}

void
on_CheckButtonDefaultAccount_toggled (GtkToggleButton * togglebutton, gpointer user_data)
{
	gchar *account_name = account_name = gnomegadu_ui_get_selected_account_name ();
	gchar *default_account_name = gnomegadu_conf_get_default_account_name ();

	if (gtk_toggle_button_get_active (togglebutton))
	{
		gnomegadu_conf_set_default_account_name (account_name);
	}
	else if (default_account_name && !g_strcasecmp (default_account_name, account_name))
	{
		gnomegadu_conf_set_default_account_name (NULL);
	}

	g_free (default_account_name);
	g_free (account_name);
}


/* currently selected account */
/* return newly allocated string */
gchar *
gnomegadu_ui_get_selected_account_name ()
{
	GtkTreeView *accounts_tree_view;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *account_name = NULL;

	accounts_tree_view = GTK_TREE_VIEW (glade_xml_get_widget (gladexml_account_preferences, "AccountsPreferencesTreeView"));

	selection = gtk_tree_view_get_selection (accounts_tree_view);
	model = gtk_tree_view_get_model (accounts_tree_view);

	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
	{
		gtk_tree_model_get (model, &iter, UI_PREF_COLUMN_ACCOUNT_NAME, &account_name, -1);
		g_assert (account_name);
	}
	return account_name;
}

void
gnomegadu_ui_set_account_data (gchar * account_name)
{
	GtkLabel *account_name_entry;
	gchar *uin, *password;
	GtkEntry *uin_entry;
	GtkEntry *password_entry;
	GtkToggleButton *togglebutton;
	gchar *default_account_name;
	gchar *tmp;

	/* NAME label */
	account_name_entry = GTK_LABEL (glade_xml_get_widget (gladexml_account_preferences, "AccountTabNameLabel"));
	g_assert (account_name_entry);

	tmp = g_strconcat("<b>",account_name,"</b>",NULL);
	gtk_label_set_markup (account_name_entry, tmp);
	g_free(tmp);
	
	/* UIN */
	uin = gnomegadu_conf_get_account_uin (account_name);
	if (!uin)
		uin = g_strdup ("");

	uin_entry = GTK_ENTRY (glade_xml_get_widget (gladexml_account_preferences, "AccountUINEntry"));

	gtk_entry_set_text (uin_entry, g_strdup (uin));

	/* PASSWORD */
	password = gnomegadu_conf_get_account_password (account_name);
	if (!password)
		password = g_strdup ("");
	
	password_entry = GTK_ENTRY (glade_xml_get_widget (gladexml_account_preferences, "AccountPasswordEntry"));

	gtk_entry_set_text (password_entry, g_strdup (password));

	togglebutton = GTK_TOGGLE_BUTTON (glade_xml_get_widget (gladexml_account_preferences, "CheckButtonDefaultAccount"));

	default_account_name = gnomegadu_conf_get_default_account_name ();

	if (default_account_name && !g_strcasecmp (default_account_name, account_name))
	{
		gtk_toggle_button_set_active (togglebutton, TRUE);
	}
	else
	{
		gtk_toggle_button_set_active (togglebutton, FALSE);
	}

	g_free (default_account_name);
	g_free (uin);
	g_free (password);
}

void
gnomegadu_ui_accounts_pref_init_list ()
{
	GtkListStore *accounts_list_store;
	GtkTreeView *accounts_tree_view;
	GtkTreeSelection *accounts_selection;
	GtkCellRenderer *render_text;
	GtkTreeViewColumn *column;
	GtkTreeIter iter, iter_default;
	GSList *list, *list_start;
	gboolean set_default = FALSE;
	gboolean select_account = FALSE;
	gchar *account_name, *account_dir, *account_dir_name, *default_account_name;
	gboolean update = FALSE;

	accounts_tree_view = GTK_TREE_VIEW (glade_xml_get_widget (gladexml_account_preferences, "AccountsPreferencesTreeView"));

	accounts_list_store = GTK_LIST_STORE (gtk_tree_view_get_model (accounts_tree_view));
	if (!accounts_list_store)
	{
		update = FALSE;
		accounts_list_store = gtk_list_store_new (UI_PREF_N_COLUMNS, G_TYPE_STRING);
	}
	else
	{
		update = TRUE;
		gtk_list_store_clear (accounts_list_store);
	}

	default_account_name = gnomegadu_conf_get_default_account_name ();

	list = gnomegadu_conf_get_accounts ();
	list_start = list;
	while (list)
	{
		account_dir = list->data;
		account_dir_name = g_strconcat (account_dir, "/name", NULL);

		account_name = gconf_client_get_string (gconf, account_dir_name, NULL);

		g_free (account_dir);
		g_free (account_dir_name);

		if (account_name)
		{
			select_account = TRUE;
			gtk_list_store_append (accounts_list_store, &iter);
			gtk_list_store_set (accounts_list_store, &iter, UI_PREF_COLUMN_ACCOUNT_NAME, account_name,	//TODO g_strdup ????
					    -1);

			if (default_account_name && !g_strcasecmp (default_account_name, account_name))
			{
				iter_default = iter;
				set_default = TRUE;
			}

			g_free (account_name);
		}

		list = list->next;
	}

	g_slist_free (list_start);
	//g_slist_foreach (list_start, gnomegadu_conf_free_list_of_string, NULL);

	accounts_selection = gtk_tree_view_get_selection (accounts_tree_view);
	
	if (!update)
	{
		render_text = gtk_cell_renderer_text_new ();
		g_object_set (G_OBJECT (render_text), "editable", TRUE, NULL);

		column = gtk_tree_view_column_new_with_attributes ("Nazwa konta", render_text, "markup", UI_PREF_COLUMN_ACCOUNT_NAME, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (accounts_tree_view), column);
		
		g_signal_connect (G_OBJECT (accounts_selection), "changed", G_CALLBACK (account_changed_cb), NULL);
		g_signal_connect (G_OBJECT (render_text), "edited", G_CALLBACK (accounts_edited_text_cb),NULL);

	}

	gtk_tree_view_set_model (accounts_tree_view, GTK_TREE_MODEL (accounts_list_store));

	if (set_default)
	{
		gtk_tree_selection_select_iter (accounts_selection, &iter_default);
	}
	else if (select_account)
	{
		gtk_tree_selection_select_iter (accounts_selection, &iter);
	}

	g_free (default_account_name);
	
	if (!update)
		g_object_unref (accounts_list_store);
}


    /*
     * Open AccountPreferences dialog
     */
gboolean
on_AccountPreferences_activate (GtkWidget * widget, GdkEvent * event, gpointer user_data)
{

	gladexml_account_preferences = glade_xml_new (PACKAGE_DATA_DIR "/gnomegadu.glade", "AccountPreferences", NULL);
	gnomegadu_ui_accounts_pref_init_list ();
	glade_xml_signal_autoconnect (gladexml_account_preferences);
	return TRUE;
}
