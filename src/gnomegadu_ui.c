#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <glade/glade.h>
#include <libgnomeui/libgnomeui.h>

#include "config.h"
#include "gnomegadu_ui.h"
#include "gnomegadu_conf.h"
#include "gnomegadu_ui_account_chooser.h"
#include "gnomegadu_ui_user_edit.h"
#include "gnomegadu_ui_user_add.h"
#include "gnomegadu_protocol.h"
#include "gnomegadu_userlist.h"
#include "gnomegadu_tray.h"
#include "gossip-cell-renderer-expander.h"


gboolean
on_ContactsTreeView_button_press_event (GtkTreeView * treeview, GdkEventButton * event, gpointer user_data)
{
	GtkTreePath *path = NULL;
	GtkTreeViewColumn *column = NULL;
	GtkMenu *menu = NULL;
	gint cell_x, cell_y;

	if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
		menu = GTK_MENU (glade_xml_get_widget (gladexml_menu, "ContactsPopupMenu"));
		g_assert (menu);

		if (gtk_tree_view_get_path_at_pos (treeview, event->x, event->y, &path, &column, &cell_x, &cell_y)) {
			GtkTreeModel *model;
			GtkTreeIter iter;
			gboolean is_group;

			model = gtk_tree_view_get_model (treeview);
			if (gtk_tree_model_get_iter (model, &iter, path)) {
				gtk_tree_model_get (model, &iter, UI_CONTACTS_COLUMN_IS_GROUP, &is_group, -1);

				if (!is_group)
					gtk_menu_popup (menu, NULL, NULL, NULL, NULL, event->button, event->time);
			}
		}
		if (path)
			gtk_tree_path_free (path);
	}

	return FALSE;
}


//używane przy aktualizacji pozycji na liscie w userlist.c
void
on_ContactsTreeView_cursor_changed (GtkTreeView * treeview, gpointer user_data)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *uuid = NULL;
	gchar *path = NULL;
	gchar *tmp = NULL;
	gchar *uin = NULL;
	gchar *first_name = NULL;
	gchar *last_name = NULL;
	GtkLabel *label_uin = NULL;
	GtkLabel *label_details = NULL;
	GList *selected_list = NULL;
	GList *selected_list_start = NULL;

	selection = gtk_tree_view_get_selection (treeview);
	model = gtk_tree_view_get_model (treeview);

	if (!selection) {
		g_print ("no selection, no error, juz warning\n");
		return;
	}

	selected_list = gtk_tree_selection_get_selected_rows (selection, &model);
	selected_list_start = selected_list;

	while (selected_list) {
		GtkTreePath *treepath = selected_list->data;
		gboolean is_group;

		gtk_tree_model_get_iter (model, &iter, treepath);

		gtk_tree_model_get (model, &iter, UI_CONTACTS_COLUMN_UUID, &uuid, -1);
		gtk_tree_model_get (model, &iter, UI_CONTACTS_COLUMN_IS_GROUP, &is_group, -1);

		if (is_group)	//PARENT
			goto next_selected;

		g_assert (uuid);

		path = gnomegadu_conf_contact_path_find_uuid (uuid);

		tmp = g_strconcat (path, "/uin", NULL);
		uin = gconf_client_get_string (gconf, tmp, NULL);
		g_free (tmp);

		tmp = g_strconcat (path, "/first_name", NULL);
		first_name = gconf_client_get_string (gconf, tmp, NULL);
		g_free (tmp);

		tmp = g_strconcat (path, "/last_name", NULL);
		last_name = gconf_client_get_string (gconf, tmp, NULL);
		g_free (tmp);

		label_uin = GTK_LABEL (glade_xml_get_widget (gladexml, "ContactDetailsTitleLabel"));
		gtk_label_set_text (label_uin, uin);

		label_details = GTK_LABEL (glade_xml_get_widget (gladexml, "ContactDetailsLabel"));

		tmp = g_strconcat ("<b>Imię:</b> ", first_name, "\n<b>Nazwisko:</b>", last_name, NULL);
		gtk_label_set_markup (label_details, (const gchar *) tmp);
		g_free (tmp);

		g_free (first_name);
		g_free (last_name);
		g_free (uin);
	      next_selected:
		selected_list = g_list_next (selected_list);
	}

	g_list_foreach (selected_list_start, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (selected_list_start);

	g_free (path);
}

