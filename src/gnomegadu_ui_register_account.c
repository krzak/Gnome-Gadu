#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <libgnomeui/libgnomeui.h>

#include "config.h"
#include "gnomegadu_ui_register_account.h"
#include "gnomegadu_ui_accounts_pref.h"
#include "gnomegadu_conf.h"
#include "libgadu.h"

void
on_RegisterNewAccountDialog_response (GtkDialog * dialog, gint response, gpointer user_data)
{


	if (response == GTK_RESPONSE_CANCEL)
	{
		//TODO remove tmp image
		gtk_widget_destroy (GTK_WIDGET (dialog));
	}
	else if (response == GTK_RESPONSE_OK)
	{
	}
}


void
gnomegadu_ui_register_account_update_token ()
{
	struct gg_http *token_http;
	struct gg_token *token;
	GIOChannel *ch;
	gchar *token_image_path;
	GtkImage *image;
	GtkEntry *token_entry;

	token_entry = GTK_ENTRY (glade_xml_get_widget (gladexml_account_register, "TokenEntry"));
	gtk_entry_set_text (token_entry, "");

	token_http = gg_token (FALSE);
	if (token_http)
	{
		token = token_http->data;
		if (token)
		{
			image = GTK_IMAGE (glade_xml_get_widget (gladexml_account_register, "TokenImage"));
			g_assert (image);

			token_image_path = g_build_filename (g_get_tmp_dir (), "gnome-gadu-register-token.gif", NULL);

			ch = g_io_channel_new_file (token_image_path, "w", NULL);
			g_assert (ch);

			g_io_channel_set_encoding (ch, NULL, NULL);
			g_io_channel_write_chars (ch, token_http->body, token_http->body_size, NULL, NULL);
			g_io_channel_shutdown (ch, TRUE, NULL);
			g_io_channel_unref (ch);

			gtk_image_set_from_file (image, token_image_path);

			g_free (tokenid);
			tokenid = g_strdup (token->tokenid);

			g_unlink (token_image_path);
			g_free (token_image_path);
		}
		gg_token_free (token_http);
	}

}

//gboolean
//on_RegisterNewAccount_activate (GtkWidget * widget, GdkEvent * event, gpointer user_data)

void
on_1_page_prepare (GnomeDruidPage * page, GnomeDruid * druid, gpointer user_data)
{
	gnome_druid_set_buttons_sensitive (druid, FALSE, TRUE, TRUE, FALSE);
}

gboolean
on_2_page_next (GnomeDruidPage * druidpage, GtkWidget * widget, gpointer user_data)
{
	GtkWidget *radio_new_account, *radio_existing_account;
	GnomeDruid *druid = GNOME_DRUID (glade_xml_get_widget (gladexml_account_register, "RegisterNewAccountDruid"));
	GnomeDruidPage *page3_2 = GNOME_DRUID_PAGE (glade_xml_get_widget (gladexml_account_register, "3_2_page"));


	radio_new_account = glade_xml_get_widget (gladexml_account_register, "NewAccountRadio");
	radio_existing_account = glade_xml_get_widget (gladexml_account_register, "ExistingAccountRadio");
	g_assert(radio_existing_account);
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(radio_existing_account)))
	{
		gnome_druid_set_page (druid, page3_2);
		gnome_druid_set_buttons_sensitive(druid,TRUE,FALSE,TRUE,FALSE);
		gnome_druid_set_show_finish(druid,TRUE);
	}
	return FALSE;
}

gboolean
on_3_2_page_back (GnomeDruidPage * druidpage, GtkWidget * widget, gpointer user_data)
{
	GnomeDruid *druid = GNOME_DRUID (glade_xml_get_widget (gladexml_account_register, "RegisterNewAccountDruid"));
	GnomeDruidPage *page3 = GNOME_DRUID_PAGE (glade_xml_get_widget (gladexml_account_register, "2_page"));

	gnome_druid_set_page(druid,page3);
	return TRUE;
}

