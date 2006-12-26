#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <glade/glade.h>
#include <libgnomeui/libgnomeui.h>

#include <gtkhtml/gtkhtml.h>

#include "config.h"

#include "gnomegadu_stock.h"
#include "gnomegadu_ui_chat.h"
#include "gnomegadu_conf.h"
#include "gnomegadu_ui.h"
#include "gnomegadu_ui_status.h"
#include "gnomegadu_userlist.h"

static GList *chat_windows_list = NULL;

/* zakłada że wywoływane z listy, musi byc uuid itd. */
gboolean
on_ContactStartChat_activate (GtkWidget * widget, GdkEvent * event, gpointer user_data)
{
	GList *uin_list = NULL;
	GtkTreeStore *contacts_tree_store = NULL;
	GtkTreeView *contacts_tree_view = NULL;
	GtkTreeSelection *selection = NULL;
	GtkTreeIter iter;
	GList *selected_list = NULL;
	GList *selected_list_start = NULL;
	GList *reference_list = NULL;
	GList *reference_list_start = NULL;

	contacts_tree_view = GTK_TREE_VIEW (glade_xml_get_widget (gladexml, "ContactsTreeView"));
	contacts_tree_store = GTK_TREE_STORE (gtk_tree_view_get_model (contacts_tree_view));
	selection = gtk_tree_view_get_selection (contacts_tree_view);

	if (gtk_tree_selection_count_selected_rows (selection) > 0) {
		GladeXML *chat_window_xml = NULL;

		selected_list = gtk_tree_selection_get_selected_rows (selection, NULL);
		selected_list_start = selected_list;
		while (selected_list) {
			GtkTreePath *treepath = selected_list->data;
			GtkTreeRowReference *reference =
			    gtk_tree_row_reference_new (GTK_TREE_MODEL (contacts_tree_store), treepath);
			reference_list = g_list_append (reference_list, reference);
			selected_list = g_list_next (selected_list);
		}

		g_list_foreach (selected_list_start, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (selected_list_start);

		reference_list = g_list_first (reference_list);
		reference_list_start = reference_list;
		while (reference_list) {
			GtkTreeRowReference *reference = (GtkTreeRowReference *) reference_list->data;
			if (gtk_tree_row_reference_valid (reference)) {
				GtkTreePath *refpath = gtk_tree_row_reference_get_path (reference);
				gchar *uuid = NULL;
				gboolean is_group;

				gtk_tree_model_get_iter (GTK_TREE_MODEL (contacts_tree_store), &iter, refpath);
				gtk_tree_model_get (GTK_TREE_MODEL (contacts_tree_store), &iter, UI_CONTACTS_COLUMN_UUID,
						    &uuid, -1);
				gtk_tree_model_get (GTK_TREE_MODEL (contacts_tree_store), &iter, UI_CONTACTS_COLUMN_IS_GROUP,
						    &is_group, -1);

				if (!is_group) {
					g_assert (uuid);

					gchar *uin = gnomegadu_conf_contact_get_uin_for_uuid (uuid);
					uin_list = g_list_append (uin_list, g_strdup (uin));
					g_free (uin);
				}
			}
			gtk_tree_row_reference_free (reference);
			reference_list = g_list_next (reference_list);
		}

		g_list_free (reference_list_start);

		/* find or create window */
		chat_window_xml = gnomegadu_ui_chat_find (uin_list, TRUE);
	} else {
		GtkDialog *msgdialog =
		    gtk_message_dialog_new_with_markup (NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO,
							GTK_BUTTONS_CLOSE,
							"<span weight=\"bold\"size=\"larger\">Nie wybrałeś kontaktu</span>\n\nAby otworzyć okno rozmowy należy wcześniej zaznaczyć wybraną pozycję na liście kontaktów.");
		gtk_dialog_run (GTK_DIALOG (msgdialog));
		gtk_widget_destroy (GTK_WIDGET (msgdialog));
	}

	return TRUE;
}


//kurwa trzeba zmienic koncepcje, bo jak sie operuje na chat_window_xml to nie jest dobrze, tego nie da sie zniszczyc
gboolean
on_CloseChat_activate (GtkWidget * widget, GdkEvent * event, gpointer user_data)
{
	GladeXML *chat_window_xml = glade_get_widget_tree (widget);
	GtkWidget *chat_window = glade_xml_get_widget (chat_window_xml, "ChatWindow");
	GList *uin_list = NULL;

	uin_list = g_object_get_data (G_OBJECT (chat_window_xml), "uin_list");
	g_assert (uin_list);

	chat_windows_list = g_list_remove_all (chat_windows_list, chat_window_xml);

	g_list_foreach (uin_list, (GFunc) g_free, NULL);
	g_list_free (uin_list);

	gtk_widget_destroy (chat_window);
	//gtk_widget_destroy (chat_window_xml);
	//g_object_unref (chat_window_xml);
	return TRUE;
}

gboolean
on_ClearMsgWindow_activate (GtkWidget * widget, GdkEvent * event, gpointer user_data)
{
	GladeXML *chat_window_xml = glade_get_widget_tree (widget);
	GtkWidget *html = GTK_WIDGET (g_object_get_data (G_OBJECT (chat_window_xml), "html"));
	g_object_set_data_full (G_OBJECT (chat_window_xml), "html_str", g_strdup(""), g_free);
	gtk_html_load_empty(GTK_HTML(html));
}

gboolean
on_ChatWindow_delete_event (GtkWidget * widget, GdkEvent * event, gpointer user_data)
{
	on_CloseChat_activate (widget, event, user_data);
	return FALSE;
}

static
    void
gnomegadu_ui_chat_free_uin_list (gpointer list)
{
	GList *list_to_clean = (GList *) list;
	g_list_foreach (list_to_clean, (GFunc) g_free, NULL);
	g_list_free (list_to_clean);
}

void
gnomegadu_ui_chat_window_configure (GladeXML * chat_window_xml, GList * uin_list)
{
	GtkWidget *chat_history_scrolled_window = NULL;
	GtkWidget *status_bar = NULL;
	GtkWidget *chat_contacts_list_scrolled = NULL;
	GtkWidget *chat_window = NULL;

	g_assert (chat_window_xml);

	chat_history_scrolled_window = glade_xml_get_widget (chat_window_xml, "ScrolledWindowHistory");

	if (g_object_get_data (G_OBJECT (chat_window_xml), "html") == NULL) {
		GtkWidget *html = gtk_html_new ();
		gtk_html_load_empty (GTK_HTML (html));
		gtk_container_add (GTK_CONTAINER (chat_history_scrolled_window), html);
		g_object_set_data (G_OBJECT (chat_window_xml), "html", html);
		g_object_set_data_full (G_OBJECT (chat_window_xml), "html_str", g_strdup (""), g_free);
	}

	status_bar = glade_xml_get_widget (chat_window_xml, "StatusBar");
	chat_contacts_list_scrolled = glade_xml_get_widget (chat_window_xml, "ChatContactsListScrolled");
	chat_window = glade_xml_get_widget (chat_window_xml, "ChatWindow");

	glade_xml_signal_autoconnect (chat_window_xml);

	/* fill lists */
	chat_windows_list = g_list_append (chat_windows_list, chat_window_xml);
	g_object_set_data_full (G_OBJECT (chat_window_xml), "uin_list", uin_list, gnomegadu_ui_chat_free_uin_list);

	/* setup window */
	GtkTreeView *chat_window_tree_view;
	GtkCellRenderer *render_text, *render_pixbuf;
	GtkListStore *chat_list_store;
	GtkTreeViewColumn *column_name, *column_icon;
	GtkTreeIter iter;
	gint i;

	chat_window_tree_view = GTK_TREE_VIEW (glade_xml_get_widget (chat_window_xml, "ChatWindowTreeView"));
	g_assert (chat_window_tree_view);

	render_pixbuf = gtk_cell_renderer_pixbuf_new ();
	render_text = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (render_text), "editable", FALSE, NULL);

	column_icon =
	    gtk_tree_view_column_new_with_attributes ("ikonka", render_pixbuf, "pixbuf", UI_CHAT_COLUMN_ICON, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (chat_window_tree_view), column_icon);

	column_name =
	    gtk_tree_view_column_new_with_attributes ("Uczestnicy", render_text, "markup", UI_CHAT_COLUMN_DISPLAYED, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (chat_window_tree_view), column_name);

	chat_list_store = gtk_list_store_new (UI_CHAT_N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);

	for (i = 0; i < g_list_length (uin_list); i++) {
		gchar *uin = g_list_nth_data (uin_list, i);

		gchar *uuid = gnomegadu_conf_contact_get_uuid_for_uin (uin);
		gchar *display = gnomegadu_conf_contact_get_display_for_uuid (uuid);
		const gchar *icon_name;
		GdkPixbuf *pix;
		gint status;

		status = gnomegadu_userlist_get_model_status (uuid);
		icon_name = gnomegadu_ui_status_get_icon_name (status);
		pix = gnomegadu_stock_get_pixbuf (icon_name);

		gtk_list_store_append (chat_list_store, &iter);
		gtk_list_store_set (chat_list_store, &iter, UI_CHAT_COLUMN_ICON, pix, -1);
		gtk_list_store_set (chat_list_store, &iter, UI_CHAT_COLUMN_DISPLAYED, g_strdup (display), -1);
		gtk_list_store_set (chat_list_store, &iter, UI_CHAT_COLUMN_UUID, g_strdup (uuid), -1);

		g_object_unref (G_OBJECT (pix));

		g_free (display);
		g_free (uuid);
	}

	gtk_tree_view_set_model (GTK_TREE_VIEW (chat_window_tree_view), GTK_TREE_MODEL (chat_list_store));


	//just not visible
	if (g_list_length (uin_list) <= 1) {
		gchar *uin = g_list_nth_data (uin_list, 0);
		gchar *uuid = gnomegadu_conf_contact_get_uuid_for_uin (uin);
		gchar *display = gnomegadu_conf_contact_get_display_for_uuid (uuid);
		
		gnome_appbar_push (GNOME_APPBAR (status_bar),
			   g_strdup_printf ("Rozmowa z %s", display));

		gtk_window_set_title (GTK_WINDOW (chat_window), display);
		g_object_set (G_OBJECT (chat_contacts_list_scrolled), "visible", FALSE, NULL);

		g_free (uuid);
	} else {
		gtk_window_set_title (GTK_WINDOW (chat_window), "Konferencja");
		gtk_widget_show_all (chat_contacts_list_scrolled);
		gnome_appbar_push (GNOME_APPBAR (status_bar),
			   g_strdup_printf ("Ilość uczestników konferencji: %d", g_list_length (uin_list)));
	}


	gtk_widget_show_all (chat_history_scrolled_window);
}


