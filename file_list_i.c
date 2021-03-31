/*
 * Copyright (C) 1992 by Rush Record
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
#include "opsys.h"

#include <stdio.h>
#include <string.h>

static FILE *fd = NULL;
static Int mode = -1;/* 0 means get one file name,
                         1 means get from file list,
                         2 means get from wildcard
                        -1 means we should fail (no file names available) */
#define NAMESIZE 128
static Char name[NAMESIZE];	/* file specification. if first char is '-', spec is a list of file names in a file */
static Long context = 0;	/* used for find_file */

extern Char *filename();

/******************************************************************************\
|Routine: file_list_init
|Callby: parse_fnm
|Purpose: Implements file list editing, and wildcards. List editing means
|         editing a sequence of files whose names are taken from a file. The
|         file_list_init routine stores either the name of such a list file,
|         or a wildcarded file specification. file_list_next returns
|         either the next name in the list, or the next file that matches the
|         wildcard.
|Arguments:
|    filenm is either the name of the file that has file names, or a
|            file specification that contains wildcards.
\******************************************************************************/
void file_list_init(filenm)
Char *filenm;
{
	if(fd)	/* if file list is open, close it */
	{
		if(fd != stdin)
			fclose(fd);
		fd = NULL;
	}
	if(context)	/* free LIB$FIND_FILE vm if still allocated */
	{
		find_files_end(context);
		context = 0;
	}
	strcpy(name,filenm);	/* save the file name */
	if(filenm[0] == '-' && (filenm[1] == 'f' || filenm[1] == 'F'))
	{
		mode = 1;
		if(filenm[2])
			fd = fopen(filenm + 2,"r");
		else
			fd = stdin;
	}
	else if(strchr(filenm,'*') || strchr(filenm,'?') || strstr(filenm,"..."))
		mode = 2;
	else
		mode = 0;
}

/******************************************************************************\
|Routine: file_list_next
|Callby: parse_fnm
|Purpose: Returns the name of the next file to be edited. See the description
|         of 'mode' above for a description of the various functional modes.
|Arguments:
|    retfile is the returned file name.
\******************************************************************************/
Int file_list_next(retfile)
Char *retfile;
{
	Char *p;

	switch(mode)
	{
		case 0:	/* single file */
			strcpy(retfile,name);
			mode = -1;
			return(1);
		case 1:	/* file list */
			if(!fd)	/* unable to open file list */
				return(0);
			if(!fgets(name,NAMESIZE,fd))
			{
				if(fd != stdin)
					fclose(fd);
				fd = NULL;
				mode = -1;
				return(0);
			}
			if((p = (Char *)strchr((char *)name,'\n')))
				*p = '\0';
			strcpy(retfile,name);
			return(1);
		case 2:	/* wildcarded file */
			if(!context)
			{
				envir_subs(name);
				if(!(context = find_files(name)))
					return(0);
			}
fileAgain:
			if(next_file(context,retfile))
			{
				if(!strcmp(filename(retfile),".."))	/* leave out the .. directory (unix only) */
					goto fileAgain;
				if((p = (Char *)strchr((char *)retfile,' ')))
					*p = '\0';
				return(1);
			}
			find_files_end(context);
			context = 0;
			mode = -1;
			return(0);
		default:	/* fail due to single file already done, or abort requested */
			return(0);
	}
 }

/******************************************************************************\
|Routine: file_list_abort
|Callby: command
|Purpose: Terminates any multiple-file editing.
|Arguments:
|    none
\******************************************************************************/
void file_list_abort()
{
	mode = -1;	/* insure failure */
}

