#include <gtk/gtk.h>
#include <string.h>
#include <gconf/gconf-client.h>
#include <libgnome/libgnome.h>
#include <glade/glade.h>

#include "config.h"
#include "gnomegadu_conf.h"
#include "gnomegadu_ui.h"
#include "gnomegadu_ui_chat.h"
#include "gnomegadu_ui_status.h"
#include "gnomegadu_userlist.h"
#include "gnomegadu_protocol.h"


/*
   wywoływane gdy zmieniony jest klucz  z nazwą "display" dla kontaktu 
   lub inna zmiana dla kontaktu na liście
   
   jak jest nowa grupa to nie nadaje sie display bo szukam tylko w podlisciach grup i dlatego nei znajduje!
*/
void
gconf_client_contacts_value_changed_cb (GConfClient * client, guint cnxn_id, GConfEntry * entry, gpointer user_data)
{
	GtkTreeStore *contacts_tree_store;
	GtkTreeView *contacts_tree_view;
	GtkTreeIter *iter_contact = NULL;
	GtkTreeIter iter_group;
	gchar **split_line;
	gchar *uuid = NULL;
	gchar *value_uuid = NULL;
	gboolean valid, valid_group;
	gchar *key = entry->key;
	GConfValue *value = entry->value;
	gchar *base_dir = NULL;
	gchar *tmp = NULL;
	gint pos;

	g_assert (key);

	split_line = g_strsplit (key, "/", 8);
	if (!split_line)
		return;

	value_uuid = split_line[6];

	/* build base dir */
	for (pos = 0; pos < g_strv_length (split_line) - 1; pos++) {
		gchar *base_dir_prev = base_dir;

		if (base_dir)
			base_dir = g_strconcat (base_dir, "/", split_line[pos], NULL);
		else
			base_dir = g_strdup (split_line[pos]);

		g_free (base_dir_prev);
	}

	contacts_tree_view = GTK_TREE_VIEW (glade_xml_get_widget (gladexml, "ContactsTreeView"));
	contacts_tree_store = GTK_TREE_STORE (gtk_tree_view_get_model (contacts_tree_view));

	//add or modify and no "uuid" key
	if (key && value) {
		gchar *group = NULL;

		tmp = g_strconcat (base_dir, "/group", NULL);
		group = gconf_client_get_string (gconf, tmp, NULL);
		g_free (tmp);

		gnomegadu_userlist_find_or_create_group (contacts_tree_store, group);

		valid_group = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (contacts_tree_store), &iter_group);
		while (valid_group) {
			iter_contact = gnomegadu_userlist_uuid_find_iter (contacts_tree_store, &iter_group, value_uuid);

			if (iter_contact)
				break;

			valid_group = gtk_tree_model_iter_next (GTK_TREE_MODEL (contacts_tree_store), &iter_group);
		}

		/* new contact */
		if (!iter_contact) {
			GdkPixbuf *pix = NULL;
			GtkTreeIter *iter_parent = NULL;
			GtkTreeIter iter;
			gchar *display = NULL;

			tmp = g_strconcat (base_dir, "/display", NULL);
			display = gconf_client_get_string (gconf, tmp, NULL);
			g_free (tmp);

			iter_parent = gnomegadu_userlist_group_find_iter (contacts_tree_store, group);
			gtk_tree_store_append (contacts_tree_store, &iter, iter_parent);

			pix = create_pixbuf (USER_NOTAVAIL_ICON);
			gtk_tree_store_set (contacts_tree_store, &iter, UI_CONTACTS_COLUMN_UUID, value_uuid, -1);	//TODO g_strdup ???
			gtk_tree_store_set (contacts_tree_store, &iter, UI_CONTACTS_COLUMN_ICON, pix, -1);	//TODO g_strdup ???
			gtk_tree_store_set (contacts_tree_store, &iter, UI_CONTACTS_COLUMN_STATUS, GNOMEGADU_STATUS_UNAVAIL, -1);	//TODO g_strdup ??
			gtk_tree_store_set (contacts_tree_store, &iter, UI_CONTACTS_COLUMN_IS_GROUP, FALSE, -1);	//TODO g_strdup ???
			gtk_tree_store_set (contacts_tree_store, &iter, UI_CONTACTS_COLUMN_DISPLAYED, display, -1);	//TODO g_strdup ???
			gdk_pixbuf_unref (pix);

			iter_contact = gtk_tree_iter_copy (&iter);

			if (iter_parent)
				gtk_tree_iter_free (iter_parent);
		} else {
			/* just update */
			if (!g_strcasecmp (split_line[7], "display")) {
				gchar *status_descr = NULL;
				gchar *displayed = NULL;

				gtk_tree_model_get (GTK_TREE_MODEL
						    (contacts_tree_store), iter_contact,
						    UI_CONTACTS_COLUMN_STATUS_DESCR, &status_descr, -1);

				if (status_descr) {
					displayed = g_markup_printf_escaped (DISPLAYED_MARKUP,
									     gconf_value_get_string (value), status_descr);

				} else {
					displayed = g_markup_printf_escaped ("%s", gconf_value_get_string (value));

				}

				gtk_tree_store_set (contacts_tree_store, iter_contact, UI_CONTACTS_COLUMN_DISPLAYED, displayed, -1);	//TODO g_strdup ???
				g_free (displayed);
				g_free (status_descr);

			} else if (!g_strcasecmp (split_line[7], "group")) {

				gint column;
				GtkTreeIter *new_group_iter = NULL;
				GtkTreeIter iter;

				/* tutaj troche za duzo sie dzieje */
				//add to new place
				gnomegadu_userlist_find_or_create_group (contacts_tree_store,
									 gconf_value_get_string (value));
				new_group_iter =
				    gnomegadu_userlist_group_find_iter (contacts_tree_store, gconf_value_get_string (value));

				if (new_group_iter) {
					/* create new one copying from old one */
					gtk_tree_store_append (contacts_tree_store, &iter, new_group_iter);

					for (column = UI_CONTACTS_COLUMN_ICON; column < UI_CONTACTS_N_COLUMNS; column++) {
						GValue column_value = { 0, };
						gtk_tree_model_get_value (GTK_TREE_MODEL (contacts_tree_store), iter_contact,
									  column, &column_value);
						gtk_tree_store_set_value (contacts_tree_store, &iter, column, &column_value);
						g_value_unset (&column_value);
					}

					/* expand to show row */
					if (gtk_tree_view_get_model (contacts_tree_view)) {
						GtkTreePath *treepath =
						    gtk_tree_model_get_path (GTK_TREE_MODEL (contacts_tree_store), &iter);
						gtk_tree_view_expand_to_path (contacts_tree_view, treepath);
						gtk_tree_path_free (treepath);
					}

				}
				//remove from old place
				gtk_tree_store_remove (contacts_tree_store, iter_contact);
				
				/* set new iter as current */
				gtk_tree_iter_free(iter_contact);
				gtk_tree_iter_copy(&iter);

				if (new_group_iter)
					gtk_tree_iter_free (new_group_iter);

				gnomegadu_userlist_clean_empty_groups (contacts_tree_store);
			}
		}
		on_ContactsTreeView_cursor_changed (contacts_tree_view, NULL);

		if (iter_contact)
			gtk_tree_iter_free (iter_contact);

		g_free (group);
	}
	//delete
	else if (!value && !split_line[7]) {
		valid_group = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (contacts_tree_store), &iter_group);
		while (valid_group) {
			iter_contact = gnomegadu_userlist_uuid_find_iter (contacts_tree_store, &iter_group, value_uuid);

			if (iter_contact)
				break;

			valid_group = gtk_tree_model_iter_next (GTK_TREE_MODEL (contacts_tree_store), &iter_group);
		}


		if (iter_contact) {
			gtk_tree_store_remove (contacts_tree_store, iter_contact);
			gtk_tree_iter_free (iter_contact);

			gnomegadu_userlist_clean_empty_groups (contacts_tree_store);;
		}
	}

	g_strfreev (split_line);
	g_free (base_dir);
}

