#ifndef GNOME_GADU_PROTOCOL_H
#define GNOME_GADU_PROTOCOL_H 1

#include <libgadu.h>

typedef enum
{
	GNOMEGADU_STATUS_UNKNOWN = 0,
	GNOMEGADU_STATUS_AVAIL,
	GNOMEGADU_STATUS_BUSY,
	GNOMEGADU_STATUS_INVISIBLE,
	GNOMEGADU_STATUS_UNAVAIL,
	GNOMEGADU_STATUS_DESC,
	GNOMEGADU_STATUS_C_ROWS
} GnomeGaduProtocolStatus;

struct gg_session *gnomegadu_gadugadu_session;
guint              gnomegadu_watch_protocol;
GIOChannel        *gnomegadu_source_chan;
gboolean           gnomegadu_protocol_is_connected;

void gnomegadu_gadugadu_connect (GnomeGaduProtocolStatus status);
void gnomegadu_gadugadu_disconnect ();

gint                    gnomegadu_gadugadu_to_protocol_status (GnomeGaduProtocolStatus status, gboolean desc);
GnomeGaduProtocolStatus gnomegadu_gadugadu_from_protocol_status (gint status);
const gchar            *gnomegadu_protocol_status_txt(GnomeGaduProtocolStatus status);

gint gnomegadu_protocol_change_status(GnomeGaduProtocolStatus status, gchar *descr);

void gnomegadu_protocol_add_notify    (gchar * uin_str);
void gnomegadu_protocol_remove_notify (gchar * uin_str);

gboolean gnomegadu_protocol_is_valid_uin (gchar * uin_str);
gboolean gnomegadu_protocol_send_message (gchar * uin_str, gchar *msg);
gboolean gnomegadu_protocol_send_message_confer (GList *uin_list_parm, gchar *msg);

gboolean gnomegadu_protocol_check_connected();

#endif
