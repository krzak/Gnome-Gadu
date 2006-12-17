#include <gtk/gtk.h>

#include <string.h>
#include <uuid/uuid.h>

#include <glade/glade.h>
#include <libgnomeui/libgnomeui.h>

#include "config.h"
#include "gnomegadu_conf.h"
#include "gnomegadu_ui.h"
#include "gnomegadu_protocol.h"
#include "gnomegadu_ui_import_userlist.h"

gboolean
on_userlist_import_page_1_next (GnomeDruidPage * druidpage, GtkWidget * widget, gpointer user_data)
{
	GtkWidget *radio_import_server, *radio_import_file;
	GnomeDruid *druid = GNOME_DRUID (glade_xml_get_widget (gladexml_import_userlist, "ImportUserlistDruid"));
	GnomeDruidPage *page_last = GNOME_DRUID_PAGE (glade_xml_get_widget (gladexml_import_userlist, "userlist_import_page_last"));

	radio_import_server = glade_xml_get_widget (gladexml_import_userlist, "RadioImportServer");
	radio_import_file = glade_xml_get_widget (gladexml_import_userlist, "RadioImportFile");
	g_assert (radio_import_file);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (radio_import_server)))
	{
		GtkWidget *progress_window;

		if (!gnomegadu_gadugadu_session)
		{
			GtkDialog *msgdialog = gtk_message_dialog_new_with_markup (NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
										   "<span weight=\"bold\"size=\"larger\">Brak połączenia</span>\n\nPołącz się z siecią Gadu-Gadu i spróbuj ponownie.");
			gtk_dialog_run (GTK_DIALOG (msgdialog));
			gtk_widget_destroy (GTK_WIDGET (msgdialog));
			return TRUE;
		}

		gladexml_import_userlist_progress = glade_xml_new (PACKAGE_DATA_DIR "/gnomegadu.glade", "ImportUserlistServerProgressWindow", NULL);
		glade_xml_signal_autoconnect (gladexml_import_userlist_progress);

		g_assert (gladexml_import_userlist_progress);

		progress_window = glade_xml_get_widget (gladexml_import_userlist_progress, "ImportUserlistServerProgressWindow");
		g_assert(progress_window);
		gtk_widget_show_all (GTK_WIDGET(progress_window));

		gg_userlist_request (gnomegadu_gadugadu_session, GG_USERLIST_GET, NULL);

		gnome_druid_set_page (druid, page_last);
		gnome_druid_set_buttons_sensitive (druid, TRUE, FALSE, TRUE, FALSE);
		gnome_druid_set_show_finish (druid, TRUE);
	}
	else
	{
		//gtk_file_chooser_get_filename ()
		//gnomegadu_import_userlist_from_file();
	}
	return FALSE;
}

void
gnomegadu_ui_import_userlist_process_line (gchar * line, GConfChangeSet *changeset)
{
	gchar *profile = gnomegadu_conf_get_profile ();
	gchar *profile_path = gnomegadu_conf_find_account_path (profile);
	gchar *conf_path = NULL;
	gchar *tmp = NULL;
	gchar *uin_str = NULL;
	gchar **split_line = NULL;
	gchar *found_path = NULL;
	gchar *line_utf = NULL;
	gint i;
	uuid_t uuid;
	gchar *uuid_str = (gchar *) g_malloc (37);

	line_utf = g_convert (line, strlen (line), "UTF-8", "WINDOWS-1250", NULL, NULL, NULL);

	split_line = g_strsplit (line_utf, ";", 14);
	if (!split_line || !g_strcasecmp ("GG70ExportString,", split_line[0]) || !split_line[6])
	{
		goto end;
	}

	found_path = gnomegadu_conf_contact_path_find_uin (split_line[6]);
	if (!found_path)
	{
		uuid_generate (uuid);
		uuid_unparse (uuid, uuid_str);
		conf_path = g_strconcat (profile_path, "/contacts/", uuid_str, NULL);
		tmp = g_strconcat (conf_path, "/uuid", NULL);
		gconf_change_set_set_string (changeset, tmp, g_strdup (uuid_str));
		g_free (tmp);
	}
	else
	{
		conf_path = g_strdup (found_path);

		tmp = g_strconcat (conf_path, "/uuid", NULL);
		uuid_str = gconf_client_get_string (gconf, tmp, NULL);
		g_free (tmp);
		g_free (found_path);
	}

	for (i = 0; i < g_strv_length (split_line); i++)
	{
		switch (i)
		{
		case 0:	//imie
			tmp = g_strconcat (conf_path, "/first_name", NULL);
			gconf_change_set_set_string (changeset, tmp, g_strdup (split_line[i]));	//moze tu trzeba g_strdup ??
			g_free (tmp);
			break;
		case 1:	//nazwisko
			tmp = g_strconcat (conf_path, "/last_name", NULL);
			gconf_change_set_set_string (changeset, tmp, g_strdup (split_line[i]));	//moze tu trzeba g_strdup ??
			g_free (tmp);
			break;
		case 2:	//pseudo
			tmp = g_strconcat (conf_path, "/nickname", NULL);
			gconf_change_set_set_string (changeset, tmp, g_strdup (split_line[i]));	//moze tu trzeba g_strdup ??
			g_free (tmp);
			break;
		case 3:	//display
			tmp = g_strconcat (conf_path, "/display", NULL);
			gconf_change_set_set_string (changeset, tmp, g_strdup (split_line[i]));	//moze tu trzeba g_strdup ??
			g_free (tmp);
			break;
		case 5:	//group
			tmp = g_strconcat (conf_path, "/group", NULL);
			gconf_change_set_set_string (changeset, tmp, g_strdup (split_line[i]));	//moze tu trzeba g_strdup ??
			g_free (tmp);
			break;
		case 6:	//UIN
			tmp = g_strconcat (conf_path, "/uin", NULL);
			gconf_change_set_set_string (changeset, tmp, g_strdup (split_line[i]));	//moze tu trzeba g_strdup ??
			uin_str = g_strdup (split_line[i]);
			g_free (tmp);
			break;
		}
	}

	gnomegadu_protocol_add_notify (uin_str);

      end:
	g_strfreev (split_line);
	g_free (uin_str);
	g_free (line_utf);
	g_free (profile);
	g_free (profile_path);
	g_free (conf_path);
}

