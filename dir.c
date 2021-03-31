/*
 * Copyright (C) 1992 by Rush Record (rhr@clio.rice.edu)
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
#include "opsys.h"

#include <stdio.h>

#include "rec.h"
#include "window.h"
#include "ed_dec.h"

/* note, none of these routines should be called by systems that set NO_FTP. */
/* but, we have them to keep the linker happy */

#ifdef NO_FTP
void dir_destroy(window)
Int window;
{
	return;
}

rec_ptr dir_find(window,dirname)
Int window;
Char *dirname;
{
	return(NULL);
}

void dir_store(window,dirname,buffer)
Int window;
Char *dirname;
rec_ptr buffer;
{
	return;
}

#else	/* not NO_FTP */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct dir_str *dir_ptr;
typedef struct dir_str
{
	dir_ptr next;
	Char *dirname;
	rec_ptr buffer;
} dir_node;
static dir_ptr base[32] = {
	NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
	NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
	NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
	NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};	/* this limits ED to 32 simultaneous windows */

/******************************************************************************\
|Routine: dir_destroy
|Callby: main wincom
|Purpose: Removes a directory from the list of already-visited directories.
|Arguments:
|    window is the window number of the diredit buffer to remove.
\******************************************************************************/
void dir_destroy(window)
Int window;
{
	dir_ptr p,l;
	Int i;
	
	for(l = base[window];l;)
	{
		ifree(l->dirname);
		p = l->next;
		ifree(l);
		l = p;
	}
	for(i = window;i < NWINDOWS;i++)	/* squidge all the base pointers */
		base[i] = base[i + 1];
	base[NWINDOWS - 1] = NULL;
	return;
}

/******************************************************************************\
|Routine: dir_find
|Callby: edit
|Purpose: Retrieves an already-visited diredit buffer by name.
|Arguments:
|    window is the window number of the diredit tree to search within.
|    dirname is the name of the dir to search for.
\******************************************************************************/
rec_ptr dir_find(window,dirname)
Int window;
Char *dirname;
{
	dir_ptr l;
	
	for(l = base[window];l;l = l->next)
		if(!strcmp(dirname,l->dirname))
			return(l->buffer);
	return(NULL);
}

/******************************************************************************\
|Routine: dir_store
|Callby: edit insert_win main
|Purpose: Adds a diredit buffer to the list for a window.
|Arguments:
|    window is the window number.
|    dirname is the name of the directory that is to be stored.
|    buffer is the diredit buffer to be stored.
\******************************************************************************/
void dir_store(window,dirname,buffer)
Int window;
Char *dirname;
rec_ptr buffer;
{
	dir_ptr new;
	
	if(!dir_find(window,dirname))
	{
		new = (dir_ptr)imalloc(sizeof(dir_node));
		new->next = base[window];
		base[window] = new;
		new->dirname = (Char *)imalloc(strlen(dirname) + 1);
		strcpy(new->dirname,dirname);
		new->buffer = buffer;
	}
	return;
}

#endif