void
on_3_2_page_finish (GnomeDruidPage * druidpage, GtkWidget * widget, gpointer user_data)
{
	GtkDialog *msgdialog;
	GtkEntry *uin_entry, *password_entry, *name_entry;
	const gchar *uin, *password, *name;
	GtkWindow *window = GTK_WINDOW (glade_xml_get_widget (gladexml_account_register, "RegisterNewAccountWindow"));

	uin_entry = GTK_ENTRY (glade_xml_get_widget (gladexml_account_register, "ExistingUINEntry"));
	uin = gtk_entry_get_text (uin_entry);
	
	name_entry = GTK_ENTRY (glade_xml_get_widget (gladexml_account_register, "ExistingNameEntry"));
	name = gtk_entry_get_text (name_entry);

	password_entry = GTK_ENTRY (glade_xml_get_widget (gladexml_account_register, "ExistingPasswordEntry"));
	password = gtk_entry_get_text (password_entry);

	if (!uin || !name || !password || g_utf8_strlen (uin,-1) <= 2 || g_utf8_strlen (name,-1) <= 1 || g_utf8_strlen(password,-1) <= 2)
	{
		msgdialog =
			gtk_message_dialog_new_with_markup (NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
							    "<span weight=\"bold\"size=\"larger\">Pola nie są wypełnione prawidłowo</span>\n\nWymagane pola nie zostały wypełnione prawidłowo, spróbuj jeszcze raz.");
		gtk_dialog_run (GTK_DIALOG (msgdialog));
		gtk_widget_destroy (GTK_WIDGET (msgdialog));
		return;
	}
	
	// add newly created account to accounts
	gnomegadu_conf_add_account ((gchar *)name, uin, (gchar *) password);
	gnomegadu_ui_accounts_pref_init_list ();

	gtk_widget_destroy(GTK_WIDGET(window));
}



void
on_3_1_page_prepare (GnomeDruidPage * page, GnomeDruid * druid, gpointer user_data)
{
	gnomegadu_ui_register_account_update_token ();
}

void
on_5_1_page_prepare (GnomeDruidPage * page, GnomeDruid * druid, gpointer user_data)
{
	GtkLabel *label;
	label = GTK_LABEL (glade_xml_get_widget (gladexml_account_register, "UINLabel"));

	if (new_registered_uin)
		gtk_label_set_markup (label, g_strconcat ("<span weight=\"bold\" size=\"larger\">", new_registered_uin, "</span>", NULL));

	gnome_druid_set_show_finish (druid, TRUE);
}

gboolean
on_3_1_page_next (GnomeDruidPage * druidpage, GtkWidget * widget, gpointer user_data)
{
	GtkDialog *msgdialog;
	GtkEntry *password_entry, *token_entry;
	const gchar *password, *tokenval;
	gboolean ret = FALSE;

	password_entry = GTK_ENTRY (glade_xml_get_widget (gladexml_account_register, "PasswordEntry"));
	password = gtk_entry_get_text (password_entry);

	token_entry = GTK_ENTRY (glade_xml_get_widget (gladexml_account_register, "TokenEntry"));
	tokenval = gtk_entry_get_text (token_entry);

	if (!password || !tokenval || g_utf8_strlen (password,-1) <= 3 || g_utf8_strlen (tokenval,-1) <= 4)
	{
		msgdialog =
			gtk_message_dialog_new_with_markup (NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
							    "<span weight=\"bold\"size=\"larger\">Pola nie są wypełnione prawidłowo</span>\n\nWymagane pola nie zostały wypełnione prawidłowo, spróbuj jeszcze raz.");
		gtk_dialog_run (GTK_DIALOG (msgdialog));
		gtk_widget_destroy (GTK_WIDGET (msgdialog));
		ret = TRUE;
	}

	return ret;
}

