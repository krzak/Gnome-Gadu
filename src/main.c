#include <config.h>

#include <gtk/gtk.h>
#include <libgnome/libgnome.h>
#include <libgnomeui/libgnomeui.h>
#include <gconf/gconf-client.h>
#include <libnotify/notify.h>
#include <libgadu.h>

#include "gnomegadu_ui.h"
#include "gnomegadu_conf.h"
#include "gnomegadu_userlist.h"
#include "gnomegadu_tray.h"
#include "gnomegadu_galago.h"

int
main (int argc, char **argv)
{
	GOptionContext *option_context;
	GnomeProgram *gnomegadu_app;
	gchar **remaining_args = NULL;
	
	GOptionEntry option_entries[] = {
		{G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY,
		 &remaining_args,
		 "Special option that collects any remaining arguments for us"},
		{NULL}
	};


	gg_debug_level = 255;

	option_context = g_option_context_new ("gnomegadu-app");

	g_option_context_add_main_entries (option_context, option_entries,
					   NULL);

	gnomegadu_app = gnome_program_init (PACKAGE_NAME, PACKAGE_VERSION,
					    LIBGNOMEUI_MODULE, argc, argv,
					    GNOME_PARAM_GOPTION_CONTEXT,
					    option_context,
					    GNOME_PARAM_HUMAN_READABLE_NAME,
					    PACKAGE_READABLE_NAME,
					    GNOME_PARAM_NONE);



	if (remaining_args != NULL)
	{
		gint i, num_args;

		num_args = g_strv_length (remaining_args);
		for (i = 0; i < num_args; ++i)
		{
			/*
			 * process remaining_args[i] here 
			 */
		}
		g_strfreev (remaining_args);
		remaining_args = NULL;
	}


	gnomegadu_conf_init (gnomegadu_app);

	if (gconf_client_get_bool (gconf, g_strconcat(gnomegadu_gconf_relative_path, "/sound",NULL), NULL))
		gnome_sound_init (NULL);

	gnomegadu_ui_init ();

	if (!notify_is_initted())
		notify_init(PACKAGE);


	gnomegadu_protocol_is_connected	= FALSE;
	//gnomegadu_galago_init();
	
	gtk_main ();

	g_object_unref (gnomegadu_app);

	return 0;
}
