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
|Routine: rec_trim
|Callby: killer rec_split
|Purpose: Removes some data from within a record.
|Arguments:
|    prec is the record affected.
|    byt1 is the first byte to remove.
|    byt2 is the last byte to remove.
\******************************************************************************/
void rec_trim(prec,byt1,byt2)
rec_ptr *prec;
Int byt1,byt2;
{
	register Int l,i;
	register rec_ptr rec,new,pred;
	register Char *c;

	if(byt1 == byt2)
		return;
	rec = *prec;
	new = (rec_ptr)imalloc(sizeof(rec_node));
	l = rec->length - (byt2 - byt1);
	new->data = (Char *)imalloc(l + 1);
	new->recflags = 1;	/* it is a freeable buffer */
	if(l > 0)
	{
		c = new->data;
		if(byt1 > 0)
		{
			memcpy(c,rec->data,byt1);	/* copy first byt1 bytes */
			c += byt1;
		}
		if(byt2 < rec->length)
			memcpy(c,&rec->data[byt2],rec->length - byt2);	/* copy trailing bytes */
	}
	new->data[l] = 0;
	new->length = l;
	pred = rec->prev;
	remq(rec);
	if(rec->recflags & 1)
		ifree(rec->data);
	ifree(rec);
	fixup_recs(rec,new);
	if(rec == SELREC)
	{
		if(SELBYT > byt1 && SELBYT < byt2)
			SELBYT = (!byt1)? 0 : byt1 - 1;
		else if(SELBYT >= byt2)
			SELBYT -= byt2 - byt1;
		SELREC = new;
	}
	for(i = 0;i < NMARK;i++)
		if(MARKREC[i] == rec)
		{
			if(MARKBYT[i] >= byt1 && MARKBYT[i] < byt2)
				MARKBYT[i] = (!byt1)? 0 : byt1 - 1;
			else if(MARKBYT[i] >= byt2)
				MARKBYT[i] -= byt2 - byt1;
			MARKREC[i] = new;
		}
	insq(new,pred);
	*prec = new;
}

