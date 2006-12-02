#include "config.h"
#include "gnomegadu_conf.h"
#include "gnomegadu_ui.h"
#include "gnomegadu_ui_user_edit.h"
#include "gnomegadu_protocol.h"
#include "gnomegadu_userlist.h"

#include <string.h>
#include <uuid/uuid.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <libgnomeui/libgnomeui.h>


void
on_EditContact_response (GtkDialog * dialog, gint response, gpointer user_data)
{
	if (response == GTK_RESPONSE_OK)
	{
		GConfChangeSet *changeset = NULL;
		GtkEntry *displayEntry = GTK_ENTRY (glade_xml_get_widget (gladexml_user_edit, "DisplayEntry"));
		GtkEntry *firstNameEntry = GTK_ENTRY (glade_xml_get_widget (gladexml_user_edit, "FirstNameEntry"));
		GtkEntry *lastNameEntry = GTK_ENTRY (glade_xml_get_widget (gladexml_user_edit, "LastNameEntry"));
		GtkEntry *nickNameEntry = GTK_ENTRY (glade_xml_get_widget (gladexml_user_edit, "NicknameEntry"));
		GtkEntry *UINEntry = GTK_ENTRY (glade_xml_get_widget (gladexml_user_edit, "UINEntry"));
		GtkComboBoxEntry *GroupComboBoxEntry = GTK_COMBO_BOX_ENTRY (glade_xml_get_widget (gladexml_user_edit, "GroupComboBoxEntry"));
		gchar *display = g_strdup (gtk_entry_get_text (displayEntry));
		gchar *firstName = g_strdup (gtk_entry_get_text (firstNameEntry));
		gchar *lastName = g_strdup (gtk_entry_get_text (lastNameEntry));
		gchar *nickName = g_strdup (gtk_entry_get_text (nickNameEntry));
		gchar *uin = g_strdup (gtk_entry_get_text (UINEntry));
		gchar *group = gtk_combo_box_get_active_text(GTK_COMBO_BOX(GroupComboBoxEntry));
		gchar *uin_prev = NULL;
		gchar *conf_path = NULL;
		gchar *tmp = NULL;
		gchar *uuid_str = NULL;
		gchar *profile = gnomegadu_conf_get_profile ();
		gchar *profile_path = gnomegadu_conf_find_account_path (profile);

		uuid_str = g_object_get_data (G_OBJECT (gladexml_user_edit), "0uuid");

		if (g_utf8_strlen (display, -1) == 0)
		{
			GtkDialog *msgdialog = gtk_message_dialog_new_with_markup (NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
										   "<span weight=\"bold\"size=\"larger\">Niepoprawnie wypełnione pola</span>\n\nWyświelana nazwa nie mogła zostać ustalona, spróbuj jeszcze raz.");
			gtk_dialog_run (GTK_DIALOG (msgdialog));
			gtk_widget_destroy (GTK_WIDGET (msgdialog));
			goto end;
		}

		conf_path = g_strconcat (profile_path, "/contacts/", uuid_str, NULL);

		changeset =  gconf_change_set_new();

		tmp = g_strconcat (conf_path, "/1group", NULL);
		gconf_change_set_set_string (changeset, tmp, group);	//moze tu trzeba g_strdup ??
		g_free (tmp);

		tmp = g_strconcat (conf_path, "/first_name", NULL);
		gconf_change_set_set_string (changeset, tmp, firstName);	//moze tu trzeba g_strdup ??
		g_free (tmp);

		tmp = g_strconcat (conf_path, "/last_name", NULL);
		gconf_change_set_set_string (changeset, tmp, lastName);	//moze tu trzeba g_strdup ??
		g_free (tmp);

		tmp = g_strconcat (conf_path, "/nickname", NULL);
		gconf_change_set_set_string (changeset, tmp, nickName);	//moze tu trzeba g_strdup ??
		g_free (tmp);

		tmp = g_strconcat (conf_path, "/display", NULL);
		gconf_change_set_set_string (changeset, tmp, display);	//moze tu trzeba g_strdup ??
		g_free (tmp);

		/* change uin if changed */
		tmp = g_strconcat (conf_path, "/uin", NULL);
		uin_prev = gconf_client_get_string (gconf, tmp, NULL);

		if (uin_prev && g_strcasecmp (uin_prev, uin))
			gnomegadu_protocol_remove_notify (uin_prev);

		gconf_change_set_set_string (changeset, tmp, uin);	//moze tu trzeba g_strdup ??

		gconf_client_commit_change_set(gconf,changeset,TRUE,NULL);
		gconf_change_set_unref(changeset);

		if (uin && g_strcasecmp (uin_prev, uin))
			gnomegadu_protocol_add_notify (uin);

		g_free (tmp);
		/* <- */

		gconf_client_suggest_sync (gconf, NULL);
		gtk_widget_destroy (GTK_WIDGET (dialog));

	      end:

		g_free (firstName);
		g_free (lastName);
		g_free (nickName);
		g_free (display);
		g_free (uin);
		g_free (group);

		g_free (conf_path);
		g_free (profile);
		g_free (profile_path);
		g_free (uuid_str);
	}
	else
	{
		gtk_widget_destroy (GTK_WIDGET (dialog));
	}
}

void
on_EditContact_Entry_changed (GtkEditable * editable, gpointer user_data)
{
}


