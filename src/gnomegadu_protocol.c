#include <string.h>

#include <gtk/gtk.h>
#include <libgadu.h>

#include "config.h"
#include "gnomegadu_ui.h"
#include "gnomegadu_stock.h"
#include "gnomegadu_conf.h"
#include "gnomegadu_protocol.h"
#include "gnomegadu_userlist.h"
#include "gnomegadu_tray.h"
#include "gnomegadu_ui_status.h"
#include "gnomegadu_ui_notify.h"
#include "gnomegadu_ui_chat.h"
#include "gnomegadu_ui_import_userlist.h"

static 
gboolean gnomegadu_protocol_ping(gpointer data)
{
	if (gnomegadu_gadugadu_session)
	    gg_ping (gnomegadu_gadugadu_session);
	else
	{
	    g_printerr("Ping disabled");
	    return FALSE;
	}
	    
	return TRUE;
}


gboolean
gnomegadu_protocol_loop (GIOChannel * source, GIOCondition condition, gpointer data)
{
	static gint prev_check = GG_CHECK_READ;
	gboolean ret = TRUE;
	struct gg_event *e = NULL;

	/* w przypadku bledu/utraty polaczenia postap tak jak w przypadku disconnect */
	if (!gnomegadu_gadugadu_session || !(e = gg_watch_fd (gnomegadu_gadugadu_session)) || (condition & G_IO_ERR) ||
	    ((condition & G_IO_HUP)
	     && ((gnomegadu_gadugadu_session->state != GG_STATE_CONNECTING_GG) && (gnomegadu_gadugadu_session->check != GG_CHECK_WRITE))))
	{
		g_printerr ("gnomegadu_protocol_loop(): some kind of problem while processing\n");
		gnomegadu_gadugadu_disconnect ();
		prev_check = GG_CHECK_READ;
		return FALSE;
	}

	switch (e->type)
	{
	case GG_EVENT_NONE:
		g_print ("GG_EVENT_NONE\n");
		break;

	case GG_EVENT_PONG:
		g_print ("GG_EVENT_PONG\n");
		gg_ping (gnomegadu_gadugadu_session);
		break;

	case GG_EVENT_CONN_FAILED:
		g_print ("GG_EVENT_CONN_FAILED\n");
		gnomegadu_gadugadu_disconnect ();
		ret = FALSE;
		break;

	case GG_EVENT_DISCONNECT:
		g_print ("GG_EVENT_DISCONNECT\n");
		gnomegadu_gadugadu_disconnect ();
		ret = FALSE;
		break;

	case GG_EVENT_CONN_SUCCESS:
	{
		g_print ("GG_EVENT_CONN_SUCCESS\n");
		//wysyłam swoją listę
		gint i, j = 0;
		uint32_t *uins;
		GSList *contacts_list = gnomegadu_conf_get_contacts ();
		GSList *contacts_list_start = contacts_list;

		gnomegadu_protocol_is_connected = TRUE;

		if (!contacts_list)
			break;

		i = g_slist_length (contacts_list);
		uins = g_malloc0 (i * sizeof (uint32_t));
		while (contacts_list)
		{
			gchar *uin_str;
			gchar *path = (gchar *) contacts_list->data;
			gchar *uin_path = g_strconcat (path, "/uin", NULL);
			uin_str = gconf_client_get_string (gconf, uin_path, NULL);
			uins[j++] = g_strtod (uin_str, NULL);

			g_free (uin_path);
			contacts_list = g_slist_next(contacts_list);
		}

		if (j > 0)
			gg_notify (gnomegadu_gadugadu_session, uins, i);

		/* pingpong */
		g_timeout_add(100000, gnomegadu_protocol_ping, NULL);


		g_slist_foreach (contacts_list_start, gnomegadu_conf_free_list_of_string, NULL);
		g_slist_free (contacts_list_start);
		g_free (uins);
	}
		break;

//      case GG_EVENT_NOTIFY_DESCR:
	case GG_EVENT_NOTIFY:
	{
		gint i;
		g_print ("GG_EVENT_NOTIFY\n");

		for (i = 0; e->event.notify[i].uin; i++)
		{
			gchar *uin_str = g_strdup_printf ("%d", e->event.notify[i].uin);
			gint new_status = gnomegadu_gadugadu_from_protocol_status (e->event.notify[i].status);
			gnomegadu_userlist_set_model_status (uin_str, new_status, NULL);	//tutaj dodac obsluge opisów
			g_free (uin_str);
		}
	}
		break;
	case GG_EVENT_NOTIFY60:
	{
		gint i;
		g_print ("GG_EVENT_NOTIFY60\n");

		for (i = 0; e->event.notify60[i].uin; i++)
		{
			gchar *descr_utf = NULL;
			gchar *uin_str = g_strdup_printf ("%d", e->event.notify60[i].uin);
			gint new_status = gnomegadu_gadugadu_from_protocol_status (e->event.notify60[i].status);

			if (e->event.notify60[i].descr)
				descr_utf =
					g_convert (e->event.notify60[i].descr, strlen (e->event.notify60[i].descr), "UTF-8", "WINDOWS-1250", NULL, NULL, NULL);

			gnomegadu_userlist_set_model_status (uin_str, new_status, descr_utf);
			g_free (uin_str);
		}
	}
	break;
	case GG_EVENT_STATUS:
	case GG_EVENT_STATUS60:
	{
		GdkPixbuf *pix = NULL;
		gchar *descr_utf = NULL;
		gchar *uuid = NULL;
		gchar *display = NULL;
		gchar *notify_txt = NULL;
		gint new_status;
		gchar *uin_str  = NULL; 
		
		if (e->type == GG_EVENT_STATUS)
		{
		    new_status = gnomegadu_gadugadu_from_protocol_status (e->event.status.status);
		    uin_str    = g_strdup_printf ("%d", e->event.status.uin);

		    if (e->event.status.descr)
			descr_utf = g_convert (e->event.status.descr, strlen (e->event.status.descr), "UTF-8", "WINDOWS-1250", NULL, NULL, NULL);

		} else {
		    new_status = gnomegadu_gadugadu_from_protocol_status (e->event.status60.status);
		    uin_str    = g_strdup_printf ("%d", e->event.status60.uin);

		    if (e->event.status60.descr)
			descr_utf = g_convert (e->event.status60.descr, strlen (e->event.status60.descr), "UTF-8", "WINDOWS-1250", NULL, NULL, NULL);

		}
		
		gnomegadu_userlist_set_model_status (uin_str, new_status, descr_utf);

		uuid = gnomegadu_conf_contact_get_uuid_for_uin(uin_str);
		display = gnomegadu_conf_contact_get_display_for_uuid(uuid);

		pix = gnomegadu_stock_get_pixbuf (gnomegadu_ui_status_get_icon_name (new_status));
		notify_txt = g_strdup_printf("Status: %s\n<span size=\"smaller\" style=\"italic\">%s</span>",gnomegadu_protocol_status_txt(new_status),descr_utf ? descr_utf : "");
		gnomegadu_ui_notify_show (display, notify_txt, pix);
		g_object_unref (pix);
		
		gnomegadu_tray_blinking(2000);

		g_free (notify_txt);
		g_free (display);
		g_free (uuid);
		g_free (uin_str);
	}    
	break;
	case GG_EVENT_USERLIST:
	{
		g_print ("GG_EVENT_USERLIST\n");
		if (e->event.userlist.type == GG_USERLIST_GET_REPLY)
		{
			gint i;
			gfloat step;
			gchar **split_buf = NULL;
			GtkWidget *progress = glade_xml_get_widget (gladexml_import_userlist_progress, "ImportUserlistServerProgress");
			GtkWidget *progress_window = glade_xml_get_widget (gladexml_import_userlist_progress, "ImportUserlistServerProgressWindow");

			if (!progress_window || !progress)
				break;

			gchar *buf = e->event.userlist.reply;
			if (!buf)
			{
				gtk_widget_destroy(GTK_WIDGET(progress_window));
				
				GtkDialog *msgdialog = gtk_message_dialog_new_with_markup (NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
											   "<span weight=\"bold\"size=\"larger\">Lista jest pusta</span>\n\nLista kontaktów na serwerze jest pusta.");
				gtk_dialog_run (GTK_DIALOG (msgdialog));
				gtk_widget_destroy (GTK_WIDGET (msgdialog));
				break;
			}

			GConfChangeSet *changeset = gconf_change_set_new();

			split_buf = g_strsplit (buf, "\r\n", 2048);
			step = 1 / (gfloat)g_strv_length (split_buf);
			for (i = 0; i < g_strv_length (split_buf); i++)
			{
				gdouble percentage = gtk_progress_bar_get_fraction(GTK_PROGRESS_BAR (progress));
				
				if (percentage < 1)
					gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress), percentage + step);
				
				while (g_main_context_pending(NULL))
					g_main_context_iteration(NULL,TRUE);

				gnomegadu_ui_import_userlist_process_line (split_buf[i], changeset);
				
				if (!gconf_client_commit_change_set(gconf, changeset, TRUE, NULL))
					g_printerr("Some error while import");
			}

			gconf_change_set_unref(changeset);

			gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress), 1);
			
			while (g_main_context_pending(NULL))
				g_main_context_iteration(NULL,TRUE);
			
			gtk_widget_destroy(GTK_WIDGET(progress_window));
		}

	}
	break;
	case GG_EVENT_MSG:
	{
		gchar *uin_sender = NULL;
		
		if (e->event.msg.msgclass == GG_CLASS_CHAT)
		{
			gint i      = 0;
			uin_sender  = g_strdup_printf("%d", e->event.msg.sender);
			gchar *txt  = g_strdup((gchar *)e->event.msg.message);
			gchar *txt_utf = NULL;
			GList *list = NULL;

			g_print("message from sender: (%s):%s\n",uin_sender,txt);
			
			/* first add sender */
			list 	  = g_list_append(list,g_strdup(uin_sender));

			if (e->event.msg.recipients_count > 0)
			{
				gchar *uin_str_2 = NULL;
				/* then add recipients */
				for (i = 0; i < e->event.msg.recipients_count; i++)
				{
				    uin_str_2 = g_strdup_printf("%d",e->event.msg.recipients[i]);
				    list = g_list_append(list,g_strdup(uin_str_2));
				    g_free(uin_str_2);

				    g_print("conference(%d): %s\n",e->event.msg.recipients_count,uin_str_2);
				}
			}

			g_assert(list);
			
			//chat window
			GladeXML *chat_window_xml = gnomegadu_ui_chat_find(list,TRUE);
			g_assert(chat_window_xml);

			txt_utf = g_convert (txt, strlen (txt), "UTF-8", "WINDOWS-1250", NULL, NULL, NULL);
			gnomegadu_ui_chat_append_text(chat_window_xml,txt_utf,GNOMEGADU_CHAT_RCV, uin_sender);

			g_free(txt);
			g_free(uin_sender);			
		}
	}
	break;

	}

	gg_free_event (e);

	if (gnomegadu_gadugadu_session && prev_check != gnomegadu_gadugadu_session->check)
	{
		prev_check = gnomegadu_gadugadu_session->check;

		if (gnomegadu_gadugadu_session->check == GG_CHECK_READ)
		{
			g_source_remove (gnomegadu_watch_protocol);
			gnomegadu_watch_protocol = g_io_add_watch (gnomegadu_source_chan, G_IO_IN | G_IO_ERR | G_IO_HUP, gnomegadu_protocol_loop, NULL);
			ret = FALSE;
		}

		if (gnomegadu_gadugadu_session->check == GG_CHECK_WRITE)
		{
			g_source_remove (gnomegadu_watch_protocol);
			gnomegadu_watch_protocol = g_io_add_watch (gnomegadu_source_chan, G_IO_OUT | G_IO_ERR | G_IO_HUP, gnomegadu_protocol_loop, NULL);
			ret = FALSE;
		}
	}

	return ret;
}


