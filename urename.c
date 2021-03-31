/*
 * Copyright (C) 1993 by Charles Sandmann (sandmann@clio.rice.edu)
 * 
 * This file is part of ED.
 * 
 * ED is free software; you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation.
 * 
 * ED is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with ED
 * (see the file COPYING).  If not, write to the Free Software Foundation, 675
 * Mass Ave, Cambridge, MA 02139, USA.
 */

/* Provides unix-line rename; will clobber the destination if exists */
#include <stdio.h>
#include <sys\stat.h>
#include <errno.h>

int unix_rename(char *from, char *to)
{
	int i;
	struct stat statbuf;
	
	if(!(i = rename(from,to)))
		return i;
#ifdef WIN32
	if(errno != EEXIST)			/* NT specific */
		return i;
#endif
	if(!stat(from,&statbuf))	/* Check if from file exists */
		if(!stat(to,&statbuf))	/* Check if to file exists */
			remove(to);			/* Both do, delete to file */
	return rename(from,to);		/* Retry rename */
}
