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
#include <string.h>

#include "ctyp_dec.h"
#include "library.h"

static Char waiting = 0;
static Char keyword[128];	/* keywords may not be more than 127 bytes long */
static Int level;

/******************************************************************************\
|Routine: help_get_keyword
|Callby: help_load
|Purpose: Reads the next keyword from the online help file. Returns the tree
|         level of the keyword, or -1 if end of file.
|Arguments:
|    fp is the file pointer for the online help file.
|    keyptr is the returned buffer containing the found keyword.
\******************************************************************************/
Int help_get_keyword(fp,keyptr)
FILE *fp;
Char **keyptr;
{
	Int status;

	if(waiting)
		waiting = 0;
	else
	{
		while((status = my_fscanf(fp,"%d%s",&level,keyword)) != 2)
			if(status == EOF)
				return(-1);
	}
	*keyptr = (Char *)imalloc(strlen(keyword)+1);
	strcpy(*keyptr,keyword);
	return(level);
}

/******************************************************************************\
|Routine: help_get_text
|Callby: help_load
|Purpose: Reads and stores online help text. Returns a pointer to the text
|         buffer.
|Arguments:
|    fp is the file pointer of the online help file.
\******************************************************************************/
Char *help_get_text(fp)
FILE *fp;
{
	Char buf[16384],*p,*textptr;

	p = buf;
	while(1)
	{
		if(!(fgets(p,sizeof(buf) - (p - buf),fp)))
			break;
		if(my_sscanf(p,"%d%s",&level,keyword) == 2 && !isspace(*p))
		{
			waiting = 1;
			*p = 0;
			break;
		}
		p += strlen(p);
	}
	textptr = (Char *)imalloc(p - buf + 1);
	strcpy(textptr,buf);
	return(textptr);
}

