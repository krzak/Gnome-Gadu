#ifndef GNOME_GADU_TRAY_H
#define GNOME_GADU_TRAY_H 1

#include <glade/glade.h>

GtkStatusIcon *status_icon;
GladeXML *gladexml_tray_menu;

void gnomegadu_tray_init ();
void gnomegadu_tray_blinking (int time);

#endif
