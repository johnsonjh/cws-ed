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

#include "rec.h"
#include "window.h"
#include "ed_dec.h"
#include "buffer.h"

/******************************************************************************\
|Routine: copier
|Callby: killer
|Purpose: Handles copying of text from windows to buffer. Returns 1 if anything was
|         actually copied, else 0.
|Arguments:
|    offset is the offset from the current position to the other end of the
|            text to be deleted.
|    buffer is the buffer into which the copied data is to be saved.
|    append is a flag that causes the copied data to be appended to the buffer,
|            instead of replacing it entirely.
\******************************************************************************/
Int copier(offset,buffer,append,boxmode)
Int offset,append,boxmode;
buf_ptr buffer;
{
	rec_ptr otherrec,mergerec;
	register rec_ptr rec;
	Int firstcol,lastcol,firstbyt,lastbyt,otherbyt,nrecs,modified_pos;
	register Int i;

	modified_pos = 0;	/* flag that we had to move up from <end> before beginning the copy operation */
	if(!offset)	/* if we are on the select range marker, and killing, handle specially */
	{
		if(append)	/* appending nothing does nothing */
			return(1);
		buffer_empty(buffer);	/* killing nothing only empties the buffer */
		return(1);
	}
	else if(offset < 0)	/* we are deleting backwards from current position */
	{
		if(CURREC == BASE->next && !CURBYT)	/* ignore backward kills at top of file */
		{
			abort_key();
			return(0);
		}
/* check for some pathological things */
		if(boxmode)
		{
		 	if(CURREC == BASE)
			{
				if(CURREC->next == BASE)	/* box cut in empty buffer does nothing except to paste buffer */
				{
					if(append)	/* appending nothing does nothing */
						return(1);
					buffer_empty(buffer);	/* killing nothing only empties the buffer */
					return(1);
				}
				WANTCOL = 1;
				up_arrow(1);	/* move up to start of last record in buffer */
				offset += CURREC->length + 1;	/* offset is less negative, by the length of the last record, plus 1 for the newline */
				modified_pos = 1;
			}
		}
/* find the other end of the buffer, backwards */
		otherrec = CURREC;
		otherbyt = CURBYT;
		nrecs = 1;
		while(offset++ < 0)
		{
			if(!otherbyt)
			{
				if(otherrec->prev == BASE)
				{
					abort_key();
					return(0);
				}
				otherrec = otherrec->prev;
				otherbyt = otherrec->length;
				nrecs++;
			}
			else
				otherbyt--;
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
/* save records to buffer */
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
	else	/* we are deleting forwards from the current position */
	{
		if(CURREC == BASE)	/* ignore forward kills at end of file */
		{
			abort_key();
			return(0);
		}
/* find the other end of the buffer, forwards */
		otherrec = CURREC;
		otherbyt = CURBYT;
		nrecs = 1;
		while(offset-- > 0)
		{
			if(otherrec == BASE)
			{
				abort_key();
				return(0);
			}
			if(otherbyt == otherrec->length)
			{
				otherrec = otherrec->next;
				otherbyt = 0;
				nrecs++;
			}
			else
				otherbyt++;
		}
/* check for some pathological things */
		if(boxmode)
		{
		 	if(otherrec == BASE)
			{
				otherrec = otherrec->prev;
				otherbyt = 0;
				nrecs--;
				offset -= otherrec->length + 1;	/* offset is smaller, by the length of the last record, plus 1 for the newline */
			}
/* if box cut in effect, get column range */
			firstcol = get_column(CURREC,CURBYT);
			lastcol = get_column(otherrec,otherbyt);
			if(firstcol > lastcol)
			{
				i = firstcol;
				firstcol = lastcol;
				lastcol = i;
			}
		}
/* save records to buffer */
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
	CURBYT = get_colbyt(CURREC,CURCOL);
	if(modified_pos)
		down_arrow(1);
	return(1);
}

