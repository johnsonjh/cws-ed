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

static buf_node parabuf = {0,1,0,0};	/* buffer to hold paragraph delimiter string during fill operation */
static rec_node sprec = {0,0,1," "};	/* record structure representing a single space character */
static buf_node spbuf = {1,1,0,0};	/* buffer holding the space char */
static Char word_delims[] = {' ',9};	/* word delimiters for fill operation */

/******************************************************************************\
|Routine: word_fill
|Callby: edit
|Purpose: Performs word fill on the select range.
|Arguments:
|    none
\******************************************************************************/
void word_fill()
{
	Int i,save_direction,was_displaying;
	Int offset;
	rec_ptr r1,r2,r;

	was_displaying = puttest();
	if(!parabuf.first)
	{
		parabuf.first = (rec_ptr)&parabuf.first;
		parabuf.last = (rec_ptr)&parabuf.first;
	}
	if(!sprec.next)
	{
		sprec.next = (rec_ptr)&spbuf.first;
		sprec.prev = (rec_ptr)&spbuf.first;
		spbuf.first = &sprec;
		spbuf.last = &sprec;
	}
	if(PAGE_BREAK_LENGTH != 0)
		load_buffer(&parabuf,PARAGRAPH_BREAK,PARAGRAPH_BREAK_LENGTH);
	else
		buffer_empty(&parabuf);
	init_word_table(2,word_delims);	/* load the special word delimiters (space, tab) */
	if(SELREC == CURREC)
		r1 = r2 = CURREC;
	else if(get_seldir() < 0)
	{
		r1 = SELREC;
		r2 = CURREC;
	}
	else
	{
		r1 = CURREC;
		r2 = SELREC;
	}
	putoff();
	unselect();
	save_direction = DIRECTION;
	if((offset = get_offset(r1,0,-1)) != 0)
		left_arrow((offset < 0)? -offset : offset);
	if(r1 == BASE)
		return;
	r = r1;
	match_init("ebnnn");
	while(1)
	{
		DIRECTION = 1;
		while(r == CURREC && CURCOL <= WRAP_MARGIN + 1)
		{
			if((offset = match_search(CURREC,CURBYT,&parabuf)) != 0)
			{
				if(CURREC == r2)
					goto done;
				for(i = 0;i < offset;i++)
				{
					right_arrow(1);
					if(CURREC == r2)
						goto done;
				}
				r = CURREC;
			}
			else
				move_word(1);
		}
		if(r == CURREC)
		{
			DIRECTION = -1;
			move_word(1);	/* back up one word */
			if(!CURBYT)	/* if we backed up all the way to the beginning of the record */
			{
				DIRECTION = 1;	/* advance to where we were */
				move_word(1);
				if(CURBYT != CURREC->length)	/* if in the middle of the record still */
				{
					killer(-1L,0,0);	/* kill the preceeding space */
					carriage_return(1);	/* insert the newline */
				}
				else	/* else, it's just a big word, leave it on a line by itself */
				{
					move_word(1);
					if(CURREC == r2)
						goto done;
				}
			}
			else
			{
				killer(-1L,0,0);	/* kill the preceeding space */
				carriage_return(1);	/* insert the newline */
				if(r == r2)
					r2 = CURREC;
			}
		}
		else
		{
			if(r1 == r2)
				goto done;
			if(CURREC == r2)
				goto done;
			if(get_column(CURREC->prev,CURREC->prev->length) + 1 + find_word(1) <= WRAP_MARGIN)
			{
				killer(-1L,0,0);
				insert(&spbuf);
			}
		}
		r = CURREC;
	}
done:
	putout();
	if(was_displaying)
		puton();
	init_word_table(NWORD_DELIMITERS,WORD_DELIMITERS);	/* reload the original word delimiters */
	DIRECTION = save_direction;
	ref_display();
}

