#ifndef GNOME_GADU_UI_CHAT_H
#define GNOME_GADU_UI_CHAT_H 1

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <glade/glade.h>
#include "gnomegadu_protocol.h"

enum
{
	UI_CHAT_COLUMN_ICON,
	UI_CHAT_COLUMN_DISPLAYED,
	UI_CHAT_COLUMN_UUID,
	UI_CHAT_N_COLUMNS
};

typedef enum
{
	GNOMEGADU_CHAT_RCV,
	GNOMEGADU_CHAT_SND
} GnomeGaduChatType;

//GList *chat_windows_list;

GladeXML *gnomegadu_ui_chat_find (GList * uin_list, gboolean autocreate);
void gnomegadu_ui_chat_append_text(GladeXML *chat_window_xml, const gchar *txt, GnomeGaduChatType type, gchar *uin_sender_str);
void gnomegadu_ui_chats_set_status (gchar * uin_str, GnomeGaduProtocolStatus new_status, gchar * status_descr);
void gnomegadu_ui_chats_userlist_cleanup_model_status ();

void gnomegadu_ui_chat_window_configure(GladeXML *chat_window_xml, GList *uin_list);
#endif