void
gnomegadu_gadugadu_connect (GnomeGaduProtocolStatus status)
{
	gchar *profile	= NULL;
	gchar *uin	= NULL;
	gchar *password = NULL;
	struct gg_login_params p;

	if (gnomegadu_gadugadu_session)
	{
		g_printerr ("gnomegadu_gadugadu_connect(): trying connect while session is not disconnected\n");
		return;
	}

	gnomegadu_userlist_cleanup_model_status ();	//just in case 

	profile = gnomegadu_conf_get_profile ();
	g_print("Connect profile: %s\n",profile);
	
	uin = gnomegadu_conf_get_account_uin (profile);
	password = gnomegadu_conf_get_account_password (profile);
	
	if (!password || !uin)
	    return;

	memset (&p, 0, sizeof (p));
	p.uin = g_ascii_strtod (uin, NULL);
	p.password = g_strdup (password);
	p.async = 1;
	p.status = gnomegadu_gadugadu_to_protocol_status (status,FALSE);

	gnomegadu_gadugadu_session = gg_login (&p);

	gnomegadu_source_chan = g_io_channel_unix_new (gnomegadu_gadugadu_session->fd);
	gnomegadu_watch_protocol = g_io_add_watch (gnomegadu_source_chan, G_IO_IN | G_IO_ERR | G_IO_HUP, gnomegadu_protocol_loop, NULL);

	g_free (profile);
	g_free (uin);
	g_free (password);
}