gboolean
on_ContactsTreeView_key_press_event (GtkWidget * widget, GdkEventKey * event, gpointer user_data)
{
	if (event->type == GDK_KEY_PRESS && (event->keyval == GDK_Delete || event->keyval == GDK_KP_Delete)) {
		on_ContactDelete_activate (widget, NULL, user_data);
	} else if (event->type == GDK_KEY_PRESS && (event->keyval == GDK_i || event->keyval == GDK_I)) {
		on_ContactEdit_activate (widget, NULL, user_data);
	} else if (event->type == GDK_KEY_PRESS && (event->keyval == GDK_n || event->keyval == GDK_N)) {
		on_ContactAdd_activate (widget, NULL, user_data);
	}

	return FALSE;
}


/* based on Gossip function */
static void
gnomegadu_contact_list_icon_cell_data_func (GtkTreeViewColumn * column,
					    GtkCellRenderer * cell,
					    GtkTreeModel * model, GtkTreeIter * iter, GtkTreeView * list)
{
	gboolean is_group;

	gtk_tree_model_get (model, iter, UI_CONTACTS_COLUMN_IS_GROUP, &is_group, -1);

	if (is_group) {
		GdkColor color;
		GtkStyle *style;

		style = gtk_widget_get_style (GTK_WIDGET (list));
		color = style->text_aa[GTK_STATE_INSENSITIVE];
		color.red = (color.red + (style->white).red) / 2;
		color.green = (color.green + (style->white).green) / 2;
		color.blue = (color.blue + (style->white).blue) / 2;

		g_object_set (cell, "cell-background-gdk", &color, NULL);
	} else {
		g_object_set (cell, "cell-background-gdk", NULL, NULL);
	}

	g_object_set (cell, "visible", !is_group, NULL);

}


/* based on Gossip function */
static void
gnomegadu_contact_list_name_cell_data_func (GtkTreeViewColumn * column,
					    GtkCellRenderer * cell,
					    GtkTreeModel * model, GtkTreeIter * iter, GtkTreeView * list)
{
	gboolean is_group;

	gtk_tree_model_get (model, iter, UI_CONTACTS_COLUMN_IS_GROUP, &is_group, -1);

	if (is_group) {
		GdkColor color;
		GtkStyle *style;

		style = gtk_widget_get_style (GTK_WIDGET (list));
		color = style->text_aa[GTK_STATE_INSENSITIVE];
		color.red = (color.red + (style->white).red) / 2;
		color.green = (color.green + (style->white).green) / 2;
		color.blue = (color.blue + (style->white).blue) / 2;

		g_object_set (cell, "cell-background-gdk", &color, NULL);
	} else {
		g_object_set (cell, "cell-background-gdk", NULL, NULL);
	}
}


/* based on Gossip function */
static void
gnomegadu_contact_list_expander_cell_data_func (GtkTreeViewColumn * column,
						GtkCellRenderer * cell,
						GtkTreeModel * model, GtkTreeIter * iter, GtkTreeView * list)
{
	if (gtk_tree_model_iter_has_child (model, iter)) {
		GdkColor color;
		GtkStyle *style;
		GtkTreePath *path;
		gboolean row_expanded;

		path = gtk_tree_model_get_path (model, iter);
		row_expanded = gtk_tree_view_row_expanded (GTK_TREE_VIEW (column->tree_view), path);
		gtk_tree_path_free (path);

		g_object_set (cell,
			      "visible", TRUE,
			      "expander-style", row_expanded ? GTK_EXPANDER_EXPANDED : GTK_EXPANDER_COLLAPSED, NULL);

		style = gtk_widget_get_style (GTK_WIDGET (list));
		color = style->text_aa[GTK_STATE_INSENSITIVE];
		color.red = (color.red + (style->white).red) / 2;
		color.green = (color.green + (style->white).green) / 2;
		color.blue = (color.blue + (style->white).blue) / 2;

		g_object_set (cell, "cell-background-gdk", &color, NULL);

	} else {
		g_object_set (cell, "visible", FALSE, NULL);
	}
}

static
 gboolean
gnomegadu_contacts_selection_cb (GtkTreeSelection * selection,
				 GtkTreeModel * model, GtkTreePath * path, gboolean path_currently_selected, gpointer data)
{
	GtkTreeIter iter;
	gboolean is_group;

	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, UI_CONTACTS_COLUMN_IS_GROUP, &is_group, -1);

	return !is_group;
}


