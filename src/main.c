/* xproj103.c - Source file:         client système de surveillance */
/*
 * Copyright (c) 2012, by:      Julien BOULANGER
 *    All rights reserved.      10 SQUARE DES SORBIERS 
 *                              94160 SAINT MANDE 
 *                             <azman0101@hotmail.com>
 *
 * This file may be used subject to the terms and conditions of the
 * GNU Library General Public License Version 2, or any later version
 * at your option, as published by the Free Software Foundation.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <rpc/xdr.h>
#include <signal.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <ifaddrs.h>

#include "proc/devname.h"
#include "proc/wchan.h"
#include "proc/procps.h"
#include "proc/readproc.h"
#include "proc/escape.h"
#include "proc/sig.h"
#include "proc/sysinfo.h"
#include "proc/version.h"
#include "proc/whattime.h"

#include "xproj103.h"

int main(int argc,char* argv[])
{
    
const char format[]="%6lu %6lu %2u\n";
struct_if** ipaddress;
struct_cpu* cpuinfo;
int nb,i;
ipaddress = malloc(sizeof(struct_if));
cpuinfo = malloc(sizeof(struct_cpu));
char ch;                   /* service variables */
int long_opt_index = 0;
int longval, port = 5333, server_mode = 0;
char *host_or_ip = NULL;
char *my_argument;

struct option long_options[] = {        /* tableau long options. sensible à la casse */
        { "addr", 1, &longval, 'a' },      /* --addr ou -a  */
        { "server", 0, NULL, 's'  },      /* --server ou -s */
        { "sample", 1, &longval, 'S' },  /* retourne 'S', ou retourne 0 et initialise longval à 'S' */
        { "port", 1, &longval, 'p' },
        { 0,    0,    0,    0   }       /* terminating -0 item */
    };
    
    
    while ((ch = getopt_long(argc, argv, "+a:S:hp:s", long_options, &long_opt_index)) != -1) {
       switch (ch) {
           case 'a':  
               host_or_ip = optarg; /* 'a' et '--addr' indique l'hote à contacter par son IP ou son nom. */
               printf("Option a, not --addr. Argument %s.\n", host_or_ip);
               break;
           case 's':
               /* 's' and '--server' are confused (aliased) */
	       server_mode = 1;
               printf("Option s, or --server.\n");
               break;
           case 'S':
               /* 'S' et '--sample' donne la taille de l'échantillon CPU et doit être compris entre 2 et 10 */
	       
	       i = atoi(optarg);
	       num_updates = (i < 11) && (i > 1) ? i : 10;
               printf("Option -S --sample activée, [2 - 10].\n");
               break;
           case 'p':
               port = atoi(optarg);
               printf("Option p, not --port. Argument %d.\n", port);
               break;
           case 0:     /* this is returned for long options with option[i].flag set (not NULL). */
                       /* the flag itself will point out the option recognized, and long_opt_index is now relevant */
               switch (longval) {
		   case 'a':
                       /* '--check' is managed here */
                       host_or_ip = optarg;
                       printf("Option --addr, not -a (Array index: %d). Argument: %s.\n", long_opt_index, host_or_ip);
		      
                       break;
                   case 'S':
                        /* 'S' et '--sample' donne la taille de l'échantillon CPU et doit être compris entre 2 et 10 */
		        
			i = atoi(optarg);
			num_updates = (i < 11) && (i > 1) ? i : 10;
                       printf("Option -S --sample activée, [2 - 10].\n");
                       break;
                   case 'x':
                       /* '--extra' is managed here */
                       port = atoi(optarg);
                       printf("Option --port, not -p (Array index: %d). Argument: %d.\n", long_opt_index, port);	  
                       break;
                   /* there's no default here */
               }
               break;
           case 'h':   /* mind that h is not described in the long option list */
               printf("Usage: cmd [-a or --addr] [-b or --back] [-c or --check] [-x or --extra]\n en mode client, -a ou -addr indique l'adresse du serveur.\n En mode serveur, -s ou -server, -a ou -addr indique l'adresse que le serveur écoutera, si le paramètre n'est pas renseigné,\n le serveur écoutera toutes les interfaces.\n ");
               break;
           default:
               printf("You, lamah!\n");
       }
    }
    

if (server_mode == 1) {
srv_rcv(host_or_ip, port);
exit(1);
}
ip_get(ipaddress);
nb = sizeof(ipaddress);



cpu_get(cpuinfo);

//TODO: fonction free pour les structures struct_if struct_cpu

for ( i=0; ipaddress[i] != NULL; i++ ) {
  
printf("<%s>  %s\n", ipaddress[i]->ip, ipaddress[i]->name);
  
}
printf(format, unitConvert(cpuinfo->free_mem), unitConvert(cpuinfo->total_mem), (unsigned) (cpuinfo->cpu_use));

if ((ipaddress != NULL) && (cpuinfo != NULL))
{
  // TODO: reflechir à l'idée de création d'un thread pour envoyer les données
  Clt_snd(ipaddress, cpuinfo, host_or_ip, port);
  
}

free(ipaddress);
free(cpuinfo);
exit(0);

}
