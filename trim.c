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
#include "rec.h"
#include "window.h"
#include "ed_dec.h"
#include "buffer.h"
#include "buf_dec.h"

/******************************************************************************\
|Routine: trim
|Callby: command
|Purpose: Trims trailing blanks from the select range or the entire file.
|Arguments:
|    none
\******************************************************************************/
void trim()
{
	register Int byt;
	Int offset;
	register Char *a;
	register rec_ptr r1,r2;

	if(!SELREC)	/* do the whole file */
	{
		r2 = WINDOW[CURWINDOW].base;
		r1 = r2->next;
	}
	else
	{
		offset = get_offset(SELREC,SELBYT,get_seldir());
		if(!offset)
		{
			r1 = CURREC;
			r2 = r1->next;
		}
		else if(offset < 0)	/* select marker preceeds current position */
		{
			r1 = CURREC;	/* find the other end of the buffer, backwards */
			byt = CURBYT;
			while(offset++ < 0)
			{
				if(!byt)
				{
					if(r1->prev == BASE)
						break;
					r1 = r1->prev;
					byt = r1->length;
				}
				else
					byt--;
			}
			r2 = CURREC->next;
		}
		else
		{
			
			r1 = r2 = CURREC;	/* find the other end of the buffer, forwards */
			byt = CURBYT;
			while(offset-- > 0)
			{
				if(r2 == BASE)
					break;
				if(byt == r2->length)
				{
					r2 = r2->next;
					byt = 0;
				}
				else
					byt++;
			}
			r2 = r2->next;
		}
		unselect();
	}
	while(r1 != r2)
	{
		if(r1->length)
		{
			a = r1->data + r1->length;
			while(isspace(*--a))
				if(a < r1->data)
					break;
			r1->length = ++a - r1->data;
			*(r1->data + r1->length) = '\0';
		}
		r1 = r1->next;
	}
	if(CURBYT > CURREC->length)
		CURBYT = CURREC->length;
	CURCOL = get_column(CURREC,CURBYT);
	move(CURROW,CURCOL);
}

