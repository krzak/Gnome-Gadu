#include <string.h>
#include <gtk/gtk.h>

#include <glade/glade.h>
#include <libgadu.h>

#include "config.h"
#include "gnomegadu_ui.h"
#include "gnomegadu_conf.h"
#include "gnomegadu_userlist.h"
#include "gnomegadu_ui_status.h"
#include "gnomegadu_protocol.h"
#include "gnomegadu_tray.h"

void
on_StatusComboBox_changed (GtkComboBox * widget, gpointer user_data)
{
	GtkComboBox *combobox = GTK_COMBO_BOX (glade_xml_get_widget (gladexml, "StatusComboBox"));
	gboolean dont_change_status = (gboolean) g_object_get_data (G_OBJECT (combobox), "dont_change_status");
	GtkListStore *status_store = NULL;
	GtkTreeIter iter;
	gboolean valid;
	gint status;
	GdkPixbuf *pix;
	static gint active_status = GNOMEGADU_STATUS_UNAVAIL; //unavail
	static gint active_status_combo = 3;
	static gchar *active_descr = NULL;

	if (active_status_combo == gtk_combo_box_get_active(combobox))
		return;
	
	status_store = GTK_LIST_STORE (gtk_combo_box_get_model (combobox));
	valid = gtk_combo_box_get_active_iter (combobox, &iter);
	if (valid)
	{
		gtk_tree_model_get (GTK_TREE_MODEL (status_store), &iter, UI_STATUS_COLUMN_STATUS, &status, -1);

		if (status == GNOMEGADU_STATUS_DESC && active_status != GNOMEGADU_STATUS_UNAVAIL)
		{
			GtkWidget *dialog_desc;
			GtkWidget *desc_entry;
			const gchar *desc_txt = NULL;
			
			gladexml_userlist_status_desc = glade_xml_new (PACKAGE_DATA_DIR "/gnomegadu.glade", "SetStatusDescDialog", NULL);
			glade_xml_signal_autoconnect (gladexml_userlist_status_desc);
		
			dialog_desc = glade_xml_get_widget (gladexml_userlist_status_desc, "SetStatusDescDialog");
			desc_entry = glade_xml_get_widget (gladexml_userlist_status_desc, "StatusDescEntry");

			if (active_descr)
				gtk_entry_set_text(GTK_ENTRY(desc_entry),g_strdup(active_descr));
			
			g_assert(dialog_desc);
			
			gtk_dialog_run(GTK_DIALOG(dialog_desc));
			
			desc_txt = gtk_entry_get_text(GTK_ENTRY(desc_entry));
			
			//update static active_descr
			g_free(active_descr);
			active_descr = g_strdup(desc_txt);
			
			gnomegadu_protocol_change_status(active_status,active_descr);

			gtk_widget_destroy(dialog_desc);			
			gtk_combo_box_set_active(combobox,active_status_combo);
			return;
		} 
		else if (status == GNOMEGADU_STATUS_DESC && active_status == GNOMEGADU_STATUS_UNAVAIL)
		{
			gtk_combo_box_set_active(combobox,active_status_combo);
			return;
		}
		
		if ((!dont_change_status && status == GNOMEGADU_STATUS_AVAIL) || (status == GNOMEGADU_STATUS_BUSY || status == GNOMEGADU_STATUS_INVISIBLE))
		{
			if (!gnomegadu_protocol_is_connected)
				gnomegadu_gadugadu_connect (status);
			else if (gnomegadu_protocol_is_connected && gnomegadu_gadugadu_session)
			{
				//TODO sprawdzic czy sie udalo zmienic status
				gnomegadu_protocol_change_status(status,active_descr);
				
			}
		}

		if (status == GNOMEGADU_STATUS_UNAVAIL && !dont_change_status)
			gnomegadu_gadugadu_disconnect ();

		if (tray_image && status != GNOMEGADU_STATUS_DESC)
		{
			pix = create_pixbuf (gnomegadu_ui_status_get_icon_name (status));
			gtk_image_set_from_pixbuf (GTK_IMAGE (tray_image), pix);
			g_object_unref (pix);
		}
		
		active_status_combo = gtk_combo_box_get_active(combobox);
		active_status = status;
	}
}


const gchar *
gnomegadu_ui_status_get_icon_name (GnomeGaduProtocolStatus status)
{
	gchar *icon_type = USER_NOTAVAIL_ICON;

	if (status == GNOMEGADU_STATUS_AVAIL)
		icon_type = USER_AVAIL_ICON;

	if (status == GNOMEGADU_STATUS_BUSY)
		icon_type = USER_AWAY_ICON;

	if (status == GNOMEGADU_STATUS_UNAVAIL)
		icon_type = USER_NOTAVAIL_ICON;

	if (status == GNOMEGADU_STATUS_INVISIBLE)
		icon_type = USER_INVISIBLE_ICON;

	return icon_type;
}