gboolean
on_4_1_page_next (GnomeDruidPage * druidpage, GtkWidget * widget, gpointer user_data)
{
	GtkDialog *msgdialog;
	GtkEntry *email_entry, *name_entry;
	GtkEntry *password_entry, *token_entry;
	const gchar *email, *tokenval, *password, *name;
	gboolean ret = TRUE;

	password_entry = GTK_ENTRY (glade_xml_get_widget (gladexml_account_register, "PasswordEntry"));
	password = gtk_entry_get_text (password_entry);

	token_entry = GTK_ENTRY (glade_xml_get_widget (gladexml_account_register, "TokenEntry"));
	tokenval = gtk_entry_get_text (token_entry);

	email_entry = GTK_ENTRY (glade_xml_get_widget (gladexml_account_register, "EmailEntry"));
	email = gtk_entry_get_text (email_entry);

	name_entry = GTK_ENTRY (glade_xml_get_widget (gladexml_account_register, "NameEntry"));
	name = gtk_entry_get_text (name_entry);

	if ((!email || !name || g_utf8_strlen (email,-1) <= 3 || g_utf8_strlen (name,-1) <= 1) || (!password || !tokenval || g_utf8_strlen (password,-1) <= 3 || g_utf8_strlen (tokenval,-1) <= 4))
	{
		msgdialog =
			gtk_message_dialog_new_with_markup (NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
							    "<span weight=\"bold\"size=\"larger\">Pola nie są wypełnione prawidłowo</span>\n\nWymagane pola nie zostały wypełnione prawidłowo, spróbuj jeszcze raz.");
		gtk_dialog_run (GTK_DIALOG (msgdialog));
		gtk_widget_destroy (GTK_WIDGET (msgdialog));
		ret = TRUE;
	}
	else
	{
		struct gg_http *reg;
		struct gg_pubdir *pubdir;

		reg = gg_register3 (email, password, tokenid, tokenval, FALSE);

		if (reg && reg->data)
		{
			pubdir = (struct gg_pubdir *) reg->data;

			if (pubdir && pubdir->success && pubdir->uin)
			{
				/*
				 * msgdialog =
				 * gtk_message_dialog_new_with_markup (NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO,
				 * GTK_BUTTONS_CLOSE,
				 * "<span weight=\"bold\"size=\"larger\">Rejestracja przebiegła pomyślnie</span>\n\nSystem Gadu-Gadu nadał numer UIN: <b>%d</b>. Zapisz ten numer ponieważ będzie Ci potrzebny do połączenia się z serwerem Gadu-Gadu.",
				 * pubdir->uin);
				 * gtk_dialog_run (GTK_DIALOG (msgdialog));
				 * gtk_widget_destroy (GTK_WIDGET (msgdialog));
				 */
				new_registered_uin = g_strdup_printf ("%d", pubdir->uin);
				ret = FALSE;
			}
			else
			{
				GnomeDruid *druid = GNOME_DRUID (glade_xml_get_widget (gladexml_account_register, "RegisterNewAccountDruid"));
				GnomeDruidPage *page3 = GNOME_DRUID_PAGE (glade_xml_get_widget (gladexml_account_register, "3_1_page"));

				msgdialog =
					gtk_message_dialog_new_with_markup (NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
									    GTK_BUTTONS_CLOSE,
									    "<span weight=\"bold\"size=\"larger\">Rejestracja nie powiodła się</span>\n\nSprawdź wpisane wartości i spróbuj jeszcze raz.");
				gtk_dialog_run (GTK_DIALOG (msgdialog));
				gtk_widget_destroy (GTK_WIDGET (msgdialog));
				gnomegadu_ui_register_account_update_token ();

				gnome_druid_set_page (druid, page3);
				ret = TRUE;
			}
		}
		gg_register_free (reg);
	}
	return ret;
}

void
on_5_1_page_finish (GnomeDruidPage * druidpage, GtkWidget * widget, gpointer user_data)
{
	GtkEntry *password_entry, *name_entry;
	const gchar *password, *name;
	GtkWindow *window = GTK_WINDOW (glade_xml_get_widget (gladexml_account_register, "RegisterNewAccountWindow"));

	password_entry = GTK_ENTRY (glade_xml_get_widget (gladexml_account_register, "PasswordEntry"));
	password = gtk_entry_get_text (password_entry);

	name_entry = GTK_ENTRY (glade_xml_get_widget (gladexml_account_register, "NameEntry"));
	name = gtk_entry_get_text (name_entry);

	// add newly created account to accounts
	gnomegadu_conf_add_account ((gchar *) name, new_registered_uin, (gchar *) password);
	gnomegadu_ui_accounts_pref_init_list ();

	gtk_widget_destroy (GTK_WIDGET (window));
}



void
on_RegisterNewAccountDruid_cancel (GnomeDruid * druid, gpointer user_data)
{
	GtkWindow *window = GTK_WINDOW (glade_xml_get_widget (gladexml_account_register, "RegisterNewAccountWindow"));
	gtk_widget_destroy (GTK_WIDGET (window));
}

void
gnomegadu_ui_register_account_add ()
{
	GnomeDruid *druid;
	gladexml_account_register = glade_xml_new (PACKAGE_DATA_DIR "/gnomegadu.glade", "RegisterNewAccountWindow", NULL);

	g_free (new_registered_uin);
	new_registered_uin = NULL;

	druid = GNOME_DRUID (glade_xml_get_widget (gladexml_account_register, "RegisterNewAccountDruid"));

	gnome_druid_set_buttons_sensitive (druid, FALSE, TRUE, TRUE, FALSE);
	glade_xml_signal_autoconnect (gladexml_account_register);
	return;
}