void
gnomegadu_gadugadu_disconnect ()
{
	GtkComboBox *combobox = GTK_COMBO_BOX (glade_xml_get_widget (gladexml, "StatusComboBox"));

	if (gnomegadu_gadugadu_session)
	{
		gg_logoff (gnomegadu_gadugadu_session);
		gg_free_session (gnomegadu_gadugadu_session);
	}

	gnomegadu_gadugadu_session = NULL;
	gnomegadu_protocol_is_connected = FALSE;

	if (gnomegadu_watch_protocol)
		g_source_remove (gnomegadu_watch_protocol);

	if (gnomegadu_source_chan)
		g_io_channel_unref (gnomegadu_source_chan);

	g_object_set_data (G_OBJECT (combobox), "dont_change_status", (gpointer) TRUE);
	gtk_combo_box_set_active (combobox, 3);
	g_object_set_data (G_OBJECT (combobox), "dont_change_status", (gpointer) FALSE);

	gnomegadu_userlist_cleanup_model_status ();
}


gint
gnomegadu_gadugadu_to_protocol_status (GnomeGaduProtocolStatus status, gboolean desc)
{
	gint ret;

	switch (status)
	{
	case GNOMEGADU_STATUS_AVAIL:
		ret = desc ? GG_STATUS_AVAIL_DESCR : GG_STATUS_AVAIL;
		break;
	case GNOMEGADU_STATUS_BUSY:
		ret = desc ? GG_STATUS_BUSY_DESCR : GG_STATUS_BUSY;
		break;
	case GNOMEGADU_STATUS_UNAVAIL:
		ret = desc ? GG_STATUS_NOT_AVAIL_DESCR : GG_STATUS_NOT_AVAIL;
		break;
	case GNOMEGADU_STATUS_INVISIBLE:
		ret = desc ? GG_STATUS_INVISIBLE_DESCR : GG_STATUS_INVISIBLE;
		break;
	default:
		ret = desc ? GG_STATUS_NOT_AVAIL_DESCR : GG_STATUS_NOT_AVAIL;
		break;
	}

	return ret;
}

