#include "config.h"
#include "gnomegadu_conf.h"
#include "gnomegadu_ui_user_add.h"
#include "gnomegadu_protocol.h"
#include "gnomegadu_userlist.h"

#include <string.h>
#include <uuid/uuid.h>

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <libgnomeui/libgnomeui.h>

#include <libgadu.h>

gboolean
on_ContactAdd_activate (GtkWidget * widget, GdkEvent * event, gpointer user_data)
{
	GtkListStore *group_store = NULL;
	GtkTreeIter group_iter;
	GtkComboBoxEntry *GroupComboBoxEntry;
	
	gladexml_user_add = glade_xml_new (PACKAGE_DATA_DIR "/gnomegadu.glade", "AddContact", NULL);
	GroupComboBoxEntry = GTK_COMBO_BOX_ENTRY (glade_xml_get_widget (gladexml_user_add, "GroupComboBoxEntry"));
	

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

		    groups = g_list_next(groups);
	}
		
	g_list_foreach (groups_start, gnomegadu_conf_free_list_of_string, NULL);
	g_list_free(groups_start);

	
	glade_xml_signal_autoconnect (gladexml_user_add);
	return TRUE;
}

void
on_AddContact_response (GtkDialog * dialog, gint response, gpointer user_data)
{
	if (response == GTK_RESPONSE_OK)
	{
		GConfChangeSet *changeset = NULL;
		GtkEntry *displayEntry = GTK_ENTRY (glade_xml_get_widget (gladexml_user_add, "DisplayEntry"));
		GtkEntry *firstNameEntry = GTK_ENTRY (glade_xml_get_widget (gladexml_user_add, "FirstNameEntry"));
		GtkEntry *lastNameEntry = GTK_ENTRY (glade_xml_get_widget (gladexml_user_add, "LastNameEntry"));
		GtkEntry *nickNameEntry = GTK_ENTRY (glade_xml_get_widget (gladexml_user_add, "NicknameEntry"));
		GtkEntry *UINEntry = GTK_ENTRY (glade_xml_get_widget (gladexml_user_add, "UINEntry"));
		GtkComboBoxEntry *GroupComboBoxEntry = GTK_COMBO_BOX_ENTRY (glade_xml_get_widget (gladexml_user_add, "GroupComboBoxEntry"));
		gchar *display = g_strdup (gtk_entry_get_text (displayEntry));
		gchar *firstName = g_strdup (gtk_entry_get_text (firstNameEntry));
		gchar *lastName = g_strdup (gtk_entry_get_text (lastNameEntry));
		gchar *nickName = g_strdup (gtk_entry_get_text (nickNameEntry));
		gchar *group = gtk_combo_box_get_active_text(GTK_COMBO_BOX(GroupComboBoxEntry));
		gchar *uin = g_strdup (gtk_entry_get_text (UINEntry));
		gchar *profile = gnomegadu_conf_get_profile ();
		gchar *profile_path = gnomegadu_conf_find_account_path (profile);
		gchar *conf_path = NULL;
		gchar *tmp = NULL;
		gchar *uuid_str = (gchar *) g_malloc (37);
		uuid_t uuid;

		if (g_utf8_strlen (display, -1) == 0)
		{
			GtkDialog *msgdialog = gtk_message_dialog_new_with_markup (NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
										   "<span weight=\"bold\"size=\"larger\">Niepoprawnie wypełnione pola</span>\n\nWyświelana nazwa nie mogła zostać ustalona, spróbuj jeszcze raz.");
			gtk_dialog_run (GTK_DIALOG (msgdialog));
			gtk_widget_destroy (GTK_WIDGET (msgdialog));
			goto end;
		}
		
		uuid_generate (uuid);
		uuid_unparse (uuid, uuid_str);

		conf_path = g_strconcat (profile_path, "/contacts/", uuid_str, NULL);
		
		changeset =  gconf_change_set_new();

		tmp = g_strconcat (conf_path, "/0uuid", NULL);
		gconf_change_set_set_string (changeset, tmp, g_strdup (uuid_str));
		g_free (tmp);
		
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

		tmp = g_strconcat (conf_path, "/uin", NULL);
		gconf_change_set_set_string (changeset, tmp, uin);	//moze tu trzeba g_strdup ??
		g_free (tmp);

		
		if (gconf_client_commit_change_set(gconf,changeset,TRUE,NULL))
		{
		    gnomegadu_protocol_add_notify(uin);
		}
		
		gconf_change_set_unref(changeset);
		
		gconf_client_suggest_sync(gconf,NULL);
		gtk_widget_destroy (GTK_WIDGET (dialog));

	      end:

		g_free (firstName);
		g_free (lastName);
		g_free (nickName);
		g_free (display);
		g_free (group);
		g_free (uin);

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
on_AddContact_Entry_changed (GtkEditable * editable, gpointer user_data)
{
	GtkEntry *displayEntry = GTK_ENTRY(glade_xml_get_widget (gladexml_user_add, "DisplayEntry"));
	GtkEntry *firstNameEntry = GTK_ENTRY (glade_xml_get_widget (gladexml_user_add, "FirstNameEntry"));
	GtkEntry *lastNameEntry = GTK_ENTRY (glade_xml_get_widget (gladexml_user_add, "LastNameEntry"));
	GtkEntry *nickNameEntry = GTK_ENTRY (glade_xml_get_widget (gladexml_user_add, "NicknameEntry"));
	gchar *display = g_strdup (gtk_entry_get_text (displayEntry));
	gchar *firstName = g_strdup (gtk_entry_get_text (firstNameEntry));
	gchar *lastName = g_strdup (gtk_entry_get_text (lastNameEntry));
	gchar *nickName = g_strdup (gtk_entry_get_text (nickNameEntry));
	gchar *new_display = NULL;
	
	if (!nickName || (nickName && g_utf8_strlen (nickName,-1) == 0))
		new_display = g_strconcat (firstName, (g_utf8_strlen(lastName,-1) > 0) ? " " : "", lastName, NULL);
	else
		new_display = g_strdup (nickName);

	gtk_entry_set_text (displayEntry, new_display);

	g_free(new_display);
	g_free (firstName);
	g_free (lastName);
	g_free (nickName);
	g_free (display);
}
