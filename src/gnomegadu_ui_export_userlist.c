#include <gtk/gtk.h>

#include <string.h>
#include <uuid/uuid.h>

#include <glade/glade.h>

#include "config.h"
#include "gnomegadu_conf.h"
#include "gnomegadu_ui.h"
#include "gnomegadu_protocol.h"
#include "gnomegadu_ui_export_userlist.h"


static GtkWidget *
gnomegadu_ui_create_vbox (GtkWidget * assistant, GtkAssistantPageType type, const gchar * title)
{
	GtkWidget *vbox;
	gchar *str = NULL;

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 24);
	gtk_assistant_append_page (GTK_ASSISTANT (assistant), vbox);
	gtk_assistant_set_page_type (GTK_ASSISTANT (assistant), vbox, type);
	gtk_assistant_set_page_title (GTK_ASSISTANT (assistant), vbox, title);
	gtk_assistant_set_page_complete (GTK_ASSISTANT (assistant), vbox, TRUE);

	return vbox;
}

static void
gnomegadu_ui_export_userlist_intro (GtkWidget * assistant)
{
	GtkWidget *label;
	GtkWidget *vbox;

	vbox = gnomegadu_ui_create_vbox (assistant, GTK_ASSISTANT_PAGE_INTRO, "Eksport listy kontaktów");

	label =
	    gtk_label_new
	    ("Gnome-Gadu umożliwia eksport kontaktów\ndo pliku lub na serwer.\n\nWyeksportowaną listę można zaimportować\ndo oryginalnego klienta Gadu-Gadu.");
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);
}

static void
gnomegadu_ui_export_userlist_select_source (GtkWidget * assistant)
{
	GtkWidget *label;
	GtkWidget *radio1, *radio2;
	GtkWidget *vbox;

	vbox = gnomegadu_ui_create_vbox (assistant, GTK_ASSISTANT_PAGE_CONTENT, "Eksport listy kontaktów2");

	label = gtk_label_new ("Wybierz miejsce do którego chcesz\nwyeksportować listę kontaktów:");
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);

	radio1 = gtk_radio_button_new_with_label (NULL, "Export na serwer za pomocą sieci Internet");
	radio2 = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio1), "Eksport do pliku tekstowego");

	gtk_box_pack_start (GTK_BOX (vbox), radio1, FALSE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (vbox), radio2, FALSE, TRUE, 2);

}

static gint
forward_page_function (gint current_page, gpointer data)
{
	g_print ("%d\n", current_page);
	switch (current_page) {
	case 0:
		return 1;
		break;
	case 1:
		return 1;
		break;
	}
	return -1;
}

gboolean
on_ContactsExport_activate (GtkWidget * widget, GdkEvent * event, gpointer user_data)
{
	GtkWidget *assistant = gtk_assistant_new ();

	g_object_set (G_OBJECT (assistant), "title", "Export listy kontaktów", NULL);
	gtk_window_set_position (GTK_WINDOW (assistant), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size (GTK_WINDOW (assistant), 440, 440);

	gtk_assistant_set_forward_page_func (assistant, forward_page_function, NULL, NULL);

	gnomegadu_ui_export_userlist_intro (assistant);
	gnomegadu_ui_export_userlist_select_source (assistant);

	g_signal_connect (G_OBJECT (assistant), "cancel", G_CALLBACK (gtk_widget_destroy), NULL);

	gtk_widget_show_all (assistant);

	return TRUE;
}
