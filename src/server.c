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


int i = 0;
char ch = ' ';                   /* service variables */
int long_opt_index = 0;
int longval = 0, port = 5333, server_mode = 0;
char *host_or_ip = NULL;

struct_pth ipport;

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
	       if (!isIpAddress(optarg)) { printf("Erreur: L'adresse passée n'est pas une IPv4.\n"); exit(EXIT_FAILURE) ; }
               ipport.hostorip = optarg; /* 'a' et '--addr' indique l'hote à contacter par son IP ou son nom. */
               printf("Option --addr active sur interface %s.\n", ipport.hostorip);
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
               ipport.port = argtoport(optarg);         
               break;
           case 0:     
                     /* branchement utilisé pour les paramètres long */
               switch (longval) {
		   case 'a':
                       /* ''a' et '--addr' indique l'hote à contacter par son IP ou son nom.  */
                      if (!isIpAddress(optarg)) { printf("Erreur: L'adresse passée n'est pas une IPv4.\n"); exit(EXIT_FAILURE) ; }
		      ipport.hostorip = optarg; /* 'a' et '--addr' indique l'hote à contacter par son IP ou son nom. */
		      printf("Option --addr active sur interface %s.\n",  ipport.hostorip);
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
                       ipport.port = argtoport(optarg);
                      
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
    

if (server_mode == 1) {
    struct sigaction a;
    pthread_t th1;
    void* ret;
    a.sa_handler = (__sighandler_t) xp_sighandler;
    a.sa_flags =  0;
    sigemptyset(&a.sa_mask);
    
    
    sigaction(SIGTSTP, &a, NULL);
    sigaction(SIGINT, &a, NULL);
    sigaction(SIGTERM, &a, NULL);
   
  /*  if (setuid(0) == -1)
    {
      perror("Setuid error");
      exit(errno);
      
     }*/

    if (pthread_create(&th1, NULL, srv_rcv, (void *)&ipport) <0 ) {
	fprintf(stderr, "thread serveur erreur\n");
    }
    
    for (i=0; i < 10000; i++) {
	//printf("Main loop: %d\n", i);
	sleep(1);
    }
    
   // srv_handle_rcv_data();
    (void)pthread_join(th1, &ret);
    
 
    exit(EXIT_SUCCESS);
}

exit(EXIT_SUCCESS);

}