gboolean
on_userlist_import_page_2_next (GnomeDruidPage * druidpage, GtkWidget * widget, gpointer user_data)
{
	GtkWidget *file_chooser = glade_xml_get_widget (gladexml_import_userlist, "ImportFileChooser");
	GIOChannel *channel = NULL;
	gchar *line = NULL;
	gsize len, terminator_pos;
	gchar *filename = NULL;
	gsize status = -1;

	g_assert (file_chooser);

	filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_chooser));
	channel = g_io_channel_new_file (filename, "r", NULL);
	if (!channel)
	{
		GtkDialog *msgdialog = gtk_message_dialog_new_with_markup (NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
									   "<span weight=\"bold\"size=\"larger\">Plik nie został wybrany</span>\n\nWymagane pola nie zostały wypełnione prawidłowo, spróbuj jeszcze raz.");
		gtk_dialog_run (GTK_DIALOG (msgdialog));
		gtk_widget_destroy (GTK_WIDGET (msgdialog));
		goto end;
	}

	g_io_channel_set_encoding (channel, NULL, NULL);
	
	GConfChangeSet *changeset = gconf_change_set_new();

	while (status != G_IO_STATUS_ERROR && status != G_IO_STATUS_EOF)
	{
		status = g_io_channel_read_line (channel, &line, &len, &terminator_pos, NULL);

		if (status == G_IO_STATUS_ERROR || status == G_IO_STATUS_EOF)
		{
			g_free (line);
			continue;
		}

		gnomegadu_ui_import_userlist_process_line (line, changeset);
		
		if (!gconf_client_commit_change_set(gconf, changeset, TRUE, NULL))
		    g_printerr("Some error while import");
		    
		g_free (line);
	}

	gconf_change_set_unref(changeset);

	gconf_client_suggest_sync (gconf, NULL);
	g_io_channel_unref (channel);

      end:
	g_free (filename);
	return FALSE;
}


gboolean
on_userlist_import_page_last_back (GnomeDruidPage * druidpage, GtkWidget * widget, gpointer user_data)
{
	GtkWidget *radio_import_server, *radio_import_file;
	GnomeDruid *druid = GNOME_DRUID (glade_xml_get_widget (gladexml_import_userlist, "ImportUserlistDruid"));
	GnomeDruidPage *page1 = GNOME_DRUID_PAGE (glade_xml_get_widget (gladexml_import_userlist, "userlist_import_page_1"));

	radio_import_server = glade_xml_get_widget (gladexml_import_userlist, "RadioImportServer");
	radio_import_file = glade_xml_get_widget (gladexml_import_userlist, "RadioImportFile");

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (radio_import_server)))
	{
		gnome_druid_set_page (druid, page1);
		return TRUE;
	}

	return FALSE;
}

void
on_ImportUserlistDruid_cancel (GnomeDruid * druid, gpointer user_data)
{
	GtkWindow *window = GTK_WINDOW (glade_xml_get_widget (gladexml_import_userlist, "ImportUserlist"));
	gtk_widget_destroy (GTK_WIDGET (window));
}

void
on_userlist_import_page_last_finish (GnomeDruid * druid, gpointer user_data)
{
	GtkWindow *window = GTK_WINDOW (glade_xml_get_widget (gladexml_import_userlist, "ImportUserlist"));
	gtk_widget_destroy (GTK_WIDGET (window));
}


gboolean
on_ContactsImport_activate (GtkWidget * widget, GdkEvent * event, gpointer user_data)
{
	GnomeDruid *druid;
	gladexml_import_userlist = glade_xml_new (PACKAGE_DATA_DIR "/gnomegadu.glade", "ImportUserlist", NULL);

	druid = GNOME_DRUID (glade_xml_get_widget (gladexml_import_userlist, "ImportUserlistDruid"));

	gnome_druid_set_buttons_sensitive (druid, FALSE, TRUE, TRUE, FALSE);
	glade_xml_signal_autoconnect (gladexml_import_userlist);
	gtk_widget_show_all (GTK_WIDGET (druid));
	return TRUE;
}
