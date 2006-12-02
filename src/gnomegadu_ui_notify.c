#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glade/glade.h>

#include <libnotify/notify.h>

#include "config.h"
#include "gnomegadu_ui.h"
#include "gnomegadu_conf.h"
#include "gnomegadu_protocol.h"
#include "gnomegadu_userlist.h"
#include "gnomegadu_ui_notify.h"
#include "gnomegadu_tray.h"

gboolean
gnomegadu_ui_notify_closed_cb (NotifyNotification *notification)
{
	g_object_unref (G_OBJECT(notification));
	return FALSE;
}

gboolean gnomegadu_ui_notify_show(gchar *title, gchar *txt, GdkPixbuf *pix)
{
		NotifyNotification *notification = NULL;
	
		if (!notify_is_initted())
		{
			g_printerr("libnotify is not initted, should be, check you installation!\n");
			return FALSE;
		}
		
		notification = notify_notification_new (title, txt, NULL,tray_icon);
		notify_notification_set_icon_from_pixbuf(notification,pix);		
		notify_notification_set_urgency (notification, NOTIFY_URGENCY_LOW);
		notify_notification_set_timeout(notification,4000);
	
		g_signal_connect (notification, "closed", G_CALLBACK(gnomegadu_ui_notify_closed_cb), NULL);
	
		return notify_notification_show (notification, NULL);
}