GnomeGaduProtocolStatus
gnomegadu_gadugadu_from_protocol_status (gint status)
{
	gint ret;

	switch (status)
	{
	case GG_STATUS_AVAIL:
	case GG_STATUS_AVAIL_DESCR:
		ret = GNOMEGADU_STATUS_AVAIL;
		break;
	case GG_STATUS_BUSY:
	case GG_STATUS_BUSY_DESCR:
		ret = GNOMEGADU_STATUS_BUSY;
		break;
	case GG_STATUS_NOT_AVAIL:
	case GG_STATUS_NOT_AVAIL_DESCR:
	case GG_STATUS_BLOCKED:
		ret = GNOMEGADU_STATUS_UNAVAIL;
		break;
	case GG_STATUS_INVISIBLE:
	case GG_STATUS_INVISIBLE_DESCR:
		ret = GNOMEGADU_STATUS_INVISIBLE;
		break;
	default:
		ret = GNOMEGADU_STATUS_UNAVAIL;
		break;
	}

	return ret;
}


void
gnomegadu_protocol_add_notify (gchar * uin_str)
{
	if (gnomegadu_protocol_check_connected() && gnomegadu_gadugadu_session && uin_str && gnomegadu_protocol_is_valid_uin (uin_str))
		gg_add_notify (gnomegadu_gadugadu_session, g_ascii_strtod (uin_str, NULL));
}


