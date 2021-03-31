/*
 * Copyright (C) 1992 by Rush Record
 * Copyright (C) 1993 by Charles Sandmann (sandmann@clio.rice.edu)
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

#include "handy.h"
#include "rec.h"
#include "window.h"
#include "ed_dec.h"
#include "buffer.h"

static Char boxmode = 0;

/******************************************************************************\
|Routine: insert_box
|Callby: edit
|Purpose: A one-shot to enable box paste.
|Arguments:
|    none
\******************************************************************************/
void insert_box()
{
	boxmode = 1;
}

/******************************************************************************\
|Routine: insert
|Callby: carriage_ret cfrendly command edit include_file openline word_fill
|Purpose: Handles insertion of new data at the current position.
|Arguments:
|    buf is the buffer containing the record(s) to be inserted.
\******************************************************************************/
void insert(buf)
register buf_ptr buf;
{
	static Int recursing = 0;
	register Int i,j,fixeob,column;
	rec_ptr r,new;
	register rec_ptr b;
	register Char c;

	if(!buf->nrecs)	/* ignore attempts to insert empty buffers */
		return;
	if(buf->nrecs == 1)	/* special handling for simple insertions */
	{
		if(CURREC == BASE && buf->last->length > 0)
		{
			carriage_return(1);
			left_arrow(1);
		}
		if(OVERSTRIKE)
			rec_overstrike(&CURREC,CURBYT,buf->first->length,buf->first->data);
		else
			rec_insert(&CURREC,CURBYT,buf->first->length,buf->first->data);
		paint(CURROW,CURROW,FIRSTCOL);
		if(buf->direction == 1)
		{
			CURBYT += buf->first->length;
			WANTCOL = CURCOL = get_column(CURREC,CURBYT);
			if(puttest() && CLOSE_PARENS)
			{
				c = buf->first->data[buf->first->length - 1];
				for(i = 0;i < PAREN_STRING_LENGTH;i++)
					if(c == PAREN_STRING[i])
					{
						if(i & 1)
							match_paren(PAREN_STRING[i - 1],PAREN_STRING[i],-1);
						else
							match_paren(PAREN_STRING[i],PAREN_STRING[i + 1],1);
						break;
					}
			}
		}
		return;
	}
	if((boxmode || OVERSTRIKE) && !recursing)
	{
		recursing = 1;
		if(CURREC == BASE)
		{
			carriage_return(1);
			left_arrow(1);
		}
		column = get_column(CURREC,CURBYT);
		i = get_colbyt(CURREC,column - 1) + 1;	/* this converts the current record, so we can get the real column number */
		column = get_column(CURREC,i);
		for(i = buf->nrecs,r = CURREC;i && (r != BASE);i--,r = r->next);	/* check whether we need to extend the file to make room */
		if(i)
		{
			if((j = buf->nrecs - i))
				down_arrow(j);
			carriage_return(i);
			up_arrow(buf->nrecs);
			if(column > 1)
				right_arrow(column - 1);
		}
		if(boxmode)
			for(r = CURREC,b = buf->first,i = buf->nrecs;i--;b = b->next,r = r->next)
			{
				j = get_colbyt(r,column - 1) + 1;
				if(OVERSTRIKE)
					rec_overstrike(&r,j,b->length,b->data);
				else
					rec_insert(&r,j,b->length,b->data);
			}
		else
			for(r = CURREC,b = buf->first,i = buf->nrecs;i--;b = b->next,r = r->next)
			{
				j = get_colbyt(r,column - 1) + 1;
				if(r == CURREC)
					rec_overstrike(&r,j,b->length,b->data);
				else
					rec_overstrike(&r,0,b->length,b->data);
			}
		if((i = CURROW + buf->nrecs - 1) > BOTROW)
			i = BOTROW;
		paint(CURROW,i,FIRSTCOL);
		recursing = 0;
		if(buf->direction == 1 && !boxmode)
		{
			for(i = buf->nrecs - 1;i--;CURREC = CURREC->next,CURROW++);
			CURBYT = max(0,buf->last->length - 1);
			if(boxmode)
				CURBYT += j;
			fix_scroll();
		}
		boxmode = 0;
		return;
	}
/* non-box-mode, regular multiline insertion */
	rec_split(&CURREC,CURBYT);	/* split the current record in two */
	if(CURREC->next == BASE && buf->last->length > 0)
	{
		carriage_return(1);
		left_arrow(1);
		fixeob = 1;
	}
	else
		fixeob = 0;
	rec_insert(&CURREC,CURBYT,buf->first->length,buf->first->data);	/* append the first insert record to the first half */
	r = CURREC;	/* all records will be inserted following to this one */
	b = buf->first;
	for(i = 2;i < buf->nrecs;i++)	/* insert all complete records */
	{
		b = b->next;
		rec_copy(b,&new);
		insq(new,r);
		r = new;
	}
	if((r = r->next) != BASE)	/* point at the second half of the split record */
		rec_insert(&r,0,buf->last->length,buf->last->data);
	if(TOPREC == BASE)
		TOPREC = BASE->next;
	if(buf->direction == -1)
	{
		for(r = CURREC,i = CURROW;i < BOTROW && r != BASE;i++,r = r->next);
		if(i == BOTROW)
			BOTREC = r;
		else
			BOTREC = 0;
		ins_line(buf->nrecs - 1);
		paint(CURROW,BOTROW,FIRSTCOL);
	}
	else
	{
		CURREC = r;
		CURBYT = buf->last->length;
		for(i = CURROW;i > TOPROW;i--,r = r->prev);
		TOPREC = r;
		if(CURROW > TOPROW)
		{
			marge(TOPROW,CURROW);
			move(TOPROW,1);
			del_line(buf->nrecs - 1);
			marge(TOPROW,BOTROW);
		}
		paint(TOPROW,CURROW,FIRSTCOL);
	}
	CURCOL = get_column(CURREC,CURBYT);
	if(fixeob)
	{
		for(i = TOPROW,r = TOPREC;i <= BOTROW,r != BASE;i++,r = r->next);
		if(r == BASE)
		{
			move(i,1);
			eob();
		}
	}
	fix_scroll();
}

