#ifndef GNOME_GADU_UI_REGISTER_ACCOUNT_H
#define GNOME_GADU_UI_REGISTER_ACCOUNT_H 1

#include <gtk/gtk.h>
#include <glade/glade.h>

GladeXML *gladexml_account_register;
gchar* tokenid;
gchar* new_registered_uin;

void gnomegadu_ui_register_account_add();
void gnomegadu_ui_register_account_update_token();

#endif
