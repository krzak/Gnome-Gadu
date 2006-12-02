#ifndef GNOME_GADU_UI_NOTIFY_H
#define GNOME_GADU_UI_NOTIFY_H 1

#include <libnotify/notify.h>

gboolean gnomegadu_ui_notify_show(gchar *title, gchar *txt, GdkPixbuf *pix);

#endif