void
gnomegadu_ui_init_contacts_treeview ()
{
	GtkTreeView *contacts_tree_view;
	GtkCellRenderer *render_text, *render_pixbuf, *render_expander;
	GtkTreeViewColumn *col;

	contacts_tree_view = GTK_TREE_VIEW (glade_xml_get_widget (gladexml, "ContactsTreeView"));
	g_object_set (contacts_tree_view, "show-expanders", FALSE, NULL);

	col = gtk_tree_view_column_new ();

	/* icon */
	render_pixbuf = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (col, render_pixbuf, FALSE);
	gtk_tree_view_column_set_cell_data_func (col, render_pixbuf,
						 (GtkTreeCellDataFunc) gnomegadu_contact_list_icon_cell_data_func,
						 contacts_tree_view, NULL);
	gtk_tree_view_column_add_attribute (col, render_pixbuf, "pixbuf", UI_CONTACTS_COLUMN_ICON);

	g_object_set (render_pixbuf, "xpad", 5, "ypad", 1, "visible", FALSE, NULL);

	/* name */
	render_text = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (render_text), "editable", FALSE, NULL);
	g_object_set (G_OBJECT (render_text), "wrap-width", 120, NULL);
	gtk_tree_view_column_pack_start (col, render_text, TRUE);
	gtk_tree_view_column_set_cell_data_func (col, render_text,
						 (GtkTreeCellDataFunc) gnomegadu_contact_list_name_cell_data_func,
						 contacts_tree_view, NULL);
	gtk_tree_view_column_add_attribute (col, render_text, "markup", UI_CONTACTS_COLUMN_DISPLAYED);

	/* expander */
	render_expander = gossip_cell_renderer_expander_new ();
	gtk_tree_view_column_pack_start (col, render_expander, FALSE);
	gtk_tree_view_column_set_cell_data_func (col, render_expander,
						 (GtkTreeCellDataFunc) gnomegadu_contact_list_expander_cell_data_func,
						 contacts_tree_view, NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (contacts_tree_view), col);


	/* set selection */
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (contacts_tree_view));
	gtk_tree_selection_set_select_function (selection, gnomegadu_contacts_selection_cb, NULL, NULL);
}


void
gnomegadu_ui_init ()
{
	GSList *accounts;
	gchar *default_account;
	gladexml = glade_xml_new (PACKAGE_DATA_DIR "/gnomegadu.glade", "MainWindow", NULL);
	glade_xml_signal_autoconnect (gladexml);

	gladexml_menu = glade_xml_new (PACKAGE_DATA_DIR "/gnomegadu.glade", "ContactsPopupMenu", NULL);
	glade_xml_signal_autoconnect (gladexml_menu);

	gnomegadu_ui_init_contacts_treeview ();
	gnomegadu_ui_init_statusbar ();

	default_account = gnomegadu_conf_get_default_account_name ();
	accounts = gnomegadu_conf_get_accounts ();

	gnomegadu_tray_init ();

	if (!accounts) {
		// there is no accounts yet
		on_AccountPreferences_activate (NULL, NULL, NULL);
	} else if (!default_account) {
		//if there is no default account
		gnomegadu_ui_account_chooser_activate ();
	} else {
		gnomegadu_conf_set_profile (default_account);
	}

	g_free (default_account);
	g_slist_free (accounts);
}

/*
 * auto signals from gnomegadu.glade 
 */
gboolean
on_MainWindow_delete_event (GtkWidget * widget, GdkEvent * event, gpointer user_data)
{
	on_MainMenuQuit_activate (widget, user_data);
	return TRUE;
}

void
on_MainMenuQuit_activate (GtkWidget * widget, gpointer user_data)
{
	gtk_main_quit ();
}

GdkPixbuf *
create_pixbuf (const gchar * filename)
{
	gchar *found_filename = NULL;
	GdkPixbuf *pixbuf = NULL;

	if (!filename || !filename[0])
		return NULL;

	/* We first try any pixmaps directories set by the application. */
	found_filename = g_strconcat (PACKAGE_DATA_DIR, "/", filename, NULL);
	if (!g_file_test (found_filename, G_FILE_TEST_IS_REGULAR)) {
		g_print ("Couldn't find pixmap file: /%s\n", found_filename);
		g_free (found_filename);
		return NULL;
	}

	pixbuf = gdk_pixbuf_new_from_file (found_filename, NULL);

	return pixbuf;
}

gint
gnomegadu_ui_tree_sort (GtkTreeModel * model, GtkTreeIter * a, GtkTreeIter * b, gpointer user_data)
{
	gchar *str_a = NULL, *str_b = NULL;
	gint status_a = 0, status_b = 0;
	gint ret = 0;

	gtk_tree_model_get (GTK_TREE_MODEL (model), a, UI_CONTACTS_COLUMN_DISPLAYED, &str_a, UI_CONTACTS_COLUMN_STATUS,
			    &status_a, -1);
	gtk_tree_model_get (GTK_TREE_MODEL (model), b, UI_CONTACTS_COLUMN_DISPLAYED, &str_b, UI_CONTACTS_COLUMN_STATUS,
			    &status_b, -1);

	if ((status_a == GNOMEGADU_STATUS_UNAVAIL) && (status_b != GNOMEGADU_STATUS_UNAVAIL))
		ret = 1;

	if ((status_a != GNOMEGADU_STATUS_UNAVAIL) && (status_b == GNOMEGADU_STATUS_UNAVAIL))
		ret = -1;

	if (str_a && str_b && ret == 0) {
		ret = g_utf8_collate (str_a, str_b);
	}

	g_free (str_a);
	g_free (str_b);
	return ret;
}