gboolean
on_ContactDelete_activate (GtkWidget * widget, GdkEvent * event, gpointer user_data)
{
	GtkTreeStore *contacts_tree_store;
	GtkTreeView *contacts_tree_view;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	gchar *path_uin = NULL;
	GList *selected_list = NULL;
	GList *selected_list_start = NULL;
	GList *reference_list = NULL;
	GList *reference_list_start = NULL;

	contacts_tree_view = GTK_TREE_VIEW (glade_xml_get_widget (gladexml, "ContactsTreeView"));
	contacts_tree_store = GTK_TREE_STORE (gtk_tree_view_get_model (contacts_tree_view));
	selection = gtk_tree_view_get_selection (contacts_tree_view);

	if (gtk_tree_selection_count_selected_rows (selection) > 0) {
		selected_list = gtk_tree_selection_get_selected_rows (selection, NULL);
		selected_list_start = selected_list;
		while (selected_list) {
			GtkTreePath *treepath = selected_list->data;
			GtkTreeRowReference *reference = gtk_tree_row_reference_new (GTK_TREE_MODEL (contacts_tree_store),
										     treepath);
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
				gchar *name = NULL;
				gchar *uuid = NULL;

				GtkTreePath *refpath = gtk_tree_row_reference_get_path (reference);
				gtk_tree_model_get_iter (GTK_TREE_MODEL (contacts_tree_store), &iter, refpath);

				gtk_tree_model_get (GTK_TREE_MODEL
						    (contacts_tree_store), &iter, UI_CONTACTS_COLUMN_UUID, &uuid, -1);
				g_assert (uuid);
				name = gnomegadu_conf_contact_get_display_for_uuid (uuid);

				GtkDialog *msgdialog = gtk_message_dialog_new_with_markup (NULL,
											   GTK_DIALOG_DESTROY_WITH_PARENT,
											   GTK_MESSAGE_QUESTION,
											   GTK_BUTTONS_YES_NO,
											   g_strdup_printf
											   ("<span weight=\"bold\"size=\"larger\">Usuwanie kontaktu z listy</span>\n\nCzy napewno skasować \"%s\" z listy kontaktów?",
											    name));

				if (gtk_dialog_run (GTK_DIALOG (msgdialog))
				    == GTK_RESPONSE_YES) {
					gchar *path = gnomegadu_conf_contact_path_find_uuid (uuid);

					path_uin = g_strconcat (path, "/uin", NULL);
					gnomegadu_protocol_remove_notify (gconf_client_get_string (gconf, path_uin, NULL));
					g_free (path_uin);

					gconf_client_recursive_unset (gconf, path, GCONF_UNSET_INCLUDING_SCHEMA_NAMES, NULL);
					gconf_client_suggest_sync (gconf, NULL);
					g_free (path);
				}

				gtk_widget_destroy (GTK_WIDGET (msgdialog));

				g_free (uuid);
				g_free (name);
			}
			gtk_tree_row_reference_free (reference);
			reference_list = g_list_next (reference_list);
		}

		g_list_free (reference_list_start);
	} else {
		GtkDialog *msgdialog = gtk_message_dialog_new_with_markup (NULL,
									   GTK_DIALOG_DESTROY_WITH_PARENT,
									   GTK_MESSAGE_INFO,
									   GTK_BUTTONS_CLOSE,
									   "<span weight=\"bold\"size=\"larger\">Nie wybrałeś pozycji do skasowania</span>\n\nAby skasować pozycję z listy kontaktów należy zaznaczyć wybraną pozycję na liście, następnie wybrać funkcję kasowania.");
		gtk_dialog_run (GTK_DIALOG (msgdialog));
		gtk_widget_destroy (GTK_WIDGET (msgdialog));
	}

	return TRUE;
}