void
gnomegadu_ui_chats_userlist_cleanup_model_status ()
{
	GladeXML *chat_window_xml = NULL;
	GtkTreeView *chat_window_tree_view = NULL;
	GtkListStore *chat_list_store = NULL;
	GtkTreeIter iter;
	gboolean valid;
	gint i;

	for (i = 0; i < g_list_length (chat_windows_list); i++) {
		chat_window_xml = g_list_nth_data (chat_windows_list, i);
		g_assert (chat_window_xml);

		chat_window_tree_view = GTK_TREE_VIEW (glade_xml_get_widget (chat_window_xml, "ChatWindowTreeView"));
		chat_list_store = GTK_LIST_STORE (gtk_tree_view_get_model (chat_window_tree_view));

		if (!chat_list_store)
			continue;

		valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (chat_list_store), &iter);
		while (valid) {
			const gchar *icon_name;
			GdkPixbuf *pix;

			icon_name = gnomegadu_ui_status_get_icon_name (GNOMEGADU_STATUS_UNAVAIL);
			pix = gnomegadu_stock_get_pixbuf (icon_name);
			gtk_list_store_set (chat_list_store, &iter, UI_CHAT_COLUMN_ICON, pix, -1);
			g_object_unref (G_OBJECT (pix));

			valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (chat_list_store), &iter);
		}
	}
}

