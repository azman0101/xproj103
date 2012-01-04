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


int main()
{

new_format();
    
data_struct** ipaddress;

//memset(&ipaddress, 0 ,sizeof(ipaddress));
ipaddress = malloc(sizeof(data_struct));
  
ip_get(ipaddress);
 printf("\taddress: <%s>\n interface: %s\n", ipaddress[0]->ip, ipaddress[0]->name);

//TODO: fonction free pour la structure data_struct

exit(0);

}