gboolean
on_ContactEdit_activate (GtkWidget * widget, GdkEvent * event, gpointer user_data)
{
	GtkTreeStore *contacts_tree_store;
	GtkTreeView *contacts_tree_view;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	gchar *uuid = NULL;
	gchar *path = NULL;
	gchar *tmp = NULL;
	GList *selected_list = NULL;

	contacts_tree_view = GTK_TREE_VIEW (glade_xml_get_widget (gladexml, "ContactsTreeView"));
	contacts_tree_store = GTK_TREE_STORE (gtk_tree_view_get_model (contacts_tree_view));
	selection = gtk_tree_view_get_selection (contacts_tree_view);

	if (selection && gtk_tree_selection_count_selected_rows (selection) > 0)
	{
		GtkListStore *group_store = NULL;
		GtkTreeIter group_iter;
		GtkTreePath *treepath;

		selected_list = gtk_tree_selection_get_selected_rows (selection, NULL);

		treepath = selected_list->data;
		if (!selected_list || !treepath)
			return TRUE;

		gtk_tree_model_get_iter (GTK_TREE_MODEL (contacts_tree_store), &iter, treepath);

		gladexml_user_edit = glade_xml_new (PACKAGE_DATA_DIR "/gnomegadu.glade", "EditContact", NULL);
		//ugly layout here hehe
		GtkEntry *displayEntry = GTK_ENTRY (glade_xml_get_widget (gladexml_user_edit, "DisplayEntry"));
		GtkEntry *firstNameEntry = GTK_ENTRY (glade_xml_get_widget (gladexml_user_edit, "FirstNameEntry"));
		GtkEntry *lastNameEntry = GTK_ENTRY (glade_xml_get_widget (gladexml_user_edit, "LastNameEntry"));
		GtkEntry *nickNameEntry = GTK_ENTRY (glade_xml_get_widget (gladexml_user_edit, "NicknameEntry"));
		GtkEntry *UINEntry = GTK_ENTRY (glade_xml_get_widget (gladexml_user_edit, "UINEntry"));
		GtkComboBoxEntry *GroupComboBoxEntry = GTK_COMBO_BOX_ENTRY (glade_xml_get_widget (gladexml_user_edit, "GroupComboBoxEntry"));
		
		gtk_tree_model_get (GTK_TREE_MODEL (contacts_tree_store), &iter, UI_CONTACTS_COLUMN_UUID, &uuid, -1);
		g_assert (uuid);

		g_object_set_data (G_OBJECT (gladexml_user_edit), "0uuid", g_strdup (uuid));
		path = gnomegadu_conf_contact_path_find_uuid (uuid);
		g_free (uuid);

		group_store = gtk_list_store_new (1, G_TYPE_STRING);
		gtk_combo_box_set_model (GTK_COMBO_BOX(GroupComboBoxEntry), GTK_TREE_MODEL (group_store));
		gtk_combo_box_entry_set_text_column(GroupComboBoxEntry,0);
		
		GList *groups = gnomegadu_userlist_get_groups();
		GList *groups_start = groups;
		while (groups)
		{
		    gchar *group_user = NULL;
		    gchar *group_name = groups->data;
		    
		    gtk_list_store_append(group_store,&group_iter);
		    gtk_list_store_set (group_store, &group_iter, 0,g_strdup(group_name), -1);

		    tmp = g_strconcat (path, "/1group", NULL);
		    group_user = gconf_client_get_string (gconf, tmp, NULL);

		    if (group_user && (g_utf8_strlen(group_user,-1) > 0) && !g_utf8_collate(group_user,group_name))
			gtk_combo_box_set_active_iter(GTK_COMBO_BOX(GroupComboBoxEntry), &group_iter);

		    g_free (group_user);
		    g_free (tmp);
		    
		    groups = g_list_next(groups);
		}
		
		g_list_foreach (groups_start, gnomegadu_conf_free_list_of_string, NULL);
		g_list_free(groups_start);
		

		tmp = g_strconcat (path, "/first_name", NULL);
		gtk_entry_set_text (firstNameEntry, gconf_client_get_string (gconf, tmp, NULL));
		g_free (tmp);

		tmp = g_strconcat (path, "/last_name", NULL);
		gtk_entry_set_text (lastNameEntry, gconf_client_get_string (gconf, tmp, NULL));
		g_free (tmp);

		tmp = g_strconcat (path, "/nickname", NULL);
		gtk_entry_set_text (nickNameEntry, gconf_client_get_string (gconf, tmp, NULL));
		g_free (tmp);

		tmp = g_strconcat (path, "/uin", NULL);
		gtk_entry_set_text (UINEntry, gconf_client_get_string (gconf, tmp, NULL));
		g_free (tmp);

		tmp = g_strconcat (path, "/display", NULL);
		gtk_entry_set_text (displayEntry, gconf_client_get_string (gconf, tmp, NULL));
		g_free (tmp);


		glade_xml_signal_autoconnect (gladexml_user_edit);
		g_free (path);

		g_list_foreach (selected_list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (selected_list);
	}
	else
	{
		GtkDialog *msgdialog = gtk_message_dialog_new_with_markup (NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
									   "<span weight=\"bold\"size=\"larger\">Nie wybrałeś pozycji do edycji</span>\n\nAby zaktualizować pozycję na liście kontaktów należy zaznaczyć wybraną pozycję na liście, następnie wybrać funkcję edycji.");
		gtk_dialog_run (GTK_DIALOG (msgdialog));
		gtk_widget_destroy (GTK_WIDGET (msgdialog));
	}

	return TRUE;


}