//GNOMEGADU_STATUS_UNKNOWN gdzy nie mam go na liscie
GnomeGaduProtocolStatus
gnomegadu_userlist_get_model_status (gchar * uuid_search)
{
	GtkTreeStore *contacts_tree_store;
	GtkTreeView *contacts_tree_view;
	GtkTreeIter iter, iter_group;
	gboolean valid, valid_group;
	gchar *uuid = NULL;
	gint status;

	if (!uuid_search)
		return GNOMEGADU_STATUS_UNKNOWN;

	contacts_tree_view = GTK_TREE_VIEW (glade_xml_get_widget (gladexml, "ContactsTreeView"));
	contacts_tree_store = GTK_TREE_STORE (gtk_tree_view_get_model (contacts_tree_view));

	valid_group = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (contacts_tree_store), &iter_group);
	while (valid_group) {
		valid = gtk_tree_model_iter_children (GTK_TREE_MODEL (contacts_tree_store), &iter, &iter_group);
		while (valid) {
			gtk_tree_model_get (GTK_TREE_MODEL (contacts_tree_store), &iter, UI_CONTACTS_COLUMN_UUID, &uuid, -1);
			if (uuid && !g_strcasecmp (uuid, uuid_search)) {
				gtk_tree_model_get (GTK_TREE_MODEL
						    (contacts_tree_store), &iter, UI_CONTACTS_COLUMN_STATUS, &status, -1);
				g_free (uuid);
				return status;
			}

			g_free (uuid);
			valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (contacts_tree_store), &iter);
		}
		valid_group = gtk_tree_model_iter_next (GTK_TREE_MODEL (contacts_tree_store), &iter_group);
	}

	return GNOMEGADU_STATUS_UNKNOWN;
}

