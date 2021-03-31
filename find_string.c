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
#include "buffer.h"

static rec_ptr saverec;	/* these are so ALL can quickly continue search from found one */
static Int savebyt;

/******************************************************************************\
|Routine: find_string
|Callby: do_grep edit
|Purpose: Implements string search. Returns the offset of the string from
|         the current position, or zero if not enough matches were found.
|Arguments:
|    rec is the record from which the search starts.
|    byt is the byte offset in that record.
|    dir is the direction of the search (-1 means backwards, 1 forwards).
|    search is the buffer containing the search string.
|    flags is the array of search flags. See match_search for a description.
|    repeat is the repeat count applied to the search.
\******************************************************************************/
Int find_string(rec,byt,dir,search,flags,repeat)
register rec_ptr rec;
register Int byt,dir;
buf_ptr search;
Char *flags;
Int repeat;
{
	register Int i;
	Int offset;

	match_init(flags);
	offset = 0;
	for(i = 0;i < repeat;i++)
	{
		if(dir > 0)
		{
			while(rec != BASE)
			{
				if(++byt > rec->length)
				{
					byt = 0;
					if((rec = rec->next) == BASE)
						return(0);
				}
				offset++;
				if(match_search(rec,byt,search))
					goto next;
			}
			return(0);
		}
		else	/* search backwards */
		{
			if(rec == BASE && rec->prev != BASE)
			{
				rec = rec->prev;
				byt = rec->length;
				offset--;
			}
			while(rec != BASE)
			{
				if(--byt < 0)
				{
					if((rec = rec->prev) == BASE)
						return(0);
					byt = rec->length;
				}
				offset--;
				if(match_search(rec,byt,search))
					goto next;
			}
			return(0);
		}
next : 
		;
	}
	saverec = rec;
	savebyt = byt;
	return(offset);
}

/******************************************************************************\
|Routine: get_string_loc
|Callby: do_grep
|Purpose: Allows caller to get string match position, as starting point for further searches.
|         NOTE: this routine returns garbage if the last call to find_string didn't return a hit.
|Arguments:
|    rec is the returned record pointer.
|    byt is the returned byte offset within the record.
\******************************************************************************/
void get_string_loc(rec,byt)
rec_ptr *rec;
Int *byt;
{
	*rec = saverec;
	*byt = savebyt;
}

