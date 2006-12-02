#ifndef GNOME_GADU_UI_STATUS_H
#define GNOME_GADU_UI_STATUS_H 1

#include <glade/glade.h>

#include "gnomegadu_protocol.h"

GladeXML *gladexml_userlist_status_desc;

const gchar *gnomegadu_ui_status_get_icon_name(GnomeGaduProtocolStatus status);

#endif