void
gnomegadu_ui_chats_set_status (gchar * uin_str, GnomeGaduProtocolStatus new_status, gchar * status_descr)
{
	GladeXML *chat_window_xml;
	gchar *uuid = NULL, *uuid_search = NULL;
	gchar *path, *path_uuid;
	gboolean valid;
	gint i;

	if (!uin_str)
		return;

	path = gnomegadu_conf_contact_path_find_uin (uin_str);
	if (!path)
		return;

	path_uuid = g_strconcat (path, "/uuid", NULL);
	uuid_search = gconf_client_get_string (gconf, path_uuid, NULL);

	g_free (path);
	g_free (path_uuid);

	for (i = 0; i < g_list_length (chat_windows_list); i++) {
		GtkTreeView *chat_window_tree_view = NULL;
		GtkListStore *chat_list_store;
		GtkTreeIter iter;

		chat_window_xml = g_list_nth_data (chat_windows_list, i);
		chat_window_tree_view = GTK_TREE_VIEW (glade_xml_get_widget (chat_window_xml, "ChatWindowTreeView"));
		chat_list_store = GTK_LIST_STORE (gtk_tree_view_get_model (chat_window_tree_view));

		if (!chat_list_store)
			continue;

		valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (chat_list_store), &iter);
		while (valid) {
			gtk_tree_model_get (GTK_TREE_MODEL (chat_list_store), &iter, UI_CHAT_COLUMN_UUID, &uuid, -1);

			if (uuid && !g_ascii_strcasecmp (uuid, uuid_search)) {
				const gchar *icon_name;
				GdkPixbuf *pix;
				gint status;

				status = gnomegadu_userlist_get_model_status (uuid);
				icon_name = gnomegadu_ui_status_get_icon_name (status);
				pix = gnomegadu_stock_get_pixbuf (icon_name);

				gtk_list_store_set (chat_list_store, &iter, UI_CHAT_COLUMN_ICON, pix, -1);
				g_object_unref (G_OBJECT (pix));
			}

			g_free (uuid);
			valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (chat_list_store), &iter);
		}
	}
	g_free (uuid_search);
}

