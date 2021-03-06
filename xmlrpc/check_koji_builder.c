/***
 * Monitoring Plugin - check_koji_builder.c
 **
 *
 * check_koji_builder - Check if a koji builder is online.
 *
 * Copyright (C) 2012 Marius Rieder <marius.rieder@durchmesser.ch>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * $Id$
 */

const char *progname  = "check_koji_builder";
const char *progdesc  = "Check if a koji builder is online.";
const char *progvers  = "0.1";
const char *progcopy  = "2011";
const char *progauth  = "Marius Rieder <marius.rieder@durchmesser.ch>";
const char *progusage = "--url URL --hostname NAME";

/* MP Includes */
#include "mp_common.h"
#include "xmlrpc_utils.h"
/* Default Includes */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
/* Library Includes */
#include <xmlrpc.h>
#include <xmlrpc_client.h>

/* Global Vars */
const char *hostname = NULL;
const char *url = NULL;

int main (int argc, char **argv) {
    /* Local Vars */
    xmlrpc_env env;
    xmlrpc_value *result;
    xmlrpc_bool ready;
    xmlrpc_bool enabled;
    xmlrpc_double load;
    xmlrpc_int hostid;
    xmlrpc_int tasks;
    const char *name = NULL;

    /* Set signal handling and alarm */
    if (signal(SIGALRM, timeout_alarm_handler) == SIG_ERR)
        critical("Setup SIGALRM trap failed!");

    /* Process check arguments */
    if (process_arguments(argc, argv) != OK)
        unknown("Parsing arguments failed!");

    /* Start plugin timeout */
    alarm(mp_timeout);

    /* Create XMLRPC env */
    env = mp_xmlrpc_init();

    /* Get Koji API Version */
    result = xmlrpc_client_call(&env, url, "getHost", "(s)", hostname);
    unknown_if_xmlrpc_fault(&env);

    if (xmlrpc_value_type(result) != XMLRPC_TYPE_STRUCT)
       critical("kojid: %s not found.", hostname);

    xmlrpc_decompose_value(&env, result, "{s:s,s:b,s:b,s:i,s:d,*}",
            "name", &name, "ready", &ready, "enabled", &enabled,
            "id", &hostid, "task_load", &load);
    unknown_if_xmlrpc_fault(&env);

    result = xmlrpc_client_call(&env, url, "listTasks", "({s:i}{s:b})", "host_id", hostid, "countOnly", 1);
    unknown_if_xmlrpc_fault(&env);

    xmlrpc_decompose_value(&env, result, "i", &tasks);
    unknown_if_xmlrpc_fault(&env);

    mp_perfdata_int("tasks", tasks, "c", NULL);
    mp_perfdata_float("tasks_load", (float)load, "", NULL);

    if (!enabled)
       critical("kojid: %s is disabled.", name);
    if (!ready)
       warning("kojid: %s is not ready.", name);
    ok("kojid: %s is ready.", name);

}

int process_arguments (int argc, char **argv) {
    int c;
    int option = 0;

    static struct option longopts[] = {
        MP_LONGOPTS_DEFAULT,
        MP_LONGOPTS_HOST,
        MP_LONGOPTS_EOPT,
        {"url", required_argument, NULL, (int)'U'},
        MP_LONGOPTS_END
    };


    while (1) {
        c = mp_getopt(&argc, &argv, MP_OPTSTR_DEFAULT"H:U:", longopts, &option);

        if (c == -1 || c == EOF)
            break;

        switch (c) {
            case 'H':
               getopt_host(optarg, &hostname);
               break;
            /* Local opts */
            case 'U':
                getopt_url(optarg, &url);
                break;
        }
    }

    /* Check requirements */
    if (!url)
        usage("URL is mandatory.");
    if (!is_url_scheme(url, "http") && !is_url_scheme(url, "https"))
        usage("Only http(s) urls are allowed.");
    if (!hostname)
       usage("Hostname is mandatory.");

    return(OK);
}

void print_help (void) {
    print_revision();
    print_copyright();

    printf("\n");

    printf("Check description: %s", progdesc);

    printf("\n\n");

    print_usage();

    print_help_default();
    print_help_host();

    printf(" -U, --url=URL\n");
    printf("      URL of the Koji-Hub XML-RPC api.\n");
}

/* vim: set ts=4 sw=4 et syn=c : */
