#include <gtk/gtk.h>
#include <string.h>
#include <gconf/gconf-client.h>
#include <libgnome/libgnome.h>
#include <glade/glade.h>
#include <gnome-keyring-1/gnome-keyring.h>

#include "config.h"
#include "gnomegadu_conf.h"
#include "gnomegadu_ui.h"
#include "gnomegadu_userlist.h"
#include "gnomegadu_protocol.h"

static GnomeKeyringAttributeList *gnomegadu_conf_construct_keyring_attributes (gchar * uin);


gboolean
gnomegadu_conf_rename_account (gchar * old_name, gchar * new_name)
{
	gboolean ret = FALSE;
	gchar *default_account, *profile;
	gchar *account_path = gnomegadu_conf_find_account_path (old_name);
	gchar *name_path = g_strconcat (account_path, "/name", NULL);

	g_print ("rename: %s -> %s\n", old_name, new_name);

	ret = gconf_client_set_string (gconf, name_path, g_strdup (new_name), NULL);

	default_account = gnomegadu_conf_get_default_account_name ();
	if (default_account && !g_utf8_collate (default_account, old_name))
		gnomegadu_conf_set_default_account_name (new_name);

	profile = gnomegadu_conf_get_profile ();
	if (profile && !g_utf8_collate (old_name, profile)) {
		gchar *path = g_strconcat (GNOMEGADU_CONF_ROOT "/profile", NULL);
		ret = gconf_client_set_string (gconf, path, g_strdup (new_name), NULL);
		g_free (path);
	}

	gconf_client_suggest_sync (gconf, NULL);

	g_free (default_account);
	g_free (account_path);
	g_free (name_path);

	return ret;
}

//zrwocony tekst należy zwolnic
gchar *
gnomegadu_conf_add_account (gchar * name, gchar * uin, gchar * password)
{
	GtkWidget *dialog;
	gchar *path;
	gint32 next_num = g_random_int_range (1, 65555);
	gchar *account_name;
	gchar *name_name;

	if (name && gnomegadu_conf_find_account_path (name)) {
		dialog = gtk_message_dialog_new_with_markup (NULL,
							     GTK_DIALOG_DESTROY_WITH_PARENT,
							     GTK_MESSAGE_ERROR,
							     GTK_BUTTONS_CLOSE,
							     "<span weight='bold' size='larger'>Primary Text</span>\n\n<span>Konto o nazwie '%s' jest już na liście</span>",
							     name);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (GTK_WIDGET (dialog));
		return NULL;
	}

	account_name = g_strdup_printf ("%s%d", "losowe", next_num);
	//znajdz kolejny numer, aż jakiś będzie wolny
	while (gnomegadu_conf_find_account_path (account_name)) {
		next_num = g_random_int ();
		g_free (account_name);
		account_name = g_strdup_printf ("%s%d", "losowe", next_num);
	}

	name_name = name ? g_strdup (name) : g_strdup (account_name);

	path = g_strconcat (GNOMEGADU_CONF_ROOT "/accounts/", account_name, "/name", NULL);
	gconf_client_set_string (gconf, path, name_name, NULL);
	g_free (path);

	if (uin)
		gnomegadu_conf_set_account_uin (name_name, uin);

	if (password)
		gnomegadu_conf_set_account_password (name_name, password);

	gconf_client_suggest_sync (gconf, NULL);
	g_free (account_name);
	return name_name;
}

