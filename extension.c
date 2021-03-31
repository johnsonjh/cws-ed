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

#include <string.h>

/******************************************************************************\
|Routine: extension
|Callby: load_file output_file
|Purpose: Returns the address of a file's extension, or NULL if it has no
|         extension. This is opsys-sensitive.
|Arguments:
|    file is the file name being analyzed.
\******************************************************************************/
Char *extension(file)
Char *file;
{
	register Char *c,v;

	c = file + strlen(file);
	while(c != file)
	{
		if((v = *--c) == '.')
			return(c);
#ifdef VMS
		if(v == ']' || v == '>' || v == ':')
#else
#ifdef GNUDOS
		if(v == '/' || v == '\\')
#else
		if(v == '/')
#endif
#endif
			return(0);
	}
	return(0);
}

/******************************************************************************\
|Routine: filename
|Callby: cfg command help load_file store_param
|Purpose: Returns the address of a file's name. This is opsys-sensitive.
|Arguments:
|    file is the file name being analyzed.
\******************************************************************************/
Char *filename(file)
Char *file;
{
	register Char *c,v;

	c = file + strlen(file);
	while(c != file)
	{
		v = *--c;
#ifdef VMS
		if(v == ']' || v == '>' || v == ':')
#else
#ifdef GNUDOS
		if(v == '/' || v == '\\' || v == ':')	/* CWS 11-93 (DOS) */
#else
		if(v == '/')
#endif
#endif
			return(++c);
	}
	return(file);
}