void
gnomegedu_ui_init_userlist ()
{
	GtkTreeStore *contacts_tree_store;
	GtkTreeView *contacts_tree_view;
	GtkTreeIter iter;
	gboolean update = FALSE;
	GSList *list, *list_start;
	gchar *path;
	gchar *root;
	gchar *display, *uuid, *group;
	GdkPixbuf *pix;
	GtkTreeSelection *selection;


	contacts_tree_view = GTK_TREE_VIEW (glade_xml_get_widget (gladexml, "ContactsTreeView"));
	contacts_tree_store = GTK_TREE_STORE (gtk_tree_view_get_model (contacts_tree_view));
	if (!contacts_tree_store) {
		update = FALSE;
		contacts_tree_store =
		    gtk_tree_store_new (UI_CONTACTS_N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT,
					G_TYPE_STRING, G_TYPE_BOOLEAN);

		selection = gtk_tree_view_get_selection (contacts_tree_view);
		gtk_tree_selection_set_mode (GTK_TREE_SELECTION (selection), GTK_SELECTION_MULTIPLE);

		gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (contacts_tree_store), gnomegadu_ui_tree_sort,
							 NULL, NULL);
		gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (contacts_tree_store),
						      GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, GTK_SORT_ASCENDING);
	} else {
		update = TRUE;
		gtk_tree_store_clear (GTK_TREE_STORE (contacts_tree_store));
	}
	gtk_tree_view_set_model (contacts_tree_view, NULL);

	/* groups */
	GList *groups = gnomegadu_userlist_get_groups ();
	GList *groups_start = groups;
	while (groups) {
		gchar *group = groups->data;
		gnomegadu_userlist_find_or_create_group (contacts_tree_store, group);
		groups = g_list_next (groups);
	}

	g_list_foreach (groups_start, gnomegadu_conf_free_list_of_string, NULL);
	g_list_free (groups_start);

	/* contacts */
	list = gnomegadu_conf_get_contacts ();
	list_start = list;

	while (list) {
		root = (gchar *) list->data;

		path = g_strconcat (root, "/uuid", NULL);
		uuid = gconf_client_get_string (gconf, path, NULL);
		g_free (path);

		path = g_strconcat (root, "/display", NULL);
		display = gconf_client_get_string (gconf, path, NULL);
		g_free (path);

		path = g_strconcat (root, "/group", NULL);
		group = gconf_client_get_string (gconf, path, NULL);
		g_free (path);

		if (!group || g_utf8_strlen (group, -1) <= 0) {
			g_free (group);
			group = g_strdup (EMPTY_GROUP);
		}

		if (display && uuid) {
			GtkTreeIter *iter_parent = gnomegadu_userlist_group_find_iter (contacts_tree_store, group);

			gtk_tree_store_append (contacts_tree_store, &iter, iter_parent);

			pix = create_pixbuf (USER_NOTAVAIL_ICON);
			gtk_tree_store_set (contacts_tree_store, &iter, UI_CONTACTS_COLUMN_ICON, pix, -1);	//TODO g_strdup ???                     
			gtk_tree_store_set (contacts_tree_store, &iter, UI_CONTACTS_COLUMN_DISPLAYED, display, -1);	//TODO g_strdup ???                     
			gtk_tree_store_set (contacts_tree_store, &iter, UI_CONTACTS_COLUMN_UUID, uuid, -1);	//TODO g_strdup ???                     
			gtk_tree_store_set (contacts_tree_store, &iter, UI_CONTACTS_COLUMN_STATUS, GNOMEGADU_STATUS_UNAVAIL, -1);	//TODO g_strdup ???                     
			gtk_tree_store_set (contacts_tree_store, &iter, UI_CONTACTS_COLUMN_STATUS_DESCR, NULL, -1);	//TODO g_strdup ???                     
			gtk_tree_store_set (contacts_tree_store, &iter, UI_CONTACTS_COLUMN_IS_GROUP, FALSE, -1);	//TODO g_strdup ???
			gdk_pixbuf_unref (pix);

			if (iter_parent)
				gtk_tree_iter_free (iter_parent);
		}

		g_free (uuid);
		g_free (display);
		g_free (group);

		list = g_slist_next (list);
	}


	g_slist_foreach (list_start, gnomegadu_conf_free_list_of_string, NULL);
	g_slist_free (list_start);

	gtk_tree_view_set_model (contacts_tree_view, GTK_TREE_MODEL (contacts_tree_store));

	gtk_tree_view_expand_all (contacts_tree_view);
}

