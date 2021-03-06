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


#include "xproj103.h"

int main(int argc,char* argv[])
{
    
const char format[]="%6lu %6lu %2u\n";
struct_if** ipaddress;
struct_cpu cpuinfo = { 0 };
int i = 0;
ipaddress = malloc(sizeof(struct_if*));
//cpuinfo = calloc(1, sizeof(struct_cpu));
char ch = ' ';                   /* service variables */
int long_opt_index = 0;
int longval = 0, port = 0, server_mode = 0;
char *host_or_ip = NULL;


struct option long_options[] = {        /* tableau long options. sensible à la casse */
        { "addr", 1, &longval, 'a' },      /* --addr ou -a  */
        { "server", 0, NULL, 's'  },      /* --server ou -s */
        { "sample", 1, &longval, 'S' },  /* retourne 'S', ou retourne 0 et initialise longval à 'S' */
        { "port", 1, NULL, 'p' },
        { 0,    0,    0,    0   }       /* terminating -0 item */
    };
    
    
    while ((ch = getopt_long(argc, argv, "+a:S:hp:s", long_options, &long_opt_index)) != -1) {
       switch (ch) {
           case 'a': 
	       if (!isIpAddress(optarg)) {
		 printf("Erreur: L'adresse passée n'est pas une IPv4.\n");
		 exit(EXIT_FAILURE) ;
		 
	      }
               host_or_ip = optarg; /* 'a' et '--addr' indique l'hote à contacter par son IP ou son nom. */
               printf("Option --addr active sur interface %s.\n", host_or_ip);
               break;
           case 's':

	       server_mode = 1;
               printf("Option s, or --server.\n");
	      break;
           case 'S':
               /* 'S' et '--sample' donne la taille de l'échantillon CPU et doit être compris entre 2 et 10 */
	       
	       i = atoi(optarg);
	       num_updates = (i < 11) && (i > 1) ? i : 10;
               printf("Option -S --sample active, [2 - 10].\n");
               break;
           case 'p':
	     /* 'p' et '--port' donne le port d'écoute et doit être compris entre 1 et 65535 */
               port = argtoport(optarg);   
               break;
           case 0:     
                     /* branchement utilisé pour les paramètres long */
               switch (longval) {
		   case 'a':
                       /* ''a' et '--addr' indique l'hote à contacter par son IP ou son nom.  */
                       host_or_ip = optarg;
                       printf("Option --addr active sur interface %s.\n", host_or_ip);
                       break;
		   case 's':

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
		     /* 'p' et '--port' donne le port d'écoute et doit être compris entre 1 et 65535 */
		       port = argtoport(optarg);
		       break;
                   /* there's no default here */
               }
               break;
           case 'h':   /* mind that h is not described in the long option list */
               printf("%s %s \n\tUsage: cmd [-a or --addr] [-p or --port] [-s or --server] [-S or --sample]\n\n\t - En mode client, -a ou -addr indique l'adresse du serveur.\n\t - En mode serveur, -s ou -server, -a ou -addr indique l'adresse que le serveur écoutera, si le paramètre n'est pas renseigné, le serveur écoutera toutes les interfaces.\n\n ", PACKAGE_NAME, PACKAGE_VERSION);
               exit(EXIT_SUCCESS);
	       break;
           default:
               printf("Args Error!\n");
       }
    }
   

for (;;) {
      ip_get(ipaddress);

      cpu_get(&cpuinfo);
      for ( i = 0; ipaddress[i] != NULL; i++ ) {
  
      printf("<%s>  %s\n", ipaddress[i]->ip, ipaddress[i]->name);
	
      }
      printf(format, unitConvert((&cpuinfo)->free_mem), unitConvert((&cpuinfo)->total_mem), (unsigned) ((&cpuinfo)->cpu_use));


      if ((ipaddress != NULL) && (&cpuinfo != NULL))
      {
	
	Clt_snd(ipaddress, &cpuinfo, host_or_ip, port);
	sleep(INTERVAL);
	
      }
}
//free(cpuinfo);
free_ipaddress(ipaddress);
exit(EXIT_SUCCESS);

}
