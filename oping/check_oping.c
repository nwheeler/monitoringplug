/***
 * Monitoring Plugin - check_oping.c
 **
 *
 * check_oping - Check if a file can be downloaded from tftp.
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
 * $Id: check_tftp.c 417 2012-08-29 11:30:53Z marius $
 */

const char *progname  = "check_oping";
const char *progdesc  = "Check if a file can be downloaded from tftp.";
const char *progvers  = "0.1";
const char *progcopy  = "2010";
const char *progauth  = "Marius Rieder <marius.rieder@durchmesser.ch>";
const char *progusage = "-H host -F file [-t timeout] [-w warn] [-c crit]";

/* MP Includes */
#include "mp_common.h"
#include "curl_utils.h"
/* Default Includes */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
/* Library Includes */
#include <oping.h>

/* Global Vars */
const char *hostname = NULL;
const char *interface = NULL;
struct timespec interval;
int ttl = 0;
int ipv = AF_UNSPEC;
thresholds *rta_thresholds = NULL;
thresholds *lost_thresholds = NULL;
int packets = 5;
int quick = 0;

/* Function prototype */

int main (int argc, char **argv) {
    /* Local Vars */
    int			rv;
    pingobj_t 	*oping;
    pingobj_iter_t *iter;

    uint32_t dropped=0, num=0;
    int ttl=0;
    double rt, rta=0;
    char haddr[40];
    size_t data_len, buf_len;

    /* Set signal handling and alarm */
    if (signal(SIGALRM, timeout_alarm_handler) == SIG_ERR)
        critical("Setup SIGALRM trap failed!");

    /* Process check arguments */
    if (process_arguments(argc, argv) != OK)
        unknown("Parsing arguments failed!");

    /* Start plugin timeout */
    alarm(mp_timeout);

    /* Init liboping */
    oping = ping_construct();
    if (oping == NULL)
    	unknown("Can't initialize liboping");
    
    
    rv = ping_setopt(oping, PING_OPT_AF, &ipv);
    if (rv != 0)
        unknown("liboping setup Error: %s", ping_get_error(oping));
    rv = ping_setopt(oping, PING_OPT_DATA,
            "monitoringplug/check_oping based on liboping.           ");
    if (rv != 0)
        unknown("liboping setup Error: %s", ping_get_error(oping));
    if (interface) {
        rv = ping_setopt(oping, PING_OPT_DEVICE, (char *)interface);
        if (rv != 0)
            unknown("liboping setup Error: %s", ping_get_error(oping));
    }
    if (ttl) {
        rv = ping_setopt(oping, PING_OPT_TTL, &ttl);
        if (rv != 0)
            unknown("liboping setup Error: %s", ping_get_error(oping));
    }

    rv = ping_host_add(oping, hostname);
    if (rv != 0)
    	unknown("liboping setup Error: %s", ping_get_error(oping));
    	

    for (; packets > 0; packets--) {
        rv = ping_send(oping);
        if (rv < 0)
            critical("Send Error: %s", ping_get_error(oping));

	    for (iter = ping_iterator_get(oping); iter != NULL; iter = ping_iterator_next(iter)) {

	        buf_len = sizeof(dropped);
	        rv =  ping_iterator_get_info(iter, PING_INFO_DROPPED,
	                &dropped, &buf_len);

            buf_len = sizeof(rt);
	        rv =  ping_iterator_get_info(iter, PING_INFO_LATENCY,
    	        &rt, &buf_len);
	        rta += rt;

            buf_len = sizeof(num);
	        rv =  ping_iterator_get_info(iter, PING_INFO_SEQUENCE,
    	        &num, &buf_len);

	        data_len = 0;
	        rv = ping_iterator_get_info (iter, PING_INFO_DATA, NULL, &data_len);

            buf_len = sizeof(haddr);
	        rv =  ping_iterator_get_info(iter, PING_INFO_ADDRESS,
	                haddr, &buf_len);

            buf_len = sizeof(ttl);
	        ping_iterator_get_info (iter, PING_INFO_RECV_TTL, &ttl, &buf_len);
    	
	        if (mp_verbose > 0)
	            printf("%zu bytes from %s (%s): icmp_seq=%u ttl=%i time=%.2f ms\n",
	                data_len, hostname,  haddr, num, ttl, rt);
	    }

	    if (quick &&
	        get_status((float)(rta/num), rta_thresholds) == STATE_OK &&
	        get_status((dropped*100)/num, lost_thresholds) == STATE_OK) {
	        break;
	    } else if (packets > 1){
	        if (interval.tv_sec || interval.tv_nsec)
	            nanosleep(&interval, NULL);
	        else
	            sleep(1);
	    }

    }

    mp_perfdata_float("rta", (float)(rta/num), "s", rta_thresholds);
    mp_perfdata_int("pl", (int)(dropped*100)/num, "%", lost_thresholds);

    int result1, result2;

    result1 = get_status((float)(rta/num), rta_thresholds);
    result2 = get_status((dropped*100)/num, lost_thresholds);
    result1 = result1 > result2 ? result1 : result2;

    free_threshold(rta_thresholds);
    free_threshold(lost_thresholds);

    switch(result1) {
        case STATE_OK:
            ok("Packet loss = %d%, RTA = %.2f ms",
                    (int)(dropped*100)/num, (float)(rta/num));
            break;
        case STATE_WARNING:
            warning("Packet loss = %d%, RTA = %.2f ms",
                    (int)(dropped*100)/num, (float)(rta/num));
            break;
        case STATE_CRITICAL:
            critical("Packet loss = %d%, RTA = %.2f ms",
                    (int)(dropped*100)/num, (float)(rta/num));
            break;
    }

    critical("You should never reach this point.");
}

