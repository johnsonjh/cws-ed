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
|Routine: rec_merge
|Callby: copier killer
|Purpose: Handles merging of records (deletion of "separators").
|Arguments:
|    prec is the record that is to be merged with its successor.
\******************************************************************************/
void rec_merge(prec)
rec_ptr *prec;
{
	register rec_ptr rec,next,new,pred;
	register Int l1,l2,i;

	rec = *prec;
	if((next = rec->next) == BASE)
	{
		if(!rec->length)
		{
			remq(rec);
			fixup_recs(rec,BASE);
			if(rec == SELREC)
			{
				SELREC = BASE;
				SELBYT = 0;
			}
			for(i = 0;i < NMARK;i++)
				if(rec == MARKREC[i])
				{
					MARKREC[i] = BASE;
					MARKBYT[i] = 0;
				}
		}
		return;
	}
	new = (rec_ptr)imalloc(sizeof(rec_node));
	l1 = rec->length;
	l2 = next->length;
	new->data = (Char *)imalloc(l1 + l2 + 1);
	new->recflags = 1;	/* it is a freeable buffer */
	if(l1 > 0)
		memcpy(new->data,rec->data,l1);
	if(l2 > 0)
		memcpy(&new->data[l1],next->data,l2);
	new->length = l1 + l2;
	new->data[l1 + l2] = 0;
	pred = rec->prev;
	remq(rec);
	if(rec->data && (rec->recflags & 1))
		ifree(rec->data);
	ifree(rec);
	remq(next);
	if(next->data && (next->recflags & 1))
		ifree(next->data);
	ifree(next);
	fixup_recs(rec,new);
	fixup_recs(next,new);
	if(rec == SELREC || next == SELREC)
	{
		if(next == SELREC)
			SELBYT += l1;
		SELREC = new;
	}
	for(i = 0;i < NMARK;i++)
		if(rec == MARKREC[i] || next == MARKREC[i])
		{
			if(next == MARKREC[i])
				MARKBYT[i] += l1;
			MARKREC[i] = new;
		}
	insq(new,pred);
	*prec = new;
}

