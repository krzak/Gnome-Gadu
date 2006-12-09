#ifndef GNOME_GADU_STOCK_H
#define GNOME_GADU_STOCK_H 1

#include <gtk/gtk.h>

/*
#define USER_AVAIL_ICON "user_orange.png"
#define USER_AWAY_ICON "user_orange_away.png"
#define USER_NOTAVAIL_ICON "user.png"
#define USER_INVISIBLE_ICON "user_red.png"
#define USER_GROUP_ICON "group.png"
#define ADD_DESCRIPTION_ICON "font_add.png"
*/

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