static gboolean
gnomegadu_chat_view_is_scrolled_down (GladeXML * chat_window_xml)
{
	GtkWidget *chat_history_scrolled_window;
	GtkAdjustment *vadj;

	chat_history_scrolled_window = glade_xml_get_widget (chat_window_xml, "ScrolledWindowHistory");
	vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (chat_history_scrolled_window));

	if (vadj->value + vadj->page_size < vadj->upper - vadj->page_size) {
		return FALSE;
	}

	return TRUE;
}

static void
gnomegadu_chat_view_scroll_down (GladeXML * chat_window_xml)
{
	GtkWidget *sw;
	GtkAdjustment *vadj;

	sw = glade_xml_get_widget (chat_window_xml, "ScrolledWindowHistory");

	vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (sw));
	gtk_adjustment_set_value (vadj, vadj->upper - vadj->page_size);
}


void
gnomegadu_ui_chat_append_text (GladeXML * chat_window_xml, const gchar * txt, GnomeGaduChatType type, gchar * uin_sender_str)
{
	GtkTextView *chat_input_text_view = GTK_TEXT_VIEW (glade_xml_get_widget (chat_window_xml, "ChatInputTextView"));
	GtkTextBuffer *chat_input_buffer = gtk_text_view_get_buffer (chat_input_text_view);
	GtkWidget *html = GTK_WIDGET (g_object_get_data (G_OBJECT (chat_window_xml), "html"));
	gchar *html_str = NULL;
	gchar *old_html_str = NULL;
	gchar *new_html_str = NULL;
	gchar *escaped_txt = g_markup_escape_text (txt, -1);

	if (type == GNOMEGADU_CHAT_SND) {
		html_str =
		    g_strdup_printf
		    ("<table width='100%' bgcolor='#e5e5e5'><tr><td><b>Ja</b></td></tr><tr><td>%s</td></tr></table><br>",
		     escaped_txt);

		gtk_text_buffer_set_text (chat_input_buffer, "", -1);
	} else if (type == GNOMEGADU_CHAT_RCV) {
		gchar *uuid_sender = gnomegadu_conf_contact_get_uuid_for_uin (uin_sender_str);
		gchar *display_sender = gnomegadu_conf_contact_get_display_for_uuid (uuid_sender);
		html_str =
		    g_strdup_printf
		    ("<table width='100%' bgcolor='#b7dbff'><tr><td><b>%s</b></td></tr><tr><td>%s</td></tr></table><br>",
		     display_sender, escaped_txt);

		g_free (uuid_sender);
		g_free (display_sender);
	}

	old_html_str = g_object_get_data (G_OBJECT (chat_window_xml), "html_str"); /* do not free this ! */
	new_html_str = g_strconcat (old_html_str, html_str, NULL);
	g_object_set_data_full (G_OBJECT (chat_window_xml), "html_str", new_html_str, g_free);

	gtk_html_load_from_string (GTK_HTML (html), new_html_str, g_utf8_strlen (new_html_str, -1));

	//if (!gnomegadu_chat_view_is_scrolled_down(chat_window_xml))
	gnomegadu_chat_view_scroll_down (chat_window_xml);

	g_free (html_str);
	g_free (escaped_txt);
}


