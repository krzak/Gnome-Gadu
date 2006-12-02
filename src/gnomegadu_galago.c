#include <gtk/gtk.h>
#include <string.h>
#include <gconf/gconf-client.h>
#include <libgalago/galago.h>

#include "config.h"
#include "gnomegadu_galago.h"

/* THIS IS EXPERIMENTAL */

void gnomegadu_galago_init()
{
	if (!galago_init (PACKAGE, GALAGO_INIT_CLIENT) || !galago_is_connected()) {
		g_printerr ("Can not initialise Galago integration");
		return;
	}

	g_print("Galago init\n");
	
	GList *services = galago_get_services(GALAGO_REMOTE,TRUE);
	while (services)
	{
	    GalagoService *service = services->data;
	    g_print("service: %s\n",galago_service_get_id(service));
	    
	    GList *accounts = galago_service_get_accounts(service,TRUE);
	    while (accounts)
	    {
		GalagoAccount *account = accounts->data;
		g_print("account %s\n",galago_account_get_username(account));
		
		accounts = g_list_next(accounts);
	    }
	    services = g_list_next(services);
	}
	
}

