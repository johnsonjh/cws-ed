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
|Routine: rec_insert
|Callby: insert
|Purpose: Handles insertion into a record.
|Arguments:
|    prec is the record containing the insertion point. A new prec is returned.
|    byt is the position of the insertion point in that record.
|    length is the length of the data to insert.
|    data is the data to insert.
\******************************************************************************/
void rec_insert(prec,byt,length,data)
rec_ptr *prec;
Int byt,length;
Char *data;
{
	register Int l;
	register rec_ptr rec,new,pred;
	register Char *from,*to;

	if(!length)
		return;
	rec = *prec;
	new = (rec_ptr)imalloc(sizeof(rec_node));
	l = rec->length + length;
	new->data = (Char *)imalloc(l + 1);
	new->recflags = 1;	/* it is a freeable buffer */
	from = rec->data;
	to = new->data;
	if(byt > 0)
	{
		memcpy(to,from,byt);
		to += byt;
		from += byt;
	}
	memcpy(to,data,length);
	to += length;
	if(byt < rec->length)
		memcpy(to,from,rec->length - byt);
	new->data[l] = 0;
	new->length = l;
	pred = rec->prev;
	remq(rec);
	if(rec->recflags & 1)
		ifree(rec->data);
	ifree(rec);
	fixup_recs(rec,new);
	for(l = 0;l < NMARK;l++)
		if(MARKREC[l] == rec)
		{
			MARKREC[l] = new;
			if(byt < MARKBYT[l])
				MARKBYT[l] += length;
		}
	if(rec == SELREC)
	{
		SELREC = new;
		if(byt < SELBYT)
			SELBYT += length;
	}
	insq(new,pred);
	*prec = new;
}

/******************************************************************************\
|Routine: rec_overstrike
|Callby: insert
|Purpose: Handles overstrike modification of a record.
|Arguments:
|    rec is the record containing the insertion point.
|    byt is the starting position of the overlay in that record.
|    length is the length of the data to overlay.
|    data is the data to overlay.
\******************************************************************************/
void rec_overstrike(prec,byt,length,data)
rec_ptr *prec;
Int byt,length;
Char *data;
{
	rec_ptr rec;
	Int i;
	Char *p;
	
	rec = *prec;
	if(byt == rec->length)
		rec_insert(prec,byt,length,data);
	else
	{
		if((i = byt + length) > rec->length)
		{
			p = (Char *)imalloc(++i);
			p[--i] = '\0';
			memcpy(p,rec->data,rec->length);
			if(rec->recflags & 1)
				ifree(rec->data);
			rec->data = p;
			rec->length = i;
		}
		memcpy(rec->data + byt,data,length);
	}
}

