#ifndef GNOME_GADU_TRAY_H
#define GNOME_GADU_TRAY_H 1

#include <glade/glade.h>

#include "eggtrayicon.h"


EggTrayIcon *tray_icon;
GtkWidget *tray_image;
GladeXML *gladexml_tray_menu;

void gnomegadu_tray_init ();

#endif
