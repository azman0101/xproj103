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


double time_so_far()
{
 struct timeval tp;

 if (gettimeofday(&tp, (struct timezone *) NULL) == -1)
   perror("gettimeofday");
 return ((double) (tp.tv_sec)) +
   (((double) tp.tv_usec) * 0.000001 );
}


int main()
{

long USER_HZ_UNIT=sysconf(_SC_CLK_TCK);
printf("\n_SC_CLK_TCK:%ld\n", USER_HZ_UNIT);

FILE *f1;
	double ti, tf;
	
	char c[10];
	int i1,i2,i3,i4,i5,i6;

	ti = time_so_far();
	f1 = fopen("/proc/stat", "r");
	fscanf(f1, "%s\t%d\t%d\t%d\n", c, &i1, &i2, &i3);
	fclose(f1);

	usleep(1000000);

	tf = time_so_far();
	f1 = fopen("/proc/stat", "r");
	fscanf(f1, "%s\t%d\t%d\t%d\n", c, &i4, &i5, &i6);
	fclose(f1);

	int t = (i4+i5+i6)-(i1+i2+i3);
	printf("cpu usage: %.1f%%\n", (t / ((tf-ti) * 100)) * 100);


return 0;

}