//Seek&destroy
//TODO /profile <- to musi sie zaktualizować, ale co w przypadku gdy kasuje ostatnie konto ?
//Kasowac z keyring
gboolean
gnomegadu_conf_del_account (gchar * account_name)
{
	GError *error;
	gchar *path;
	gchar *root;
	gchar *default_account;
	gchar *keyring = NULL;
	gchar *uin = NULL;
	GnomeKeyringAttributeList *attributes = NULL;
	GnomeKeyringResult keyringret;
	GList *result;

	g_assert (account_name);

	g_print ("Trying remove from keyring\n");

	root = gnomegadu_conf_find_account_path (account_name);
	if (root) {
		path = g_strconcat (root, "/name", NULL);

		g_print ("root found with path %s\n", path);

		//Keyring part
		uin = gnomegadu_conf_get_account_uin(account_name);

		gnome_keyring_get_default_keyring_sync (&keyring);
		uin = gnomegadu_conf_get_account_uin (account_name);

		if (keyring && uin)
		{
			attributes = gnomegadu_conf_construct_keyring_attributes (uin);
			keyringret = gnome_keyring_find_items_sync (GNOME_KEYRING_ITEM_NETWORK_PASSWORD,
								    attributes,
								    &result);

			if (keyringret) {
				g_print ("Couldn't get UIN: %s\n", account_name);
			} else if (g_list_length (result) == 1) {
				GnomeKeyringFound *found = (GnomeKeyringFound *) result->data;
				keyringret = gnome_keyring_item_delete_sync (keyring, found->item_id);
				g_print ("Remove from keyring: %s %d\n", uin, keyringret);
			}

			gnome_keyring_found_list_free (result);
			gnome_keyring_attribute_list_free (attributes);
		}

		/* gconf part */
		if (!gconf_client_recursive_unset (gconf, root, 0, &error)) {
			g_printerr ("unable remove: %s, %s\n", root, error->message);
			g_free (root);
			g_free (path);
			return FALSE;
		}

		default_account = gnomegadu_conf_get_default_account_name ();
		if (default_account && !g_strcasecmp (default_account, account_name))
			gnomegadu_conf_set_default_account_name (NULL);

		gconf_client_suggest_sync (gconf, NULL);



		g_free (default_account);
		g_free (root);
		g_free (path);
		g_free (uin);
		g_free (keyring);
	}
	return TRUE;
}

GSList *
gnomegadu_conf_get_accounts ()
{
	return gconf_client_all_dirs (gconf, GNOMEGADU_CONF_ROOT "/accounts", NULL);
}

void
gnomegadu_conf_sound_enabled_notify (GConfClient * client, guint cnxn_id, GConfEntry * entry, gpointer user_data)
{
	GConfValue *value = gconf_entry_get_value (entry);
	int val = gconf_value_get_bool (value);

	if (val == TRUE) {
		gnome_sound_init (NULL);
	} else {
		gnome_sound_shutdown ();
	}
}

void
gnomegadu_conf_free_list_of_string (gpointer data, gpointer user_data)
{
	g_free (data);
}

// Trzbeba zwolnic zwrócony tekst
gchar *
gnomegadu_conf_find_account_path (gchar * account_name)
{
	gchar *path;
	gchar *root;
	gchar *check_name;
	gchar *ret = NULL;
	GSList *list, *list_start;

	list = gnomegadu_conf_get_accounts ();
	list_start = list;

	if (!list)
		return NULL;

	while (list) {
		root = (gchar *) list->data;
		path = g_strconcat (root, "/name", NULL);
		check_name = gconf_client_get_string (gconf, path, NULL);
		if (check_name && !g_strcasecmp (check_name, account_name)) {
			ret = g_strdup (root);
			g_free (path);
			break;
		}
		g_free (path);
		list = g_slist_next (list);
	}

	g_slist_foreach (list_start, gnomegadu_conf_free_list_of_string, NULL);
	g_slist_free (list_start);
	return ret;
}

gchar *
gnomegadu_conf_get_account_uin (gchar * account_name)
{
	gchar *root = gnomegadu_conf_find_account_path (account_name);
	gchar *path = g_strconcat (root, "/uin", NULL);

	return gconf_client_get_string (gconf, path, NULL);

	g_free (root);
	g_free (path);
}