void
gnomegadu_protocol_remove_notify (gchar * uin_str)
{
	if (gnomegadu_protocol_check_connected() && gnomegadu_gadugadu_session && uin_str && gnomegadu_protocol_is_valid_uin (uin_str))
		gg_remove_notify (gnomegadu_gadugadu_session, g_ascii_strtod (uin_str, NULL));
}

gboolean
gnomegadu_protocol_is_valid_uin (gchar * uin_str)
{
	int i = 0;
	gboolean valid = TRUE;

	if (!uin_str || (g_utf8_strlen (uin_str, -1) == 0))
		return FALSE;

	for (i = 0; i < g_utf8_strlen (uin_str, -1); i++)
		if (!g_unichar_isdigit (uin_str[i]))
			valid = FALSE;

	return valid;
}


gboolean gnomegadu_protocol_send_message (gchar * uin_str, gchar *msg)
{
    gchar *msg_windows1250 = g_convert (msg, strlen (msg), "WINDOWS-1250", "UTF-8", NULL, NULL, NULL);

    if (!gnomegadu_protocol_check_connected())
	return FALSE;

    if (gg_send_message(gnomegadu_gadugadu_session, GG_CLASS_CHAT, g_ascii_strtod (uin_str, NULL), msg_windows1250) == -1)
	return FALSE;

    g_free(msg_windows1250);
    return TRUE;
}

gboolean gnomegadu_protocol_send_message_confer (GList *uin_list_parm, gchar *msg)
{
    GList   *uin_list         = uin_list_parm;
    gint     recipients_count = g_list_length(uin_list);
    uin_t   *recipients       = g_malloc(recipients_count * sizeof (uin_t));
    gchar   *msg_windows1250  = NULL;
    gboolean ret              = TRUE;
    gint     i                = 0;
    
    g_assert(uin_list);
    
    if (!gnomegadu_protocol_check_connected())
	return FALSE;
    
    while (uin_list)
    {
    	if (uin_list->data != NULL)
		recipients[i++] = g_ascii_strtod((gchar *) uin_list->data,NULL);
		
	uin_list = g_list_next(uin_list);
    }
    
    msg_windows1250 = g_convert (msg, strlen (msg), "WINDOWS-1250", "UTF-8", NULL, NULL, NULL);

    if (gg_send_message_confer(gnomegadu_gadugadu_session, GG_CLASS_CHAT, recipients_count, recipients, msg_windows1250) == -1)
	ret = FALSE;

    g_free(msg_windows1250);
    g_free(recipients);
    
    return ret;
    /*czy tu nie trzeba zwolnic tego recipients ? */
}

const gchar *gnomegadu_protocol_status_txt(GnomeGaduProtocolStatus status)
{
    const gchar *ret = NULL;
    
    switch (status)
    {
	case GNOMEGADU_STATUS_AVAIL:
		ret = "Dostępny";
		break;
	case GNOMEGADU_STATUS_BUSY:
		ret = "Zajęty";
		break;
	case GNOMEGADU_STATUS_UNAVAIL:
		ret = "Niedostępny";
		break;
	case GNOMEGADU_STATUS_INVISIBLE:
		ret = "Ukryty";
		break;
	default:
		ret = "Niedostępny";
		break;
    }

    return ret;    
}

gint gnomegadu_protocol_change_status(GnomeGaduProtocolStatus status, gchar *descr)
{
    gboolean set_descr = FALSE;
    gchar *descr_windows1250 = NULL;
    gint ret = -1;

    if (!gnomegadu_protocol_check_connected())
	return FALSE;
    
    if (descr && g_utf8_strlen(descr,-1) > 0)
    {
	set_descr = TRUE;
	descr_windows1250 = g_convert (descr, strlen (descr), "WINDOWS-1250", "UTF-8", NULL, NULL, NULL);
    }

    if (set_descr)
        ret = gg_change_status_descr(gnomegadu_gadugadu_session,gnomegadu_gadugadu_to_protocol_status (status,set_descr ? TRUE : FALSE),descr_windows1250);
    else
	ret = gg_change_status(gnomegadu_gadugadu_session,gnomegadu_gadugadu_to_protocol_status (status, FALSE));

    g_free(descr_windows1250);
    return ret;
}


gboolean gnomegadu_protocol_check_connected()
{
    if (!gnomegadu_protocol_is_connected)
	return FALSE;
	
    return TRUE;
}
