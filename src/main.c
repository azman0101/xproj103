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



return 0;

}