int process_arguments (int argc, char **argv) {
    int c;
    int option = 0;

    static struct option longopts[] = {
        MP_LONGOPTS_DEFAULT,
        MP_LONGOPTS_HOST,
        {"packets", required_argument, 0, 'p'},
        {"quick", no_argument, 0, 'q'},
        {"interval", required_argument, 0, 'i'},
        {"interface", required_argument, 0, 'I'},
        {"ttl", required_argument, 0, 'T'},
        MP_LONGOPTS_WC,
        MP_LONGOPTS_END
    };

    if (argc < 4) {
        print_help();
        exit(STATE_OK);
    }

    /* Set default */

    while (1) {
        c = mp_getopt(&argc, &argv, MP_OPTSTR_DEFAULT"H:46p:qi:I:T:w:c:", longopts, &option);

        if (c == -1 || c == EOF)
            break;

        getopt_46(c, &ipv);

        switch (c) {
            /* Hostname opt */
            case 'H':
                getopt_host(optarg, &hostname);
                break;
            /* Packets opt */
            case 'p':
                if (!is_integer(optarg))
                    usage("Illegal packets value '%s'.", optarg);
                packets = (int)strtol(optarg, NULL, 10);
                break;
            /* Quick opt */
            case 'q':
                quick = 1;
                break;
            /* Interface opt */
            case 'I':
                interface = optarg;
                break;
            /* Interval opt */
            case 'i': {
                double val;
                val = strtod(optarg, NULL);
                if (val <= 0)
                    usage("Illegal interval value '%s'.", optarg);
                interval.tv_sec = (int)val;
                interval.tv_nsec = ((int)(val*1000000000))%1000000000;
            }
                break;
            /* TTL opt */
            case 'T':
                if (!is_integer(optarg))
                    usage("Illegal ttl value '%s'.", optarg);
                ttl = (int)strtol(optarg, NULL, 10);
                if (ttl < 1 || ttl > 255)
                    usage("Illegal ttl value '%s'.", optarg);
                break;
            /* Warn/Crit opt */
            case 'w':
            case 'c': {
                char *rta, *pl;
                pl = optarg;
                rta = strsep(&pl, ",");
                if (rta == NULL || pl == NULL) {
                    usage("Waring/Critical threshold is <rta>,<pl>%%.");
                }
                if (c == 'w') {
                    if (setWarn(&rta_thresholds, rta, NOEXT) == ERROR)
                        usage("Illegal -w tra threshold '%s'.", rta);
                    if (setWarn(&lost_thresholds, pl, NOEXT) == ERROR)
                        usage("Illegal -w pl threshold '%s'.", pl);
                } else {
                    if (setCrit(&rta_thresholds, rta, NOEXT) == ERROR)
                        usage("Illegal -c tra threshold '%s'.", rta);
                    if (setCrit(&lost_thresholds, pl, NOEXT) == ERROR)
                        usage("Illegal -c pl threshold '%s'.", pl);
                }
            }
                break;
        }
    }

    /* Check requirements */
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
#ifdef USE_IPV6
    print_help_46();
#endif //USE_IPV6
}

/* vim: set ts=4 sw=4 et syn=c : */
