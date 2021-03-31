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
#include "handy.h"
#include "buffer.h"

/******************************************************************************\
|Routine: change_case
|Callby: edit
|Purpose: Changes the case of all bytes (inclusive) between the current
|         position and another position at a given offset from the current
|         position. Optionally moves to the other end of the region.
|Arguments:
|    offset is the offset from the current position to the other end of the
|            region that is to be affected.
|    otherend is a flag that indicates that the cursor should be left at the
|            other end of the changed region.
|    table is the case-changing table, or NULL if they want capitalization.
\******************************************************************************/
Int change_case(offset,otherend,table)
register Int offset,otherend;
Uchar *table;
{
	register rec_ptr otherrec,rec;
	register Int otherbyt,otherrow,i,nrecs;

	if(offset < 0)	/* we are working backwards from current position */
	{
		if(CURREC == BASE->next && !CURBYT)	/* ignore backward at top of file */
			return(0);
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
					return(0);
				otherrec = otherrec->prev;
				otherbyt = otherrec->length;
				otherrow--;
				nrecs++;
			}
			else
				otherbyt--;
		}
/* alter the record data structures and redisplay */
		if(nrecs == 1)
		{
			rec_chgcas(CURREC,otherbyt,CURBYT,table);	/* was entirely in current record */
			paint(CURROW,CURROW,FIRSTCOL);	/* repaint current record */
		}
		else
		{
/* deal with the database */
			rec = otherrec;
			if(otherbyt != otherrec->length)
				rec_chgcas(rec,otherbyt,otherrec->length,table);      /* trim the tail off the first record if necessary */
			rec = rec->next;
			for(i = 1;i < nrecs - 1;i++)	/* remove intervening records, if any */
			{
				rec_chgcas(rec,0,rec->length,table);
				rec = rec->next;
			}
			if(CURBYT != 0)
				rec_chgcas(rec,0,CURBYT,table);	/* trim the front off the last record if necessary */
/* fix the display */
			paint(max(TOPROW,otherrow),CURROW,FIRSTCOL);
		}
	}
/* we are deleting forwards from the current position */
	else
	{
		if(CURREC == BASE)	/* ignore forward at end of file */
			return(0);
/* find the other end of the buffer, forwards */
		otherrec = CURREC;
		otherbyt = CURBYT;
		otherrow = CURROW;
		nrecs = 1;
		while(offset-- > 0)
		{
			if(otherrec == BASE)
				return(0);
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
/* alter the record data structures and redisplay */
		if(nrecs == 1)
		{
			rec_chgcas(CURREC,CURBYT,otherbyt,table);	/* kill was entirely in current record */
			paint(CURROW,CURROW,FIRSTCOL);	/* repaint current record */
		}
		else
		{
/* deal with the database */
			rec = CURREC;
			if(CURBYT != rec->length)
				rec_chgcas(CURREC,CURBYT,CURREC->length,table);       /* trim the tail off the first record if necessary */
			rec = rec->next;
			for(i = 1;i < nrecs - 1;i++)	/* remove intervening records, if any */
			{
				rec_chgcas(rec,0,rec->length,table);
				rec = rec->next;
			}
			if(otherbyt != 0)
				rec_chgcas(rec,0,otherbyt,table);	/* do the front on the last record if necessary */
/* fix the display */
			paint(CURROW,min(BOTROW,otherrow),FIRSTCOL);
		}
/* establish new location and column context */
	}
	if(otherend)
	{
		CURROW = otherrow;
		CURREC = otherrec;
		CURBYT = otherbyt;
		fix_scroll();
		WANTCOL = CURCOL;
	}
	return(1);
}

