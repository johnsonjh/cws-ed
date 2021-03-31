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
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "rec.h"
#include "window.h"
#include "ed_dec.h"

/******************************************************************************\
|Routine: rec_split
|Callby: insert
|Purpose: Splits a record in two.
|Arguments:
|    prec is the record to be split.
|    byt is the location of the split point. Everything from [byt] forward goes
|            to the new record.
\******************************************************************************/
void rec_split(prec,byt)
rec_ptr *prec;
Int byt;
{
	rec_ptr rec,new;
	Int i;

	rec = *prec;
	new = (rec_ptr)imalloc(sizeof(rec_node));
	new->data = (Char *)imalloc(rec->length - byt + 1);
	new->recflags = 1;	/* it is a freeable buffer */
	if((new->length = rec->length - byt) > 0)
		memcpy(new->data,&rec->data[byt],new->length);
	new->data[new->length] = '\0';
	if(rec == BASE)
	{
		insq(new,rec->prev);	/* insert a blank record just before the [eob] */
		*prec = new;	/* we be located at the new record, not [eob] */
	}
	else
	{
		for(i = 0;i < NMARK;i++)
			if(rec == MARKREC[i] && MARKBYT[i] >= byt)
			{
				MARKREC[i] = new;
				MARKBYT[i] -= byt;
			}
		if(rec == SELREC && SELBYT > byt)
		{
			SELREC = new;
			SELBYT -= byt;
		}
		rec_trim(prec,byt,rec->length);	/* trim what follows byt in original rec */
		insq(new,*prec);
	}
}

