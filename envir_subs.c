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

#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "ctyp_dec.h"

/******************************************************************************\
|Routine: envir_subs
|Callby: command init_term journal load_file main match_search parse_fnm wincom
|Purpose: Translates strings containing environment variables. Returns a pointer to the translation.
|Arguments:
|	file is the string that may contain vars.
\******************************************************************************/
void envir_subs(file)
char *file;
{
#ifndef VMS
	Char temp[512],envbuf[512],*a,*aa,*b,c,*e;
	
/* move the string, translating environment variables found there */
	memset(temp,0,sizeof(temp));
	for(a = file,b = temp;(c = *a++);)
	{
		if(c == '$')
		{
			for(e = envbuf,aa = a;isalnum(*aa) || *aa == '_';*e++ = *aa++);
			*e = '\0';
			if((e = getenv(envbuf)))
			{
				strcpy(b,e);
				b += strlen(e);
				a = aa;
			}
			else
			{
				for(e = envbuf;*e;e++)
					*e = toupper(*e);
				if((e = getenv(envbuf)))
				{
					strcpy(b,e);
					b += strlen(e);
					a = aa;
				}
				else
					*b++ = c;
			}
		}
		else if(c == '~')
		{
			if((e = getenv("HOME")))
			{
				strcpy(b,e);
				b += strlen(e);
			}
			else
				*b++ = '~';
		}
		else
			*b++ = c;
	}
	strcpy(file,temp);
#endif
}