static gboolean
userlist_combo_separator_func (GtkTreeModel * model, GtkTreeIter * iter, gpointer data)
{
	gint status;
	gtk_tree_model_get (model, iter, UI_STATUS_COLUMN_STATUS, &status, -1);
	if (status == GNOMEGADU_STATUS_UNKNOWN)
		return TRUE;

	return FALSE;
}

void
gnomegadu_ui_init_statusbar ()
{
	GtkComboBox *combobox = GTK_COMBO_BOX (glade_xml_get_widget (gladexml, "StatusComboBox"));
	GtkListStore *status_store = NULL;
	GtkCellRenderer *render_text, *render_pixbuf;
	GtkTreeIter iter, iter_init;
	GdkPixbuf *pixbuf;

	gtk_cell_layout_clear (GTK_CELL_LAYOUT (combobox));

	status_store = gtk_list_store_new (UI_STATUS_N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_INT);

	render_pixbuf = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combobox), render_pixbuf, FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combobox), render_pixbuf, "pixbuf", UI_STATUS_COLUMN_ICON, NULL);

	render_text = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combobox), render_text, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combobox), render_text, "markup", UI_STATUS_COLUMN_NAME, NULL);

	pixbuf = create_pixbuf (USER_AVAIL_ICON);
	gtk_list_store_append (status_store, &iter);
	gtk_list_store_set (status_store, &iter, UI_STATUS_COLUMN_ICON, pixbuf, UI_STATUS_COLUMN_NAME, "Dostępny",
			    UI_STATUS_COLUMN_STATUS, GNOMEGADU_STATUS_AVAIL, -1);
	gdk_pixbuf_unref (pixbuf);

	pixbuf = create_pixbuf (USER_AWAY_ICON);
	gtk_list_store_append (status_store, &iter);
	gtk_list_store_set (status_store, &iter, UI_STATUS_COLUMN_ICON, pixbuf, UI_STATUS_COLUMN_NAME, "Zajęty",
			    UI_STATUS_COLUMN_STATUS, GNOMEGADU_STATUS_BUSY, -1);
	gdk_pixbuf_unref (pixbuf);

	pixbuf = create_pixbuf (USER_INVISIBLE_ICON);
	gtk_list_store_append (status_store, &iter);
	gtk_list_store_set (status_store, &iter, UI_STATUS_COLUMN_ICON, pixbuf, UI_STATUS_COLUMN_NAME, "Niewidoczny",
			    UI_STATUS_COLUMN_STATUS, GNOMEGADU_STATUS_INVISIBLE, -1);
	gdk_pixbuf_unref (pixbuf);

	pixbuf = create_pixbuf (USER_NOTAVAIL_ICON);
	gtk_list_store_append (status_store, &iter_init);
	gtk_list_store_set (status_store, &iter_init, UI_STATUS_COLUMN_ICON, pixbuf, UI_STATUS_COLUMN_NAME, "Niedostępny",
			    UI_STATUS_COLUMN_STATUS, GNOMEGADU_STATUS_UNAVAIL, -1);
	gdk_pixbuf_unref (pixbuf);

	gtk_list_store_append (status_store, &iter);
	gtk_list_store_set (status_store, &iter, UI_STATUS_COLUMN_STATUS, GNOMEGADU_STATUS_UNKNOWN, -1);

	pixbuf = create_pixbuf (ADD_DESCRIPTION_ICON);
	gtk_list_store_append (status_store, &iter);
	gtk_list_store_set (status_store, &iter, UI_STATUS_COLUMN_ICON, pixbuf, UI_STATUS_COLUMN_NAME, "Ustaw opis",
			    UI_STATUS_COLUMN_STATUS, GNOMEGADU_STATUS_DESC, -1);
	gdk_pixbuf_unref (pixbuf);

	gtk_combo_box_set_row_separator_func (combobox, userlist_combo_separator_func, NULL, NULL);

	gtk_combo_box_set_model (combobox, GTK_TREE_MODEL (status_store));
	gtk_combo_box_set_active_iter (combobox, &iter_init);
	g_object_unref (status_store);
}
