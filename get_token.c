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

#include <string.h>

#include "ctyp_dec.h"

/******************************************************************************\
|Routine: get_token
|Callby: load_file parse_fnm
|Purpose: Gets the next token from a string. Returns 0 if no more tokens remain.
|Arguments:
|    buf is the string containing tokens. A token is deleted from the string
|            each time the routine is called.
|    tok is the returned token.
\******************************************************************************/
Int get_token(buf,tok)
Char *buf,*tok;
{
	register Char *s,*e;
	Char *from,*to;

	if(!strlen(buf))
		return(0);
	s = buf;
	while(isspace(*s++));
	if(!*--s)
		return(0);
	e = s;
	if(*e == '"')	/* handle quoting */
	{
		e++;
		while(1)
		{
			if(*e == '"')
				if(*(e - 1) != '\\')
					break;
			e++;
		}
		e++;
	}
	else	/* not a quoted string */
	{
		while(!isspace(*++e))
			if(!*e)
			{
				*(e + 1) = 0;	/* this could be dangerous */
				break;
			}
	}
	*e = 0;
	strcpy(tok,s);
	*e = ' ';
	for(from = e,to = buf;(*to++ = *from++););
	return(1);
}

