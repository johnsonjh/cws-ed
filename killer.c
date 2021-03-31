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

#include "memory.h"
#include "rec.h"
#include "window.h"
#include "ed_dec.h"
#include "buffer.h"

static Char copyonly = 0;
static Char boxcut = 0;
	
/******************************************************************************\
|Routine: copy_only
|Callby: command edit output_file sort_recs
|Purpose: Sets the copyonly flag, which makes the next kill operation not alter
|         the buffer, but just copy the selection to the paste buffer.
|Arguments:
|    none
\******************************************************************************/
void copy_only(value)
Int value;
{
	copyonly = value;
}

/******************************************************************************\
|Routine: killer_box
|Callby: edit output_file sort_recs
|Purpose: A one-shot to enable box cuts.
|Arguments:
|    none
\******************************************************************************/
void killer_box()
{
	boxcut = 1;
}

/******************************************************************************\
|Routine: killer
|Callby: edit output_file sort_recs word_fill
|Purpose: Handles deletion of text from buffers.
|Arguments:
|    offset is the offset from the current position to the other end of the
|            text to be deleted.
|    buffer is the buffer into which the deleted data is to be saved.
|    append is a flag that causes the deleted data to be appended to the buffer,
|            instead of replacing it entirely.
\******************************************************************************/
void killer(offset,buffer,append)
Int offset;
buf_ptr buffer;
Int append;
{
	register rec_ptr otherrec,next,newbotrec;
	rec_ptr rec,mergerec;
	Int otherbyt,nrecs,frombottom,fromtop,fixeob,firstcol,lastcol,firstbyt,lastbyt,otherrow,boxmode;
	register Int i;

	boxmode = boxcut;
	boxcut = 0;
	if(copyonly)
	{
		copyonly = 0;
		copier(offset,buffer,append,boxmode);
		return;
	}
	if(!offset && !boxmode)	/* if we are on the select range marker, and killing, handle specially */
	{
		if(append)	/* appending nothing does nothing */
			return;
		buffer_empty(buffer);	/* killing nothing only empties the buffer */
		return;
	}
	if(offset <= 0)	/* we are deleting backwards from current position */
	{
		if(CURREC == BASE->next && !CURBYT && offset)	/* ignore backward kills at top of file */
		{
			abort_key();
			return;
		}
/* check for some pathological things */
		if(boxmode)
		{
		 	if(CURREC == BASE)
			{
				if(CURREC->next == BASE)	/* box cut in empty buffer does nothing except to paste buffer */
				{
					if(append)	/* appending nothing does nothing */
						return;
					buffer_empty(buffer);	/* killing nothing only empties the buffer */
					return;
				}
				WANTCOL = 1;
				up_arrow(1);	/* move up to start of last record in buffer */
				offset += CURREC->length + 1;	/* offset is less negative, by the length of the last record, plus 1 for the newline */
			}
			fixeob = 0;			
		}
		else
		{
/* make a note of whether the EOB is likely to get wiped */
			if(CURREC == BASE && !BOTREC)
				fixeob = 1;
			else
				fixeob = 0;
		}
/* find the other end of the buffer, backwards */
		otherrec = CURREC;
		otherbyt = CURBYT;
		otherrow = CURROW;
		nrecs = 1;
		while(offset++ < 0)
		{
			if(!otherbyt)
			{
				if(otherrec->prev == BASE)
				{
					abort_key();
					return;
				}
				otherrec = otherrec->prev;
				otherbyt = otherrec->length;
				nrecs++;
				otherrow--;
			}
			else
				otherbyt--;
		}
		if(otherrow < TOPROW)
			otherrow = TOPROW;
/* if box cut in effect, get column range */
		if(boxmode)
		{
			firstcol = get_column(CURREC,CURBYT);
			lastcol = get_column(otherrec,otherbyt);
			if(firstcol > lastcol)
			{
				i = firstcol;
				firstcol = lastcol;
				lastcol = i;
			}
		}
/* save records to buffer, if it was passed */
		if(buffer != 0)
		{
			if(!append)
				buffer_empty(buffer);
			else
			{
				if(buffer->first == (rec_ptr)&buffer->first)    /* if the buffer is empty, forget about appending */
					append = 0;
				else
					mergerec = buffer->last;
			}
			if(boxmode)
			{
				for(rec = otherrec;rec != CURREC->next;rec = rec->next)
				{
					firstbyt = get_colbyt(rec,firstcol);	/* get_colbyt must convert tabs to spaces, make record long enough, etc. */
					lastbyt = get_colbyt(rec,lastcol);
					buffer_append(buffer,rec,firstbyt,lastbyt + 1);
				}
				if(append)
					buffer->nrecs += nrecs;
				else
					buffer->nrecs = nrecs;
			}
			else
			{
				if(otherrec == CURREC)
					buffer_append(buffer,CURREC,otherbyt,CURBYT);
				else
				{
					rec = otherrec;
					buffer_append(buffer,rec,otherbyt,rec->length);
					rec = rec->next;
					for(i = 1;i < nrecs - 1;i++)
					{
						buffer_append(buffer,rec,0,rec->length);
						rec = rec->next;
					}
					buffer_append(buffer,rec,0,CURBYT);
				}
				if(!append)
					buffer->nrecs = nrecs;
				else
				{
					buffer->nrecs += nrecs - 1;
					rec_merge(&mergerec);
				}
			}
			buffer->direction = -1;
		}
/* alter the record data structures and redisplay */
		if(boxmode)
		{
			for(rec = otherrec,i = nrecs;i--;rec = rec->next)
			{
				firstbyt = get_colbyt(rec,firstcol);
				lastbyt = get_colbyt(rec,lastcol);
				if(OVERSTRIKE)
					memset(rec->data + firstbyt,' ',lastbyt - firstbyt + 1);
				else
					rec_trim(&rec,firstbyt,lastbyt + 1);
			}
			CURREC = rec->prev;
			paint(otherrow,CURROW,FIRSTCOL);
			if((i = nrecs - 1))
				up_arrow(i);	/* move up to where select marker was */
			CURBYT = get_colbyt(CURREC,firstcol - 1) + 1;	/* position to smallest of curcol, selcol */
		}
		else
		{
			if(nrecs == 1)
			{
				if(OVERSTRIKE)
					memset(CURREC->data + otherbyt,' ',CURBYT - otherbyt);
				else
					rec_trim(&CURREC,otherbyt,CURBYT);	/* kill was entirely in current record */
				paint(CURROW,CURROW,FIRSTCOL);	/* repaint current record */
				CURBYT = otherbyt;
			}
			else
			{
/* figure out where lines scroll into view */
				if(!OVERSTRIKE)
				{
					for(NEWTOPREC = TOPREC,i = 1;NEWTOPREC->prev != BASE && i < nrecs;NEWTOPREC = NEWTOPREC->prev,i++);
					fromtop = i - 1;	/* how many we are able to get from the top */
					frombottom = nrecs - i;	/* how many we are not able to get from top, thus must get from bottom */
				}
/* deal with the database */
				rec = otherrec;	/* don't use otherrec any more, rec_trim may change it */
				if(otherbyt != otherrec->length)
				{
					if(OVERSTRIKE)
						memset(rec->data + otherbyt,' ',otherrec->length - otherbyt);
					else
						rec_trim(&rec,otherbyt,otherrec->length);       /* trim the tail off the first record if necessary */
				}
				rec = rec->next;
				if(OVERSTRIKE)
					for(i = 1;i < nrecs - 1;i++)	/* blank out intervening records, if any */
					{
						memset(rec->data,' ',rec->length);
						rec = rec->next;
					}
				else
					for(i = 1;i < nrecs - 1;i++)	/* remove intervening records, if any */
					{
						next = rec->next;
						remq(rec);
						if(rec->recflags & 1)
							ifree(rec->data);
						ifree(rec);
						rec = next;
					}
				if(CURBYT != 0)
				{
					if(OVERSTRIKE)
						memset(rec->data,' ',CURBYT);
					else
						rec_trim(&rec,0,CURBYT);	/* trim the front off the last record if necessary */
				}
/* fix the display */
				if(OVERSTRIKE)
				{
					paint(otherrow,CURROW,FIRSTCOL);
					CURREC = otherrec;
					CURBYT = otherbyt;
					CURROW = otherrow;
				}
				else
				{
					CURREC = rec->prev;
					rec_merge(&CURREC);	/* merge the two records */
					TOPREC = NEWTOPREC;
					if(fromtop > 0)
					{
						if(fromtop >= CURROW - TOPROW)	/* complete repaint from TOPROW to CURROW - 1 */
							paint(TOPROW,CURROW - 1,FIRSTCOL);
						else
						{
							marge(TOPROW,CURROW);
							move(TOPROW,1);
							ins_line(fromtop);
							paint(TOPROW,TOPROW + fromtop - 1,FIRSTCOL);
							marge(TOPROW,BOTROW);
						}
					}
					if(frombottom > 0)
					{
						CURROW -= frombottom;
						if(CURROW != TOPROW)
							marge(CURROW,BOTROW);
						move(CURROW,1);
						del_line(frombottom);
						if(CURROW != TOPROW)
							marge(TOPROW,BOTROW);
						move(BOTROW - frombottom + 1,1);
						paint(BOTROW - frombottom + 1,BOTROW,FIRSTCOL);
					}
					paint(CURROW,CURROW,FIRSTCOL);
					for(BOTREC = TOPREC,i = TOPROW;BOTREC != BASE && i < BOTROW;BOTREC = BOTREC->next,i++);
					if(i < BOTROW)
						BOTREC = 0;
					CURBYT = otherbyt;
				}
			}
		}
	}
/********************************************************************************/
/********************************************************************************/
/********************************************************************************/
	else	/* we are deleting forwards from the current position */
	{
		if(CURREC == BASE)	/* ignore forward kills at end of file */
		{
			abort_key();
			return;
		}
/* find the other end of the buffer, forwards */
		otherrec = CURREC;
		otherbyt = CURBYT;
		otherrow = CURROW;
		nrecs = 1;
		while(offset-- > 0)
		{
			if(otherrec == BASE)
			{
				abort_key();
				return;
			}
			if(otherbyt == otherrec->length)
			{
				otherrec = otherrec->next;
				otherbyt = 0;
				otherrow++;
				nrecs++;
			}
			else
				otherbyt++;
		}
		if(otherrow > BOTROW)
			otherrow = BOTROW;
/* check for some pathological things */
		if(boxmode)
		{
			fixeob = 0;			
		 	if(otherrec == BASE)
			{
				otherrec = otherrec->prev;
				otherbyt = 0;
				nrecs--;
				offset -= otherrec->length + 1;	/* offset is smaller, by the length of the last record, plus 1 for the newline */
			}
		}
		else
		{
/* make a note of whether the EOB is likely to get wiped */
			if(otherrec == BASE && !BOTREC)
				fixeob = 1;
			else
				fixeob = 0;
		}
/* if box cut in effect, get column range */
		if(boxmode)
		{
			firstcol = get_column(CURREC,CURBYT);
			lastcol = get_column(otherrec,otherbyt);
			if(firstcol > lastcol)
			{
				i = firstcol;
				firstcol = lastcol;
				lastcol = i;
			}
		}
/* save records to buffer, if it was passed */
		if(buffer != 0)
		{
			if(!append)
				buffer_empty(buffer);
			else
			{
				if(buffer->first == (rec_ptr)&buffer->first)    /* if the buffer is empty, forget about appending */
					append = 0;
				else
					mergerec = buffer->last;
			}
			if(boxmode)
			{
				for(rec = CURREC;rec != otherrec->next;rec = rec->next)
				{
					firstbyt = get_colbyt(rec,firstcol);	/* get_colbyt must convert tabs to spaces, make record long enough, etc. */
					lastbyt = get_colbyt(rec,lastcol);
					buffer_append(buffer,rec,firstbyt,lastbyt + 1);
				}
				if(append)
					buffer->nrecs += nrecs;
				else
					buffer->nrecs = nrecs;
			}
			else
			{
				if(otherrec == CURREC)
					buffer_append(buffer,CURREC,CURBYT,otherbyt);
				else
				{
					buffer_append(buffer,CURREC,CURBYT,CURREC->length);
					rec = CURREC->next;
					for(i = 1;i < nrecs - 1;i++)
					{
						buffer_append(buffer,rec,0,rec->length);
						rec = rec->next;
					}
					buffer_append(buffer,rec,0,otherbyt);
				}
				if(!append)
					buffer->nrecs = nrecs;
				else
				{
					buffer->nrecs += nrecs - 1;
					rec_merge(&mergerec);
				}
			}
			buffer->direction = 1;
		}
/* alter the record data structures and redisplay */
		if(boxmode)
		{
			for(rec = CURREC,i = nrecs;i--;rec = rec->next)
			{
				firstbyt = get_colbyt(rec,firstcol);
				lastbyt = get_colbyt(rec,lastcol);
				if(OVERSTRIKE)
					memset(rec->data + firstbyt,' ',lastbyt - firstbyt + 1);
				else
					rec_trim(&rec,firstbyt,lastbyt + 1);
			}
			paint(CURROW,otherrow,FIRSTCOL);
			CURBYT = get_colbyt(CURREC,firstcol - 1) + 1;	/* position to smallest of curcol, selcol */
		}
		else
		{
			if(nrecs == 1)
			{
				if(OVERSTRIKE)
					memset(CURREC->data + CURBYT,' ',otherbyt - CURBYT);
				else
					rec_trim(&CURREC,CURBYT,otherbyt);	/* kill was entirely in current record */
				paint(CURROW,CURROW,FIRSTCOL);	/* repaint current record */
			}
			else
			{
/* figure out where lines scroll into view */
				if(!OVERSTRIKE)
				{
					for(newbotrec = BOTREC,i = 1;newbotrec != BASE && newbotrec != 0 && i < nrecs;newbotrec = newbotrec->next,i++);
					frombottom = i - 1;	/* how many we are able to get from the bottom */
					fromtop = nrecs - i;	/* how many we are not able to get from bottom, thus must get from top */
					for(NEWTOPREC = TOPREC,i = 0;NEWTOPREC->prev != BASE && i < fromtop;NEWTOPREC = NEWTOPREC->prev,i++);
					if((i = fromtop - i) > 0)
					{
						frombottom += i;	/* if top can't give us that many, get from bottom instead, leaving [eob] floating */
						fromtop -= i;	/* reduce acquisition from top */
					}
				}
/* deal with the database */
				if(CURBYT != CURREC->length)
				{
					if(OVERSTRIKE)
						memset(CURREC->data + CURBYT,' ',CURREC->length - CURBYT);
					else
						rec_trim(&CURREC,CURBYT,CURREC->length);        /* trim the tail off the first record if necessary */
				}
				rec = CURREC->next;
				if(OVERSTRIKE)
					for(i = 1;i < nrecs - 1;i++)	/* blank out intervening records, if any */
					{
						memset(rec->data,' ',rec->length);
						rec = rec->next;
					}
				else
					for(i = 1;i < nrecs - 1;i++)	/* remove intervening records, if any */
					{
						next = rec->next;
						remq(rec);
						if(rec->recflags & 1)
							ifree(rec->data);
						ifree(rec);
						rec = next;
					}
				if(otherbyt != 0)
				{
					if(OVERSTRIKE)
						memset(rec->data,' ',otherbyt);
					else
						rec_trim(&rec,0,otherbyt);	/* trim the front off the last record if necessary */
				}
/* fix the display */
				if(OVERSTRIKE)
					paint(CURROW,otherrow,FIRSTCOL);
				else
				{
					rec_merge(&CURREC);	/* merge the two records */
					TOPREC = NEWTOPREC;
					if(fromtop > 0)
					{
						if(CURROW + fromtop - 1 < BOTROW)
							marge(TOPROW,CURROW + fromtop - 1);
						move(TOPROW,1);
						ins_line(fromtop);
						paint(TOPROW,TOPROW + fromtop - 1,FIRSTCOL);
						if(CURROW + fromtop - 1 < BOTROW)
							marge(TOPROW,BOTROW);
						CURROW += fromtop;
					}
					if(frombottom > 0)
					{
						if(frombottom >= BOTROW - CURROW)	/* complete repaint from CURROW + 1 to BOTROW */
							paint(CURROW + 1,BOTROW,FIRSTCOL);
						else
						{
							if(CURROW != TOPROW)
								marge(CURROW,BOTROW);
							move(CURROW,1);
							del_line(frombottom);
							if(CURROW != TOPROW)
								marge(TOPROW,BOTROW);
							move(BOTROW - frombottom + 1,1);
							paint(BOTROW - frombottom + 1,BOTROW,FIRSTCOL);
						}
					}
					paint(CURROW,CURROW,FIRSTCOL);
					for(BOTREC = TOPREC,i = TOPROW;BOTREC != BASE && i < BOTROW;BOTREC = BOTREC->next,i++);
					if(i < BOTROW)
						BOTREC = 0;
				}
			}
		}
	}
/* insure that EOB is there if it is above the bottom */
	if(fixeob)
	{
		for(rec = TOPREC,i = TOPROW;rec != BASE;rec = rec->next,i++);
		move(i,1);
		eob();
	}
/* establish new location and column context */
	fix_scroll();
	return;
}