gboolean
gnomegadu_conf_set_account_uin (gchar * account_name, gchar * value_uin)
{
	gchar *keyring = NULL;
	gchar *uin = NULL;
	gchar *current_password = NULL;
	GnomeKeyringAttributeList *attributes = NULL;
	GnomeKeyringResult keyringret;
	GList *result;
	gboolean ret;
	gboolean change = FALSE;

	//Keyring part
	gnome_keyring_get_default_keyring_sync (&keyring);
	uin = gnomegadu_conf_get_account_uin (account_name);

	if (keyring && g_strcasecmp (uin, value_uin) && uin && value_uin && (strlen (uin) > 0) && (strlen (value_uin) > 0))	//zmiana uin
	{
		attributes = gnomegadu_conf_construct_keyring_attributes (uin);
		keyringret = gnome_keyring_find_items_sync (GNOME_KEYRING_ITEM_NETWORK_PASSWORD,	/* type */
							    attributes,	/* attribute list */
							    &result);


		if (keyringret) {
			g_print ("Couldn't get UIN: %s\n", account_name);
		} else if (g_list_length (result) == 1) {
			current_password = gnomegadu_conf_get_account_password (account_name);

			GnomeKeyringFound *found = (GnomeKeyringFound *) result->data;
			keyringret = gnome_keyring_item_delete_sync (keyring, found->item_id);

			g_print ("Remove from keyring: %s %d\n", uin, keyringret);
			change = TRUE;

		}

		gnome_keyring_found_list_free (result);
		gnome_keyring_attribute_list_free (attributes);
	}
	g_free (uin);

	//GConf part
	gchar *root = gnomegadu_conf_find_account_path (account_name);
	gchar *path = g_strconcat (root, "/uin", NULL);

	ret = gconf_client_set_string (gconf, path, g_strdup (value_uin), NULL);

	if (current_password && change)
		gnomegadu_conf_set_account_password (account_name, current_password);

	g_free (root);
	g_free (path);

	return ret;
}

gboolean
gnomegadu_conf_set_profile (gchar * account_name)
{
	GtkWindow *window;
	gchar *path, *path_contacts, *path_profile;
	gchar *prev_profile, *prev_profile_path;
	gboolean ret = FALSE;
	static guint cnxn_id = 0;

	g_assert (gladexml);

	window = GTK_WINDOW (glade_xml_get_widget (gladexml, "MainWindow"));
	gtk_window_set_title (window, g_strdup (account_name));

	path = g_strconcat (GNOMEGADU_CONF_ROOT "/profile", NULL);

	ret = gconf_client_set_string (gconf, path, g_strdup (account_name), NULL);
	gnomegedu_ui_init_userlist ();

	prev_profile = gconf_client_get_string (gconf, path, NULL);
	if (prev_profile) {
		prev_profile_path = gnomegadu_conf_find_account_path (prev_profile);
		path_contacts = g_strconcat (prev_profile_path, "/contacts", NULL);

		if (cnxn_id)
			gconf_client_notify_remove (gconf, cnxn_id);

		g_free (path_contacts);
		g_free (prev_profile_path);
	}

	path_profile = gnomegadu_conf_find_account_path (account_name);
	path_contacts = g_strconcat (path_profile, "/contacts", NULL);
	gconf_client_notify_add (gconf, path_contacts, (GConfClientNotifyFunc) gconf_client_contacts_value_changed_cb, NULL,
				 NULL, NULL);
	gconf_client_suggest_sync (gconf, NULL);

	g_free (path_profile);
	g_free (path_contacts);
	g_free (path);
	return ret;
}

gchar *
gnomegadu_conf_get_profile ()
{
	gchar *ret = NULL;
	gchar *path = g_strconcat (GNOMEGADU_CONF_ROOT "/profile", NULL);
	ret = gconf_client_get_string (gconf, path, NULL);
	g_free (path);
	return ret;
}

gchar *
gnomegadu_conf_get_account_password (gchar * account_name)
{
	gchar *keyring = NULL;
	gchar *ret = NULL;
	gchar *uin = NULL;
	gchar *pass = NULL;
	GnomeKeyringAttributeList *attributes = NULL;
	GnomeKeyringResult keyringret;
	GList *result;

	gnome_keyring_get_default_keyring_sync (&keyring);

	if (keyring) {
		uin = gnomegadu_conf_get_account_uin (account_name);
		attributes = gnomegadu_conf_construct_keyring_attributes (uin);
		keyringret = gnome_keyring_find_items_sync (GNOME_KEYRING_ITEM_NETWORK_PASSWORD,	/* type */
							    attributes,	/* attribute list */
							    &result);


		if (keyringret) {
			g_print ("Couldn't get password: %s\n", account_name);
		} else if (g_list_length (result) == 1) {
			GnomeKeyringFound *found = (GnomeKeyringFound *) result->data;
			if (found->secret)
				pass = g_strdup (found->secret);

			if (g_utf8_strlen (pass, -1) > 0)
				ret = pass;
			else
				g_free (pass);
		}

		gnome_keyring_found_list_free (result);
		gnome_keyring_attribute_list_free (attributes);
		g_free (uin);
	}

	return ret;
}