//FIXME A co jeśli nie mam go w książce ?
void
gnomegadu_userlist_set_model_status (gchar * uin_str, GnomeGaduProtocolStatus new_status, gchar * status_descr)
{
	GtkTreeStore *contacts_tree_store;
	GtkTreeView *contacts_tree_view;
	GtkTreeRowReference *reference = NULL;
	GtkTreeIter iter, iter_group;
	gboolean valid, valid_group;
	gboolean found = FALSE;
	gchar *uuid = NULL, *uuid_search = NULL;
	gchar *path, *path_uuid;
	GdkPixbuf *pix = NULL;

	if (!uin_str)
		return;

	contacts_tree_view = GTK_TREE_VIEW (glade_xml_get_widget (gladexml, "ContactsTreeView"));
	contacts_tree_store = GTK_TREE_STORE (gtk_tree_view_get_model (contacts_tree_view));

	path = gnomegadu_conf_contact_path_find_uin (uin_str);
	if (!path) {
		g_printerr ("gnomegadu_userlist_set_model_status: no path for uin mean no uin on the list, aborting\n");
		return;
	}
	path_uuid = g_strconcat (path, "/uuid", NULL);
	uuid_search = gconf_client_get_string (gconf, path_uuid, NULL);

	/* iter every group */
	valid_group = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (contacts_tree_store), &iter_group);
	while (valid_group && !found) {
		/* iter every contact in group */
		valid = gtk_tree_model_iter_children (GTK_TREE_MODEL (contacts_tree_store), &iter, &iter_group);
		while (valid) {
			gtk_tree_model_get (GTK_TREE_MODEL (contacts_tree_store), &iter, UI_CONTACTS_COLUMN_UUID, &uuid, -1);
			if (uuid && !g_strcasecmp (uuid, uuid_search)) {
				GtkTreePath *treepath = gtk_tree_model_get_path (GTK_TREE_MODEL (contacts_tree_store),
										 &iter);
				reference = gtk_tree_row_reference_new (GTK_TREE_MODEL (contacts_tree_store), treepath);
				gtk_tree_path_free (treepath);
				found = TRUE;
				break;
			}

			g_free (uuid);
			valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (contacts_tree_store), &iter);
		}
		valid_group = gtk_tree_model_iter_next (GTK_TREE_MODEL (contacts_tree_store), &iter_group);
	}

	if (gtk_tree_row_reference_valid (reference) && found) {
		gchar *name = NULL;
		gchar *uuid = NULL;
		GtkTreePath *refpath = gtk_tree_row_reference_get_path (reference);

		gtk_tree_model_get_iter (GTK_TREE_MODEL (contacts_tree_store), &iter, refpath);
		gtk_tree_path_free (refpath);

		gtk_tree_model_get (GTK_TREE_MODEL (contacts_tree_store), &iter, UI_CONTACTS_COLUMN_UUID, &uuid, -1);
		name = gnomegadu_conf_contact_get_display_for_uuid (uuid);

		pix = create_pixbuf (gnomegadu_ui_status_get_icon_name (new_status));
		gtk_tree_store_set (contacts_tree_store, &iter, UI_CONTACTS_COLUMN_ICON, pix, -1);	//TODO g_strdup ???                     
		gtk_tree_store_set (contacts_tree_store, &iter, UI_CONTACTS_COLUMN_STATUS, new_status, -1);	//TODO g_strdup ???                     
		gdk_pixbuf_unref (pix);

		if (status_descr) {
			gchar *displayed = g_markup_printf_escaped (DISPLAYED_MARKUP,
								    name, status_descr);
			gtk_tree_store_set (contacts_tree_store, &iter, UI_CONTACTS_COLUMN_DISPLAYED, displayed, -1);	//TODO g_strdup ???                             
			gtk_tree_store_set (contacts_tree_store, &iter, UI_CONTACTS_COLUMN_STATUS_DESCR, status_descr, -1);	//TODO g_strdup ???
			g_free (displayed);
		} else {
			gchar *displayed = g_markup_printf_escaped ("%s", name);
			gtk_tree_store_set (contacts_tree_store, &iter, UI_CONTACTS_COLUMN_DISPLAYED, displayed, -1);	//TODO g_strdup ???
			g_free (displayed);
		}

		gnomegadu_ui_chats_set_status (uin_str, new_status, status_descr);

		g_free (name);
		g_free (uuid);
	}

	g_free (path);
	g_free (path_uuid);
	g_free (uuid_search);
}


