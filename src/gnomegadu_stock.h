#ifndef GNOME_GADU_STOCK_H
#define GNOME_GADU_STOCK_H 1

#include <gtk/gtk.h>

#define GNOMEGADU_ICON_SIZE "gnomegadu-icon-size"

static const GtkStockItem gnomegadu_stock_items[] = {
	{"gnomegadu-user-available", "Dostępny", 0, 0, NULL},
	{"gnomegadu-user-not-available", "Niedostępny", 0, 0, NULL},
	{"gnomegadu-user-away", "Zajęty", 0, 0, NULL},
	{"gnomegadu-user-invisible", "Niewidzialny", 0, 0, NULL},
	{"gnomegadu-group", "Grupa", 0, 0, NULL},
	{"gnomegadu-description", "Opis", 0, 0, NULL}
};


void gnomegadu_stock_icons_init ();
GdkPixbuf *gnomegadu_stock_get_pixbuf(const gchar *stock_id);

#endif