/* KEYRING SUPPORT */
gboolean
gnomegadu_conf_set_account_password (gchar * account_name, gchar * value_password)
{
	gboolean ret = FALSE;
	gchar *uin = NULL;
	gchar *keyring = NULL;
	GnomeKeyringAttributeList *attributes = NULL;
	GnomeKeyringResult keyringret;
	guint32 item;

	gnome_keyring_get_default_keyring_sync (&keyring);

	if (keyring) {
		uin = gnomegadu_conf_get_account_uin (account_name);
		attributes = gnomegadu_conf_construct_keyring_attributes (uin);

		keyringret = gnome_keyring_item_create_sync (keyring,	/* Use default keyring */
							     GNOME_KEYRING_ITEM_NETWORK_PASSWORD,	/* type */
							     "GnomeGadu Password",	/* name */
							     attributes,	/* attribute list */
							     value_password,	/* password */
							     TRUE,	/* Update if already exists */
							     &item);

		gnome_keyring_attribute_list_free (attributes);

		if (keyringret != GNOME_KEYRING_RESULT_OK && keyringret != GNOME_KEYRING_RESULT_ALREADY_EXISTS)
			ret = FALSE;
		else
			ret = TRUE;

		g_print ("Save password in keyring %s %d %d\n", keyring, ret, keyringret);

		g_free (uin);
	}

	return ret;
}

gchar *
gnomegadu_conf_get_default_account_name ()
{
	gchar *path;
	gchar *account_name;

	path = g_strconcat (GNOMEGADU_CONF_ROOT "/default_account", NULL);
	account_name = gconf_client_get_string (gconf, path, NULL);

	g_free (path);
	return account_name;
}

gboolean
gnomegadu_conf_set_default_account_name (gchar * account_name)
{
	gchar *path;
	gboolean ret;

	path = g_strconcat (GNOMEGADU_CONF_ROOT "/default_account", NULL);

	if (!account_name)
		ret = gconf_client_unset (gconf, path, NULL);
	else {
		ret = gconf_client_set_string (gconf, path, g_strdup (account_name), NULL);
	}

	g_free (path);
	return ret;
}


void
gnomegadu_conf_init ()
{
	GError *error = NULL;
	;
	gconf = gconf_client_get_default ();

	gconf_client_add_dir (gconf, GNOMEGADU_CONF_ROOT, GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);

	if (!gconf_client_notify_add
	    (gconf, GNOMEGADU_CONF_ROOT "/sound", gnomegadu_conf_sound_enabled_notify, NULL, NULL, &error))
		g_printerr ("%s\n", error->message);

	gconf_client_suggest_sync (gconf, NULL);
}

GSList *
gnomegadu_conf_get_contacts ()
{
	gchar *profile = gnomegadu_conf_get_profile ();
	gchar *profile_path = gnomegadu_conf_find_account_path (profile);
	gchar *path = g_strconcat (profile_path, "/contacts", NULL);
	GSList *list = NULL;

	list = gconf_client_all_dirs (gconf, path, NULL);

	g_free (profile);
	g_free (profile_path);
	g_free (path);
	return list;
}

gchar *
gnomegadu_conf_contact_path_find_uin (gchar * uin)
{
	gchar *path;
	gchar *root;
	gchar *check_uin;
	gchar *ret = NULL;
	GSList *list, *list_start;

	list = gnomegadu_conf_get_contacts ();
	list_start = list;

	if (!list)
		return NULL;

	while (list) {
		root = (gchar *) list->data;
		path = g_strconcat (root, "/uin", NULL);
		check_uin = gconf_client_get_string (gconf, path, NULL);
		g_free (path);

		if (check_uin && !g_ascii_strcasecmp (check_uin, uin)) {
			ret = g_strdup (root);
			g_free (check_uin);
			break;
		}

		g_free (check_uin);
		list = list->next;
	}

	g_slist_foreach (list_start, gnomegadu_conf_free_list_of_string, NULL);
	g_slist_free (list_start);
	return ret;
}


