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

#include "rec.h"
#include "window.h"
#include "ed_dec.h"

/******************************************************************************\
|Routine: get_offset
|Callby: edit output_file sort_recs trim word_fill
|Purpose: Calculates the offset from the current position to a spot in the file.
|Arguments:
|    rec is the record containing the spot.
|    byt is the byte offset in that record.
|    dir is the direction from the cursor to the spot.
\******************************************************************************/
Int get_offset(rec,byt,dir)
rec_ptr rec;
Int byt,dir;
{
	register rec_ptr r;
	register Int offset,b;

	r = CURREC;
	b = CURBYT;
	if(r == rec && b == byt)
		return(0);
	if(r == rec)
		return(byt - b);
	offset = 0;
	if(dir >= 0)
	{
		while(r != rec)
		{
			offset += r->length - b + 1;
			b = 0;
			r = r->next;
		}
		return(offset + byt);
	}
	else
	{
		b = r->length - b;
		while(r != rec)
		{
			offset += r->length - b + 1;
			b = 0;
			r = r->prev;
		}
		return(-(offset + r->length - byt));
	}
}