void
gnomegadu_userlist_cleanup_model_status ()
{
	GtkTreeStore *contacts_tree_store;
	GtkTreeView *contacts_tree_view;
	GtkTreeIter iter, iter_group;
	gboolean valid = FALSE;
	gboolean valid_group = FALSE;
	GSList *reference_list = NULL;
	GSList *reference_list_tmp = NULL;
	GdkPixbuf *pix = NULL;

	contacts_tree_view = GTK_TREE_VIEW (glade_xml_get_widget (gladexml, "ContactsTreeView"));
	g_assert (contacts_tree_view);

	contacts_tree_store = GTK_TREE_STORE (gtk_tree_view_get_model (contacts_tree_view));

	if (!contacts_tree_store)
		return;

	/* iter every group */
	valid_group = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (contacts_tree_store), &iter_group);
	while (valid_group) {
		valid = gtk_tree_model_iter_children (GTK_TREE_MODEL (contacts_tree_store), &iter, &iter_group);
		while (valid) {
			GtkTreePath *treepath = gtk_tree_model_get_path (GTK_TREE_MODEL (contacts_tree_store),
									 &iter);
			GtkTreeRowReference *reference = gtk_tree_row_reference_new (GTK_TREE_MODEL (contacts_tree_store),
										     treepath);
			reference_list = g_slist_append (reference_list, reference);
			gtk_tree_path_free (treepath);

			valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (contacts_tree_store), &iter);
		}
		valid_group = gtk_tree_model_iter_next (GTK_TREE_MODEL (contacts_tree_store), &iter_group);
	}

	reference_list_tmp = reference_list;
	while (reference_list_tmp) {
		GtkTreeRowReference *reference = (GtkTreeRowReference *) reference_list_tmp->data;

		if (gtk_tree_row_reference_valid (reference)) {
			gchar *name = NULL;
			gchar *uuid = NULL;
			GtkTreePath *refpath = gtk_tree_row_reference_get_path (reference);
			gtk_tree_model_get_iter (GTK_TREE_MODEL (contacts_tree_store), &iter, refpath);

			gtk_tree_model_get (GTK_TREE_MODEL (contacts_tree_store), &iter, UI_CONTACTS_COLUMN_UUID, &uuid, -1);	//TODO g_strdup ???                     
			name = gnomegadu_conf_contact_get_display_for_uuid (uuid);

			pix = create_pixbuf (USER_NOTAVAIL_ICON);
			gtk_tree_store_set (contacts_tree_store, &iter, UI_CONTACTS_COLUMN_ICON, pix, -1);	//TODO g_strdup ???                     
			gtk_tree_store_set (contacts_tree_store, &iter, UI_CONTACTS_COLUMN_STATUS, GNOMEGADU_STATUS_UNAVAIL, -1);	//TODO g_strdup ???                     
			gtk_tree_store_set (contacts_tree_store, &iter, UI_CONTACTS_COLUMN_STATUS_DESCR, NULL, -1);	//TODO g_strdup ???                     
			//FIXME ewentualnie dodac eskejpowanie
			gtk_tree_store_set (contacts_tree_store, &iter, UI_CONTACTS_COLUMN_DISPLAYED, name, -1);	//TODO g_strdup ???                     
			gdk_pixbuf_unref (pix);

			g_free (uuid);
			g_free (name);
			gtk_tree_path_free (refpath);
		}

		gtk_tree_row_reference_free (reference);
		reference_list_tmp = reference_list_tmp->next;
	}

	g_slist_free (reference_list);

	gnomegadu_ui_chats_userlist_cleanup_model_status ();
}