gchar *
gnomegadu_conf_contact_path_find_uuid (gchar * uuid)
{
	gchar *path;
	gchar *root;
	gchar *check_uuid;
	gchar *ret = NULL;
	GSList *list, *list_start;

	if (!uuid) {
		g_printerr ("gnomegadu_conf_contact_path_find_uuid(): no UUID parameter\n");
		return NULL;
	}

	list = gnomegadu_conf_get_contacts ();
	list_start = list;

	if (!list)
		return NULL;

	while (list) {
		root = (gchar *) list->data;
		path = g_strconcat (root, "/uuid", NULL);
		check_uuid = gconf_client_get_string (gconf, path, NULL);
		if (check_uuid && !g_strcasecmp (check_uuid, uuid)) {
			ret = g_strdup (root);
			g_free (check_uuid);
			g_free (path);
			break;
		}
		g_free (check_uuid);
		g_free (path);
		list = list->next;
	}

	g_slist_foreach (list_start, gnomegadu_conf_free_list_of_string, NULL);
	g_slist_free (list_start);

	return ret;
}

static
gchar *
gnomegadu_conf_contact_get_a_for_uuid (gchar * a, gchar * uuid)
{
	gchar *display = NULL;
	gchar *tmp = NULL;
	gchar *path = NULL;

	path = gnomegadu_conf_contact_path_find_uuid (uuid);
	if (path) {
		tmp = g_strconcat (path, a, NULL);
		display = gconf_client_get_string (gconf, tmp, NULL);
	} else {
		display = g_strdup (uuid);
	}

	g_free (tmp);
	g_free (path);

	return display;
}

gchar *
gnomegadu_conf_contact_get_uin_for_uuid (gchar * uuid)
{
	return gnomegadu_conf_contact_get_a_for_uuid ("/uin", uuid);
}


gchar *
gnomegadu_conf_contact_get_display_for_uuid (gchar * uuid)
{
	return gnomegadu_conf_contact_get_a_for_uuid ("/display", uuid);
}

gchar *
gnomegadu_conf_contact_get_uuid_for_uin (gchar * uin)
{
	gchar *uuid = NULL;
	gchar *tmp = NULL;
	gchar *path = NULL;

	path = gnomegadu_conf_contact_path_find_uin (uin);
	if (path) {
		tmp = g_strconcat (path, "/uuid", NULL);
		uuid = gconf_client_get_string (gconf, tmp, NULL);
	} else {
		uuid = g_strdup (uin);	//hm...
	}

	g_free (tmp);
	g_free (path);
	return uuid;
}


static
GnomeKeyringAttributeList *
gnomegadu_conf_construct_keyring_attributes (gchar * uin)
{
	GnomeKeyringAttributeList *ret_attributes = NULL;
	GnomeKeyringAttribute attribute;
	ret_attributes = gnome_keyring_attribute_list_new ();

	attribute.name = g_strdup ("protocol");
	attribute.type = GNOME_KEYRING_ATTRIBUTE_TYPE_STRING;
	attribute.value.string = g_strdup ("gadugadu");	//gnome apps use gadugadu instead gg
	g_array_append_val (ret_attributes, attribute);

	attribute.name = g_strdup ("user");
	attribute.type = GNOME_KEYRING_ATTRIBUTE_TYPE_STRING;
	attribute.value.string = g_strdup (uin);
	g_array_append_val (ret_attributes, attribute);

	return ret_attributes;
}

// DEFAULTS FROM SCHEMA!!!!
/*
 * GConfValue *val = gconf_value_new (GCONF_VALUE_BOOL); if
 * (!gconf_client_dir_exists(gconf,"/apps/gnomegadu",NULL)) {
 * gconf_value_set_bool(val,TRUE);
 * gconf_client_set(gconf,"/apps/gnomegadu/sound/enabled",val,NULL); } 
 */