gboolean
on_ChatInputTextView_key_press_event (GtkWidget * widget, GdkEventKey * event, gpointer user_data)
{
	if (event->type == GDK_KEY_PRESS && (event->keyval == GDK_KP_Enter || event->keyval == GDK_Return)) {
		gchar *text = NULL;
		GtkTextIter iter_start, iter_end;
		GladeXML *chat_window_xml = glade_get_widget_tree (widget);
		GtkTextView *chat_input_text_view =
		    GTK_TEXT_VIEW (glade_xml_get_widget (chat_window_xml, "ChatInputTextView"));
		GtkTextBuffer *chat_input_buffer = GTK_TEXT_BUFFER (gtk_text_view_get_buffer (chat_input_text_view));

		if (gtk_text_buffer_get_char_count (chat_input_buffer) > 0) {
			/* get typed text */
			gtk_text_buffer_get_bounds (chat_input_buffer, &iter_start, &iter_end);

			/* append */
			text = gtk_text_buffer_get_slice (chat_input_buffer, &iter_start, &iter_end, FALSE);
			gnomegadu_ui_chat_append_text (chat_window_xml, text, GNOMEGADU_CHAT_SND, NULL);

			/* send */
			GList *uin_list_window = g_object_get_data (G_OBJECT (chat_window_xml), "uin_list");

			if (g_list_length (uin_list_window) > 1) {
				gint i;
				GList *uin_list = NULL;

				for (i = 0; i < g_list_length (uin_list_window); i++) {
					gchar *uin_str = g_list_nth_data (uin_list_window, i);
					uin_list = g_list_append (uin_list, g_strdup (uin_str));
				}

				gnomegadu_protocol_send_message_confer (uin_list, text);
				g_list_foreach (uin_list, (GFunc) g_free, NULL);
				g_list_free (uin_list);

			} else {
				gchar *uin_str = g_list_nth_data (uin_list_window, 0);
				gnomegadu_protocol_send_message (uin_str, text);
			}

			g_free (text);
		}

		return TRUE;
	}
	return FALSE;
}


//TODO porownywac nie UUID ale UIN
//Przestawic sie na uin_list zamiast uuid_list poniewaz 
//jesli jest uuid_list i sie skasuje ludka z listy to 
//okazuje sie ze tworzy sie nowe okno bo porownywane jest nie uin ale uuid
GladeXML *
gnomegadu_ui_chat_find (GList * uin_list, gboolean autocreate)
{
	GladeXML *chat_window_xml = NULL;
	GList *uin_list_window = NULL;
	gboolean window_found = FALSE;
	gboolean found = FALSE;
	gint i, j, k;

	for (i = 0; i < g_list_length (chat_windows_list); i++) {
		window_found = TRUE;

		chat_window_xml = g_list_nth_data (chat_windows_list, i);
		uin_list_window = g_object_get_data (G_OBJECT (chat_window_xml), "uin_list");

		for (j = 0; j < g_list_length (uin_list_window); j++) {
			found = FALSE;

			for (k = 0; k < g_list_length (uin_list); k++) {
//                              gchar *uin_k = gnomegadu_conf_contact_get_uin_for_uuid (g_list_nth_data (uuid_list, k));
//                              gchar *uin_j = gnomegadu_conf_contact_get_uin_for_uuid (g_list_nth_data (uuid_list_window, j));
				gchar *uin_k = g_list_nth_data (uin_list, k);
				gchar *uin_j = g_list_nth_data (uin_list_window, j);

				g_print ("cmp %s %s\n", uin_k, uin_j);

				if (!g_ascii_strcasecmp (uin_k, uin_j))
					found = TRUE;

				//g_free(uin_k);
				//g_free(uin_j);
			}

			if (!found)
				window_found = FALSE;
		}

		if (window_found) {
			g_print ("window found\n");
			return chat_window_xml;
		}
	}

	if (autocreate) {
		chat_window_xml = glade_xml_new (PACKAGE_DATA_DIR "/gnomegadu.glade", "ChatWindow", NULL);
		/* configure window */
		gnomegadu_ui_chat_window_configure (chat_window_xml, uin_list);
		g_print ("new window\n");
		return chat_window_xml;
	}


	return NULL;
}