static gint
gnomegadu_compare_groups (gconstpointer a, gconstpointer b)
{
	const gchar *aa = a;
	const gchar *bb = b;

	return g_utf8_collate (aa, bb);
}

GList *
gnomegadu_userlist_get_groups ()
{
	gchar *path;
	gchar *root;
	gchar *check_group;
	GSList *list, *list_start;
	GList *groups = NULL;
	GList *groups_start = NULL;
	GList *ret = NULL;

	list = gnomegadu_conf_get_contacts ();
	list_start = list;

	if (!list)
		return NULL;

	// dodac wszystkie, posortowac i usunac duplikaty
	while (list) {
		root = (gchar *) list->data;
		path = g_strconcat (root, "/group", NULL);
		check_group = gconf_client_get_string (gconf, path, NULL);

		if (check_group && (g_utf8_strlen (check_group, -1) > 0)) {
			groups = g_list_insert_sorted (groups, check_group, gnomegadu_compare_groups);
		} else {
			groups = g_list_insert_sorted (groups, g_strdup (EMPTY_GROUP), gnomegadu_compare_groups);
		}

		list = g_slist_next (list);
	}

	g_slist_foreach (list_start, gnomegadu_conf_free_list_of_string, NULL);
	g_slist_free (list_start);

	groups = g_list_first (groups);
	groups_start = groups;
	while (groups) {
		gchar *group = groups->data;
		gchar *prev_group = NULL;

		if (g_list_previous (groups))
			prev_group = g_list_previous (groups)->data;

		if (!prev_group || g_utf8_collate (group, prev_group) != 0)
			ret = g_list_append (ret, g_strdup (group));

		groups = g_list_next (groups);
	}

	g_list_foreach (groups_start, gnomegadu_conf_free_list_of_string, NULL);
	g_list_free (groups_start);

	if (!ret)
		ret = g_list_append (ret, g_strdup (EMPTY_GROUP));

	return ret;
}

/* must be freed with gtk_tree_iter_free(iter) */
GtkTreeIter *
gnomegadu_userlist_group_find_iter (GtkTreeStore * treestore, gchar * group_name)
{
	GtkTreeIter iter;
	gboolean valid;
	gchar *real_group_name = NULL;

	real_group_name = gnomegadu_userlist_get_group_name (group_name);

	valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (treestore), &iter);
	while (valid) {
		gchar *displayed_group = NULL;
		gtk_tree_model_get (GTK_TREE_MODEL (treestore), &iter, UI_CONTACTS_COLUMN_DISPLAYED, &displayed_group, -1);	//TODO g_strdup ???

		if (!g_utf8_collate (displayed_group, real_group_name)) {
			g_free (displayed_group);
			g_free (real_group_name);
			return gtk_tree_iter_copy (&iter);
		}

		g_free (displayed_group);
		valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (treestore), &iter);
	}

	g_free (real_group_name);
	return NULL;
}


