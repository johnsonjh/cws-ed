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

typedef struct sort_str *sort_ptr;	/* structures for sorting by keys */
typedef struct sort_str
{
	rec_ptr rec;
	Char *key;
	Int length,recflags;
} sort_node;

extern Int *cqsort_s();

/******************************************************************************\
|Routine: sort_recs
|Callby: command
|Purpose: Sorts a group of records into alphanumeric order. If select is active,
|         sorts the select range (including entire records, no partial records
|         allowed), otherwise sorts the entire buffer. If box mode is active,
|         and select is active, uses the box as the key for the sort.
|Arguments:
|    reverse is a flag indicating that the sort should be in decreasing order.
\******************************************************************************/
void sort_recs(reverse)
Int reverse;
{
	buf_node keybuf;	/* copy-only buffer for sorting */
	register rec_ptr r,kr;
	register Int i,*tag,nrecs,dir;
	Int offset;
	register rec_ptr firstrec,lastrec,save_selrec;
	register sort_ptr s;
	Int must_trim;	/* flag that we need to trim the records after the sort */
	
	must_trim = 0;
	if((save_selrec = SELREC))	/* sort the select range */
	{
		dir = get_seldir();
		unselect();
		if(dir > 0)
		{
			firstrec = CURREC;
			lastrec = ((SELBYT || BOXCUT)? save_selrec : save_selrec->prev);
		}
		else
		{
			firstrec = save_selrec;
			lastrec = ((CURBYT || BOXCUT)? CURREC : CURREC->prev);
		}
		if(BOXCUT && lastrec == BASE)
			lastrec = BASE->prev;
	}
	else	/* sort the entire file */
	{
		firstrec = BASE->next;
		lastrec = BASE->prev;
	}
	for(nrecs = 1,r = firstrec;r != lastrec;r = r->next,nrecs++);	/* count records to be sorted */
	s = (sort_ptr)imalloc((nrecs + 1) * sizeof(sort_node));	/* get sort structures */
	if(save_selrec && BOXCUT)
	{
		offset = get_offset(save_selrec,SELBYT,dir);
		keybuf.nrecs = 0;
		keybuf.direction = 1;
		keybuf.first = (rec_ptr)&keybuf.first;
		keybuf.last = (rec_ptr)&keybuf.first;
		copy_only(1);
		killer_box();
		killer(offset,&keybuf,0);	/* get a copy of the key data */
		for(i = 0,r = firstrec,kr = keybuf.first;i < nrecs;r = r->next,kr = kr->next)
		{
			s[i].rec = r;	/* load sort structures */
			s[i].recflags = r->recflags;
			s[i++].key = kr->data;
		}
		tag = cqsort_s(nrecs,(Char *)&s[0].key,sizeof(s[0]));
		buffer_empty(&keybuf);
		if(WINDOW[CURWINDOW].diredit)	/* we need to trim the select range */
			must_trim = 1;
	}
	else
	{
		for(i = 0,r = firstrec;i < nrecs;r = r->next)
		{
			s[i].rec = r;	/* load sort structures */
			s[i++].key = r->data;
		}
		tag = cqsort_s(nrecs,(Char *)&s[0].key,sizeof(s[0]));
	}
	if(reverse)
		for(i = 0;i < nrecs;i++)
		{
			s[nrecs - 1 - i].key = s[tag[i]].rec->data;
			s[nrecs - 1 - i].recflags = s[tag[i]].rec->recflags;
			s[nrecs - 1 - i].length = s[tag[i]].rec->length;
		}
	else
		for(i = 0;i < nrecs;i++)
		{
			s[i].key = s[tag[i]].rec->data;
			s[i].recflags = s[tag[i]].rec->recflags;
			s[i].length = s[tag[i]].rec->length;
		}
	for(i = 0,r = firstrec;i < nrecs;i++,r = r->next)
	{
		r->data = s[i].key;
		r->length = s[i].length;
		r->recflags = s[i].recflags;
	}
	ifree(s);
	ifree(tag);
	ref_window(CURWINDOW);
	SELREC = save_selrec;
	if(must_trim)
		trim();
}