/* must be freed with gtk_tree_iter_free(iter) */
GtkTreeIter *
gnomegadu_userlist_uuid_find_iter (GtkTreeStore * treestore, GtkTreeIter * parent_iter, gchar * search_uuid)
{
	GtkTreeIter iter;
	gboolean valid;

	valid = gtk_tree_model_iter_children (GTK_TREE_MODEL (treestore), &iter, parent_iter);
	while (valid) {
		gchar *displayed_uuid = NULL;
		gtk_tree_model_get (GTK_TREE_MODEL (treestore), &iter, UI_CONTACTS_COLUMN_UUID, &displayed_uuid, -1);	//TODO g_strdup ???

		if (!g_utf8_collate (displayed_uuid, search_uuid)) {
			g_free (displayed_uuid);
			return gtk_tree_iter_copy (&iter);
		}

		g_free (displayed_uuid);
		valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (treestore), &iter);
	}

	return NULL;
}

void
gnomegadu_userlist_find_or_create_group (GtkTreeStore * treestore, gchar * group)
{
	gchar *real_group_name = NULL;
	GtkTreeIter *iter = NULL;
	GtkTreeIter iter_new;
	GtkTreePath *treepath = NULL;
	GtkTreeView *treeview = NULL;

	real_group_name = gnomegadu_userlist_get_group_name (group);

	iter = gnomegadu_userlist_group_find_iter (treestore, real_group_name);
	if (iter) {
		gtk_tree_iter_free (iter);
		g_free (real_group_name);
		return;
	}

	gtk_tree_store_append (treestore, &iter_new, NULL);
	gtk_tree_store_set (treestore, &iter_new, UI_CONTACTS_COLUMN_DISPLAYED, g_strdup (real_group_name), -1);	//TODO g_strdup ???
	gtk_tree_store_set (treestore, &iter_new, UI_CONTACTS_COLUMN_IS_GROUP, TRUE, -1);	//TODO g_strdup ???

	treeview = GTK_TREE_VIEW (glade_xml_get_widget (gladexml, "ContactsTreeView"));
	if (gtk_tree_view_get_model (treeview)) {
		treepath = gtk_tree_model_get_path (GTK_TREE_MODEL (treestore), &iter_new);
		gchar *path = gtk_tree_path_to_string (treepath);
		g_print ("%s\n", path);
		gtk_tree_view_expand_to_path (treeview, treepath);
		//gtk_tree_view_expand_row(treeview,treepath,FALSE);
		gtk_tree_path_free (treepath);
	}

	g_free (real_group_name);
}


void
gnomegadu_userlist_clean_empty_groups (GtkTreeStore * treestore)
{
	GtkTreeIter iter, iter_group;
	GList *reference_list = NULL;
	GList *reference_list_start = NULL;
	gboolean valid_group = FALSE;
	gboolean valid = FALSE;

	valid_group = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (treestore), &iter_group);
	while (valid_group) {
		valid = gtk_tree_model_iter_children (GTK_TREE_MODEL (treestore), &iter, &iter_group);
		if (!valid)	//nie ma nic w grupie
		{
			GtkTreePath *treepath = gtk_tree_model_get_path (GTK_TREE_MODEL (treestore), &iter_group);
			GtkTreeRowReference *reference = gtk_tree_row_reference_new (GTK_TREE_MODEL (treestore), treepath);

			gtk_tree_path_free (treepath);
			reference_list = g_list_append (reference_list, reference);
		}
		valid_group = gtk_tree_model_iter_next (GTK_TREE_MODEL (treestore), &iter_group);
	}

	/* delete empty groups */
	reference_list = g_list_first (reference_list);
	reference_list_start = reference_list;
	while (reference_list) {
		GtkTreeRowReference *reference = (GtkTreeRowReference *) reference_list->data;
		GtkTreePath *refpath = gtk_tree_row_reference_get_path (reference);

		gtk_tree_model_get_iter (GTK_TREE_MODEL (treestore), &iter, refpath);
		gtk_tree_store_remove (treestore, &iter);

		gtk_tree_row_reference_free (reference);
		reference_list = g_list_next (reference_list);
	}

	g_list_free (reference_list_start);
}

gchar *
gnomegadu_userlist_get_group_name (gchar * group)
{
	gchar *name = g_strdup (group);

	if (!group || g_utf8_strlen (group, -1) <= 0) {
		name = g_strdup (EMPTY_GROUP);
	}

	return name;
}
